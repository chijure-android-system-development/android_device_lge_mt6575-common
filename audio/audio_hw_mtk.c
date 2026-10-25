/*
 * audio.primary.mt6575: hands the audio HAL module over to the stock LG/MTK
 * legacy HAL (/system/lib/libaudio.primary.default.so).
 *
 * That blob's hw_module_t.id is "libaudio". CM's hw_get_module_by_class()
 * wants "audio" and refuses it (-EINVAL), so AudioFlinger had no primary
 * device at all, input or output. This module carries the right id and
 * opens the device through the blob's own methods, so libhardware stays
 * untouched (same approach as the Y220 CM10.1 port).
 */

#define LOG_TAG "audio_hw_mtk"

#include <dlfcn.h>
#include <errno.h>
#include <stddef.h>
#include <cutils/log.h>
#include <hardware/audio.h>
#include <hardware/hardware.h>

#define MTK_AUDIO_HAL "libaudio.primary.default.so"

static struct hw_module_t *mtk_module;

static int load_mtk_module(void)
{
    void *handle;

    if (mtk_module != NULL)
        return 0;

    handle = dlopen(MTK_AUDIO_HAL, RTLD_NOW);
    if (handle == NULL) {
        ALOGE("dlopen %s: %s", MTK_AUDIO_HAL, dlerror());
        return -EINVAL;
    }

    mtk_module = dlsym(handle, HAL_MODULE_INFO_SYM_AS_STR);
    if (mtk_module == NULL) {
        ALOGE("%s has no %s", MTK_AUDIO_HAL, HAL_MODULE_INFO_SYM_AS_STR);
        dlclose(handle);
        return -EINVAL;
    }
    mtk_module->dso = handle;

    ALOGI("loaded %s (%s, id %s)", MTK_AUDIO_HAL, mtk_module->name,
          mtk_module->id);
    return 0;
}

static int adev_open(const hw_module_t *module, const char *name,
                     hw_device_t **device)
{
    int ret;

    (void)module;

    ret = load_mtk_module();
    if (ret != 0)
        return ret;

    return mtk_module->methods->open(mtk_module, name, device);
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "MT6575 audio HAL wrapper",
        .author = "CyanogenMod mt6575-common",
        .methods = &hal_module_methods,
    },
};
