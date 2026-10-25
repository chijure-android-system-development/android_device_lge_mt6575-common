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
 */

#define LOG_TAG "mt6575-omx"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

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

        return mStock->makeComponentInstance(
                stockName(name, buf, sizeof(buf)), callbacks, appData,
                component);
    }

    virtual OMX_ERRORTYPE destroyComponentInstance(
            OMX_COMPONENTTYPE *component) {
        return mStock->destroyComponentInstance(component);
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
