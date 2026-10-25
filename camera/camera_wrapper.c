/*
 * LG L4 II / L5 II (MT6575) camera HAL wrapper.
 *
 * The stock MTK HAL reports a phantom second/front camera.  This wrapper keeps
 * the fix device-local by delegating to the stock blob and exposing only the
 * single physical rear camera.
 *
 * The stock HAL also never sets video-frame-format, and CM's CameraSource
 * strcmp()s it unchecked, killing mediaserver when recording starts.  The
 * parameters are returned with video-frame-format=yuv420p added: recording
 * frames are planar (HAL_PIXEL_FORMAT_I420), unlike the semi-planar
 * preview, and declaring them yuv420sp scrambled the video's colors.
 */

#define LOG_TAG "mt6575-camera"

#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <cutils/log.h>
#include <hardware/camera.h>
#include <hardware/hardware.h>

static const char kStockCameraHal[] = "/system/lib/hw/camera.stock.mt6575.so";

static void *g_stock_handle;
static camera_module_t *g_stock_module;

static int load_stock_camera_hal(void)
{
    if (g_stock_module)
        return 0;

    g_stock_handle = dlopen(kStockCameraHal, RTLD_NOW);
    if (!g_stock_handle) {
        ALOGE("failed to load %s: %s", kStockCameraHal, dlerror());
        return -EINVAL;
    }

    g_stock_module = (camera_module_t *)dlsym(g_stock_handle, HAL_MODULE_INFO_SYM_AS_STR);
    if (!g_stock_module) {
        ALOGE("failed to find %s in %s: %s", HAL_MODULE_INFO_SYM_AS_STR,
                kStockCameraHal, dlerror());
        dlclose(g_stock_handle);
        g_stock_handle = NULL;
        return -EINVAL;
    }

    return 0;
}

static int mt6575_get_number_of_cameras(void)
{
    int count;

    if (load_stock_camera_hal())
        return 0;

    count = g_stock_module->get_number_of_cameras();
    if (count > 1)
        ALOGW("stock HAL reports %d cameras; exposing only rear camera", count);

    return count > 0 ? 1 : 0;
}

static int mt6575_get_camera_info(int camera_id, struct camera_info *info)
{
    if (load_stock_camera_hal())
        return -EINVAL;

    if (camera_id != 0)
        return -EINVAL;

    return g_stock_module->get_camera_info(0, info);
}

static const char kVideoFrameFormat[] = "video-frame-format=";
static const char kVideoFrameFormatParam[] = ";video-frame-format=yuv420p";

static camera_device_ops_t *g_stock_ops;
static camera_device_ops_t g_ops;

/* Always hands out a wrapper-owned copy, released by mt6575_put_parameters. */
static char *mt6575_get_parameters(struct camera_device *dev)
{
    char *stock = g_stock_ops->get_parameters(dev);
    char *params;
    size_t len;

    if (!stock)
        return NULL;

    len = strlen(stock);
    params = malloc(len + sizeof(kVideoFrameFormatParam));
    if (params) {
        memcpy(params, stock, len + 1);
        if (!strstr(stock, kVideoFrameFormat))
            strcat(params, kVideoFrameFormatParam);
    } else {
        ALOGE("out of memory copying camera parameters");
    }

    g_stock_ops->put_parameters(dev, stock);
    return params;
}

static void mt6575_put_parameters(struct camera_device *dev, char *params)
{
    (void)dev;
    free(params);
}

static int mt6575_camera_open(const hw_module_t *module, const char *id,
        hw_device_t **device)
{
    camera_device_t *camera;
    int ret;

    (void)module;

    if (load_stock_camera_hal())
        return -EINVAL;

    if (strcmp(id, "0"))
        return -EINVAL;

    ret = g_stock_module->common.methods->open(&g_stock_module->common, id, device);
    if (ret)
        return ret;

    camera = (camera_device_t *)*device;
    g_stock_ops = camera->ops;
    g_ops = *g_stock_ops;
    g_ops.get_parameters = mt6575_get_parameters;
    g_ops.put_parameters = mt6575_put_parameters;
    camera->ops = &g_ops;

    return 0;
}

static hw_module_methods_t g_methods = {
    .open = mt6575_camera_open,
};

camera_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = CAMERA_HARDWARE_MODULE_ID,
        .name = "MT6575 camera wrapper",
        .author = "lge/mtk",
        .methods = &g_methods,
    },
    .get_number_of_cameras = mt6575_get_number_of_cameras,
    .get_camera_info = mt6575_get_camera_info,
};
