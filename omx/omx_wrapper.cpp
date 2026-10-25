/*
 * LG L4 II / L5 II (MT6575) OMX plugin wrapper.
 *
 * MediaTek's libstagefrighthw (installed as libstagefrighthw_mtk.so) is
 * loaded unchanged. Its video decoders cannot render into CM's native
 * window buffers: they report an OMX color format that the PowerVR
 * gralloc rejects, and they fail to map gralloc buffers into the M4U.
 * MediaTek's own OMXCodec handles both; CM's never hands a native window
 * to components named "OMX.google.*" and copies their output to the
 * screen instead (SoftwareRenderer, which takes I420).
 *
 * So the decoders are also published as "OMX.google.MTK.VIDEO.DECODER.*"
 * (the names media_codecs.xml uses); those still decode in hardware but
 * get ordinary output buffers. Everything else passes through.
 *
 * The AVC decoder also returns every frame with timestamp 0; MediaTek's
 * OMXCodec puts the input timestamps back, CM's does not. AwesomePlayer
 * then took each frame for seconds late, kept skipping ahead and the video
 * stood still. So the wrapper remembers the timestamps of the frames that
 * go in and gives each frame that comes out without one the earliest still
 * pending: the decoder outputs in presentation order, one frame per input
 * frame. A flush (seek) drops the pending ones.
 */

#define LOG_TAG "mt6575-omx"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#include <utils/KeyedVector.h>
#include <utils/SortedVector.h>
#include <utils/threads.h>

#include <cutils/log.h>
#include <media/hardware/OMXPluginBase.h>

namespace android {

static const char kStockPlugin[] = "/system/lib/libstagefrighthw_mtk.so";
static const char kAliasPrefix[] = "OMX.google.";
static const char kVideoDecoderPrefix[] = "OMX.MTK.VIDEO.DECODER.";

static bool isVideoDecoder(const char *name)
{
    return !strncmp(name, kVideoDecoderPrefix,
            sizeof(kVideoDecoderPrefix) - 1);
}

/* "OMX.google.OMX.MTK..." would read badly; the alias drops the stock
 * name's "OMX." */
static const char *stockName(const char *name, char *buf, size_t size)
{
    if (strncmp(name, kAliasPrefix, sizeof(kAliasPrefix) - 1))
        return name;
    snprintf(buf, size, "OMX.%s", name + sizeof(kAliasPrefix) - 1);
    return isVideoDecoder(buf) ? buf : name;
}

static const OMX_U32 kInputPort = 0;

/* Stands between an aliased decoder and OMXCodec; the decoder gets it as
 * its app data. */
struct DecoderHooks {
    OMX_CALLBACKTYPE callbacks;     /* the ones the decoder calls */
    const OMX_CALLBACKTYPE *client; /* OMXCodec's */
    OMX_PTR clientData;
    OMX_ERRORTYPE (*emptyThisBuffer)(OMX_HANDLETYPE component,
            OMX_BUFFERHEADERTYPE *buffer);
    Mutex lock;
    SortedVector<OMX_TICKS> pending;    /* timestamps of decoding frames */
};

static Mutex sHooksLock;
static KeyedVector<OMX_HANDLETYPE, DecoderHooks *> sHooks;

static DecoderHooks *hooksOf(OMX_HANDLETYPE component)
{
    Mutex::Autolock lock(sHooksLock);
    ssize_t i = sHooks.indexOfKey(component);

    return i >= 0 ? sHooks.valueAt(i) : NULL;
}

static OMX_ERRORTYPE emptyThisBuffer(OMX_HANDLETYPE component,
        OMX_BUFFERHEADERTYPE *buffer)
{
    DecoderHooks *hooks = hooksOf(component);

    /* With wants-NAL-fragments the slices of a frame share its timestamp;
     * the set keeps one. */
    if (buffer->nFilledLen > 0 &&
            !(buffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)) {
        Mutex::Autolock lock(hooks->lock);
        hooks->pending.add(buffer->nTimeStamp);
    }
    return hooks->emptyThisBuffer(component, buffer);
}

static OMX_ERRORTYPE onEvent(OMX_HANDLETYPE component, OMX_PTR appData,
        OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventData)
{
    DecoderHooks *hooks = (DecoderHooks *)appData;

    if (event == OMX_EventCmdComplete && data1 == OMX_CommandFlush &&
            (data2 == kInputPort || data2 == OMX_ALL)) {
        Mutex::Autolock lock(hooks->lock);
        hooks->pending.clear();
    }
    return hooks->client->EventHandler(component, hooks->clientData, event,
            data1, data2, eventData);
}

static OMX_ERRORTYPE onEmptyBufferDone(OMX_HANDLETYPE component,
        OMX_PTR appData, OMX_BUFFERHEADERTYPE *buffer)
{
    DecoderHooks *hooks = (DecoderHooks *)appData;

    return hooks->client->EmptyBufferDone(component, hooks->clientData,
            buffer);
}

static OMX_ERRORTYPE onFillBufferDone(OMX_HANDLETYPE component,
        OMX_PTR appData, OMX_BUFFERHEADERTYPE *buffer)
{
    DecoderHooks *hooks = (DecoderHooks *)appData;

    if (buffer->nFilledLen > 0) {
        Mutex::Autolock lock(hooks->lock);

        if (!hooks->pending.isEmpty()) {
            if (buffer->nTimeStamp == 0)
                buffer->nTimeStamp = hooks->pending[0];
            hooks->pending.removeAt(0);
        }
    }
    return hooks->client->FillBufferDone(component, hooks->clientData,
            buffer);
}

struct MT6575OMXPlugin : public OMXPluginBase {
    MT6575OMXPlugin(void *handle, OMXPluginBase *stock)
        : mHandle(handle), mStock(stock), mStockCount(0), mDecoders(0) {
        char name[128];

        /* Stock components first, then one alias per video decoder. */
        while (mStock->enumerateComponents(name, sizeof(name), mStockCount)
                == OMX_ErrorNone) {
            if (isVideoDecoder(name) && mDecoders < kMaxDecoders)
                mDecoderIndex[mDecoders++] = mStockCount;
            mStockCount++;
        }
    }

    virtual ~MT6575OMXPlugin() {
        delete mStock;
        dlclose(mHandle);
    }

    virtual OMX_ERRORTYPE makeComponentInstance(const char *name,
            const OMX_CALLBACKTYPE *callbacks, OMX_PTR appData,
            OMX_COMPONENTTYPE **component) {
        char buf[128];
        const char *stock = stockName(name, buf, sizeof(buf));
        DecoderHooks *hooks;
        OMX_ERRORTYPE err;

        if (stock != buf)
            return mStock->makeComponentInstance(stock, callbacks, appData,
                    component);

        hooks = new DecoderHooks;
        hooks->callbacks.EventHandler = onEvent;
        hooks->callbacks.EmptyBufferDone = onEmptyBufferDone;
        hooks->callbacks.FillBufferDone = onFillBufferDone;
        hooks->client = callbacks;
        hooks->clientData = appData;

        err = mStock->makeComponentInstance(stock, &hooks->callbacks, hooks,
                component);
        if (err != OMX_ErrorNone) {
            delete hooks;
            return err;
        }
        hooks->emptyThisBuffer = (*component)->EmptyThisBuffer;
        (*component)->EmptyThisBuffer = emptyThisBuffer;

        Mutex::Autolock lock(sHooksLock);
        sHooks.add(*component, hooks);
        return err;
    }

    virtual OMX_ERRORTYPE destroyComponentInstance(
            OMX_COMPONENTTYPE *component) {
        OMX_ERRORTYPE err = mStock->destroyComponentInstance(component);
        Mutex::Autolock lock(sHooksLock);
        ssize_t i = sHooks.indexOfKey(component);

        if (i >= 0) {
            delete sHooks.valueAt(i);
            sHooks.removeItemsAt(i);
        }
        return err;
    }

    virtual OMX_ERRORTYPE enumerateComponents(OMX_STRING name, size_t size,
            OMX_U32 index) {
        char stock[128];
        OMX_ERRORTYPE err;

        if (index < mStockCount)
            return mStock->enumerateComponents(name, size, index);
        index -= mStockCount;
        if (index >= mDecoders)
            return OMX_ErrorNoMore;

        err = mStock->enumerateComponents(stock, sizeof(stock),
                mDecoderIndex[index]);
        if (err == OMX_ErrorNone)
            snprintf(name, size, "%s%s", kAliasPrefix, stock + 4);
        return err;
    }

    virtual OMX_ERRORTYPE getRolesOfComponent(const char *name,
            Vector<String8> *roles) {
        char buf[128];

        return mStock->getRolesOfComponent(
                stockName(name, buf, sizeof(buf)), roles);
    }

private:
    static const OMX_U32 kMaxDecoders = 16;

    void *mHandle;
    OMXPluginBase *mStock;
    OMX_U32 mStockCount;
    OMX_U32 mDecoders;
    OMX_U32 mDecoderIndex[kMaxDecoders];
};

OMXPluginBase *createMT6575OMXPlugin()
{
    typedef OMXPluginBase *(*CreateFunc)();
    void *handle = dlopen(kStockPlugin, RTLD_NOW);
    CreateFunc create;

    if (!handle) {
        ALOGE("failed to load %s: %s", kStockPlugin, dlerror());
        return NULL;
    }

    /* MediaTek exports it as android::createOMXPlugin(), not extern "C". */
    create = (CreateFunc)dlsym(handle, "createOMXPlugin");
    if (!create)
        create = (CreateFunc)dlsym(handle, "_ZN7android15createOMXPluginEv");
    if (!create) {
        ALOGE("no createOMXPlugin in %s", kStockPlugin);
        dlclose(handle);
        return NULL;
    }

    return new MT6575OMXPlugin(handle, create());
}

}  // namespace android
