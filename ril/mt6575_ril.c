/*
 * libmt6575-ril: rild.libpath shim in front of the stock mtk-ril.so, so CM's
 * rild stays unpatched.
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
 */

#define LOG_TAG "mt6575-ril"

#include <dlfcn.h>
#include <stddef.h>
#include <unistd.h>
#include <cutils/log.h>
#include <telephony/ril.h>

#define MTK_RIL         "/system/lib/mtk-ril.so"
#define MTK_LIBRIL      "libril.so"
#define AT_CHANNEL_DEV  "/dev/radio/pttycmd1"
#define AT_CHANNEL_WAIT_MS 15000

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

typedef const RIL_RadioFunctions *(*ril_init_t)(const struct mtk_ril_env *env,
        int argc, char **argv);

static struct mtk_ril_env s_env;

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

    libril = dlopen(MTK_LIBRIL, RTLD_NOW);
    mtk = dlopen(MTK_RIL, RTLD_NOW);
    if (libril == NULL || mtk == NULL) {
        ALOGE("dlopen: %s", dlerror());
        return NULL;
    }

    s_env.OnRequestComplete = env->OnRequestComplete;
    s_env.OnUnsolicitedResponse = env->OnUnsolicitedResponse;
    s_env.RequestTimedCallback = env->RequestTimedCallback;
    s_env.RequestProxyTimedCallback =
            dlsym(libril, "RIL_requestProxyTimedCallback");
    s_env.QueryMyChannelId = dlsym(libril, "RIL_queryMyChannelId");
    s_env.QueryMyProxyIdByThread = dlsym(libril, "RIL_queryMyProxyIdByThread");
    mtk_init = (ril_init_t) dlsym(mtk, "RIL_Init");

    if (s_env.RequestProxyTimedCallback == NULL ||
            s_env.QueryMyChannelId == NULL ||
            s_env.QueryMyProxyIdByThread == NULL || mtk_init == NULL) {
        ALOGE("%s is not MediaTek's libril, or %s has no RIL_Init",
                MTK_LIBRIL, MTK_RIL);
        return NULL;
    }

    wait_for_at_channel();
    return mtk_init(&s_env, argc, argv);
}
