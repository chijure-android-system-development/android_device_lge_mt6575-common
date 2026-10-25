/*
 * libmt6575-ril: rild.libpath shim in front of the stock mtk-ril.so, so CM's
 * rild and telephony framework stay unpatched.
 *
 * mtk-ril.so takes MediaTek's six-entry RIL_Env, not AOSP's three: after
 * the AOSP callbacks it uses RequestProxyTimedCallback, QueryMyChannelId
 * (the AT channel of a request) and QueryMyProxyIdByThread. The stock rild
 * fills them from MTK's libril, which this device also ships as libril.so.
 * With stubs there, requests went to the URC channel, AT commands failed and
 * requestRadioPower dereferenced the missing response.
 *
 * It also waits for gsm0710muxd's /dev/radio/pttycmd1, which mtk-ril.so
 * opens in RIL_Init and which may not exist yet when rild starts.
 *
 * The rest stands in for what MediaTek's telephony framework does and CM's
 * does not:
 *  - Call progress (the modem's +ECPI) comes as MediaTek's unsolicited 3004
 *    instead of RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED; CM's RILJ drops it,
 *    never polls GET_CURRENT_CALLS and keeps a connected call "dialing". The
 *    AOSP notification is added after it.
 *  - SETUP_DATA_CALL takes an eighth string, the requested context id, which
 *    MediaTek's framework sets to "1"; with seven mtk-ril.so fails at once.
 *  - When the radio turns on, MediaTek's framework sends
 *    SET_GPRS_TRANSFER_TYPE (its gprs_transfer_setting, 1 "data preferred"
 *    by default) and SET_GPRS_CONNECT_TYPE "always"; until then the modem
 *    stays detached and CM never sets up a data call.
 *  - An incoming call is announced with unsolicited 3014 and only rings once
 *    the framework accepts it with SET_CALL_INDICATION.
 *  - Changes of the PS registration and of the SIM come as MediaTek
 *    unsolicited responses; they are passed on as the AOSP ones that make
 *    the framework poll. CM's RILJ does not know the other MediaTek ones and
 *    logs an exception for each, so they are dropped.
 * The shim sends those requests itself, with its own tokens: it answers
 * mtk-ril.so's QueryMyChannelId for them and keeps their completion from
 * reaching rild.
 */

#define LOG_TAG "mt6575-ril"

#include <dlfcn.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <cutils/log.h>
#include <telephony/ril.h>

#define MTK_RIL         "/system/lib/mtk-ril.so"
#define MTK_LIBRIL      "libril.so"
#define AT_CHANNEL_DEV  "/dev/radio/pttycmd1"
#define AT_CHANNEL_WAIT_MS 15000

/* MediaTek requests and unsolicited responses (RIL_REQUEST_MTK_BASE 2000,
 * RIL_UNSOL_MTK_BASE 3000) */
#define MTK_REQUEST_SET_GPRS_CONNECT_TYPE   2016
#define MTK_REQUEST_SET_GPRS_TRANSFER_TYPE  2017
#define MTK_REQUEST_SET_CALL_INDICATION     2036
#define MTK_UNSOL_BASE                      3000
#define MTK_UNSOL_CALL_PROGRESS_INFO        3004
#define MTK_UNSOL_SIM_INSERTED_STATUS       3007
#define MTK_UNSOL_INCOMING_CALL_INDICATION  3014
#define MTK_UNSOL_SIM_MISSING               3015
#define MTK_UNSOL_PS_NETWORK_STATE_CHANGED  3023

/* MediaTek's libril runs data requests on RIL_CMD_PROXY_4 / AT channel 4
 * and call requests on RIL_CMD_PROXY_2 / AT channel 2. */
#define DATA_PROXY      4
#define CALL_PROXY      2

#define AOSP_SETUP_DATA_CALL_ARGS 7

struct mtk_ril_env {
    void (*OnRequestComplete)(RIL_Token t, RIL_Errno e, void *response,
            size_t responselen);
    void (*OnUnsolicitedResponse)(int unsolResponse, const void *data,
            size_t datalen);
    void (*RequestTimedCallback)(RIL_TimedCallback callback, void *param,
            const struct timeval *relativeTime);
    void (*RequestProxyTimedCallback)(RIL_TimedCallback callback, void *param,
            const struct timeval *relativeTime, int proxyId);
    int (*QueryMyChannelId)(RIL_Token t);
    int (*QueryMyProxyIdByThread)(void);
};

/* mtk-ril.so's onStateRequest takes the SIM (RILId, 0 for SIM 1) and,
 * unless NULL, a pointer it stores the SIM state in. */
typedef RIL_RadioState (*mtk_state_request_t)(int rid, int *sim_state);

typedef const RIL_RadioFunctions *(*ril_init_t)(const struct mtk_ril_env *env,
        int argc, char **argv);

/* A request the shim sends on its own; its address is the RIL_Token. */
struct shim_request {
    const char *name;
    int request;
    int channel;
    int data[3];
    size_t datalen;
};

static struct shim_request s_gprs_transfer = {
    "SET_GPRS_TRANSFER_TYPE", MTK_REQUEST_SET_GPRS_TRANSFER_TYPE, DATA_PROXY,
    { 1 }, sizeof(int),
};
static struct shim_request s_gprs_attach = {
    "SET_GPRS_CONNECT_TYPE", MTK_REQUEST_SET_GPRS_CONNECT_TYPE, DATA_PROXY,
    { 1 }, sizeof(int),
};
static struct shim_request s_call_indication = {
    "SET_CALL_INDICATION", MTK_REQUEST_SET_CALL_INDICATION, CALL_PROXY,
    { 0 }, 3 * sizeof(int),
};

static struct mtk_ril_env s_env;
static RIL_RadioFunctions s_funcs;
static RIL_RequestFunc s_mtk_on_request;
static mtk_state_request_t s_mtk_on_state_request;
static void (*s_on_request_complete)(RIL_Token t, RIL_Errno e, void *response,
        size_t responselen);
static void (*s_on_unsolicited)(int unsolResponse, const void *data,
        size_t datalen);
static int (*s_query_channel_id)(RIL_Token t);
static int s_radio_on;

static int is_shim_token(RIL_Token t)
{
    return t == (RIL_Token) &s_gprs_transfer ||
            t == (RIL_Token) &s_gprs_attach ||
            t == (RIL_Token) &s_call_indication;
}

static void run_shim_request(void *param)
{
    struct shim_request *r = param;

    s_mtk_on_request(r->request, r->data, r->datalen, (RIL_Token) r);
}

static void send_shim_request(struct shim_request *r)
{
    static const struct timeval now = { 0, 0 };

    ALOGI("> %s %d %d %d", r->name, r->data[0], r->data[1], r->data[2]);
    s_env.RequestProxyTimedCallback(run_shim_request, r, &now, r->channel);
}

static int query_channel_id(RIL_Token t)
{
    if (is_shim_token(t))
        return ((struct shim_request *) t)->channel;
    return s_query_channel_id(t);
}

static void on_request_complete(RIL_Token t, RIL_Errno e, void *response,
        size_t responselen)
{
    if (is_shim_token(t)) {
        ALOGI("< %s %d", ((struct shim_request *) t)->name, e);
        return;
    }
    s_on_request_complete(t, e, response, responselen);
}

static void on_radio_state_changed(void)
{
    RIL_RadioState state = s_mtk_on_state_request(0, NULL);
    int on = state != RADIO_STATE_OFF && state != RADIO_STATE_UNAVAILABLE;

    if (on && !s_radio_on) {
        send_shim_request(&s_gprs_transfer);
        send_shim_request(&s_gprs_attach);
    }
    s_radio_on = on;
}

static void accept_incoming_call(const void *data, size_t datalen)
{
    char **info = (char **) data;

    /* callId, number, type, call mode, sequence number */
    if (datalen < 5 * sizeof(char *) || info[0] == NULL || info[4] == NULL) {
        ALOGE("unexpected incoming call indication");
        return;
    }
    s_call_indication.data[0] = 0;      /* accept */
    s_call_indication.data[1] = atoi(info[0]);
    s_call_indication.data[2] = atoi(info[4]);
    send_shim_request(&s_call_indication);
}

static void on_unsolicited_response(int unsolResponse, const void *data,
        size_t datalen)
{
    switch (unsolResponse) {
    case MTK_UNSOL_INCOMING_CALL_INDICATION:
        accept_incoming_call(data, datalen);
        return;
    case MTK_UNSOL_CALL_PROGRESS_INFO:
        s_on_unsolicited(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED, NULL, 0);
        return;
    case MTK_UNSOL_PS_NETWORK_STATE_CHANGED:
        s_on_unsolicited(RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
                NULL, 0);
        return;
    case MTK_UNSOL_SIM_INSERTED_STATUS:
    case MTK_UNSOL_SIM_MISSING:
        s_on_unsolicited(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED, NULL, 0);
        return;
    case RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED:
        s_on_unsolicited(unsolResponse, data, datalen);
        on_radio_state_changed();
        return;
    default:
        if (unsolResponse < MTK_UNSOL_BASE)
            s_on_unsolicited(unsolResponse, data, datalen);
    }
}

static void on_request(int request, void *data, size_t datalen, RIL_Token t)
{
    if (request == RIL_REQUEST_SETUP_DATA_CALL &&
            datalen == AOSP_SETUP_DATA_CALL_ARGS * sizeof(char *)) {
        char *args[AOSP_SETUP_DATA_CALL_ARGS + 1];

        memcpy(args, data, datalen);
        args[AOSP_SETUP_DATA_CALL_ARGS] = "1";
        s_mtk_on_request(request, args, sizeof(args), t);
        return;
    }
    s_mtk_on_request(request, data, datalen, t);
}

static void wait_for_at_channel(void)
{
    int waited = 0;

    while (access(AT_CHANNEL_DEV, F_OK) != 0 && waited < AT_CHANNEL_WAIT_MS) {
        usleep(200 * 1000);
        waited += 200;
    }
    if (waited)
        ALOGI("waited %d ms for %s", waited, AT_CHANNEL_DEV);
    if (access(AT_CHANNEL_DEV, F_OK) != 0)
        ALOGE("%s still missing, starting anyway", AT_CHANNEL_DEV);
}

const RIL_RadioFunctions *RIL_Init(const struct RIL_Env *env, int argc,
        char **argv)
{
    void *libril, *mtk;
    ril_init_t mtk_init;
    const RIL_RadioFunctions *funcs;

    libril = dlopen(MTK_LIBRIL, RTLD_NOW);
    mtk = dlopen(MTK_RIL, RTLD_NOW);
    if (libril == NULL || mtk == NULL) {
        ALOGE("dlopen: %s", dlerror());
        return NULL;
    }

    s_on_request_complete = env->OnRequestComplete;
    s_env.OnRequestComplete = on_request_complete;
    s_on_unsolicited = env->OnUnsolicitedResponse;
    s_env.OnUnsolicitedResponse = on_unsolicited_response;
    s_env.RequestTimedCallback = env->RequestTimedCallback;
    s_env.RequestProxyTimedCallback =
            dlsym(libril, "RIL_requestProxyTimedCallback");
    s_query_channel_id = dlsym(libril, "RIL_queryMyChannelId");
    s_env.QueryMyChannelId = query_channel_id;
    s_env.QueryMyProxyIdByThread = dlsym(libril, "RIL_queryMyProxyIdByThread");
    mtk_init = (ril_init_t) dlsym(mtk, "RIL_Init");

    if (s_env.RequestProxyTimedCallback == NULL ||
            s_query_channel_id == NULL ||
            s_env.QueryMyProxyIdByThread == NULL || mtk_init == NULL) {
        ALOGE("%s is not MediaTek's libril, or %s has no RIL_Init",
                MTK_LIBRIL, MTK_RIL);
        return NULL;
    }

    wait_for_at_channel();
    funcs = mtk_init(&s_env, argc, argv);
    if (funcs == NULL)
        return NULL;

    s_funcs = *funcs;
    s_mtk_on_request = funcs->onRequest;
    s_mtk_on_state_request = (mtk_state_request_t) funcs->onStateRequest;
    s_funcs.onRequest = on_request;
    return &s_funcs;
}
