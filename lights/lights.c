/*
 * lights.c — Lights HAL for LG E440g / E450g (LP5521 RGB LED controller)
 *
 * Replicates the stock LG/MTK lights.default.so ("MTK lights Module") of
 * firmwares E44010a and E450g04a, read back from their disassembly (the
 * E450g's lacks mode 4, built here only with LP5521_HAS_CURRENT_INDEX):
 *
 *  - backlight: lcd-backlight gets the luminance of the color,
 *    (77 * R + 150 * G + 29 * B) >> 8.
 *  - buttons: button-backlight is 255 for any non-black color, else 0.
 *  - lg_notifications: LG's flashMode switch on the LP5521:
 *      0  off: led_pattern and R/G/B brightness to 0
 *      1  blink: led_blink gets "0x06<rgb hex> <onMS> <offMS>"
 *      2  solid: R/G/B brightness
 *      3  led_pattern gets the pattern id in the color's low byte
 *      4  led_current_index gets the index in the color's low byte
 *  - notifications: stock only pokes LG's proxy HAL there. CM's framework
 *    drives this one, so its AOSP semantics are mapped onto the cases above
 *    (black = off, LIGHT_FLASH_TIMED = blink, any other lit color = solid).
 *  - battery: the stock HAL drives /sys/class/leds/{red,green,blue}, which
 *    this phone lacks. The stock charging LED comes from LG's
 *    EmotionalLedService (LGSystemServer.apk): while plugged it starts the
 *    LP5521 SRAM pattern 3 (charging) or 4 (level 100) through
 *    lg_notifications, only with the screen off, and nothing when unplugged.
 *    CM's BatteryService sets a solid color while plugged (green from 90%)
 *    and flashes for low battery, so: solid green at 100% -> 4, other
 *    solid -> 3, flashing -> nothing. A notification takes precedence.
 *  - attention: stock only drives a jogball LED, absent here.
 *  - boot and shutdown, also from EmotionalLedService: the bootloader's
 *    power-on pattern keeps running until boot completes, and shutdown or
 *    reboot shows pattern 6. init.mt6575.rc does both; the HAL leaves the
 *    LED alone until boot has completed and once shutdown is requested.
 */
#define LOG_TAG "lights"

#include <cutils/log.h>
#include <cutils/properties.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <hardware/lights.h>

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

#define LCD_FILE    "/sys/class/leds/lcd-backlight/brightness"
#define BUTTON_FILE "/sys/class/leds/button-backlight/brightness"

#define LP5521_DIR          "/sys/bus/i2c/drivers/lp5521/1-0032"
#define LP5521_RED_FILE     LP5521_DIR "/leds/R/brightness"
#define LP5521_GREEN_FILE   LP5521_DIR "/leds/G/brightness"
#define LP5521_BLUE_FILE    LP5521_DIR "/leds/B/brightness"
#define LP5521_LED_BLINK    LP5521_DIR "/led_blink"
#define LP5521_LED_PATTERN  LP5521_DIR "/led_pattern"
#ifdef LP5521_HAS_CURRENT_INDEX
#define LP5521_LED_CURRENT_INDEX "/sys/class/leds/R/device/led_current_index"
#endif

#define BATTERY_CAPACITY_FILE "/sys/class/power_supply/battery/capacity"

/* LG's flashMode values for lg_notifications */
#define LG_FLASH_OFF            0
#define LG_FLASH_BLINK          1
#define LG_FLASH_SOLID          2
#define LG_FLASH_PATTERN        3
#define LG_FLASH_CURRENT_INDEX  4

/* LP5521 SRAM patterns started by LG's EmotionalLedService */
#define LG_PATTERN_CHARGING       3
#define LG_PATTERN_CHARGING_FULL  4

struct led_request {
    int color;
    int flashMode;
    int onMS;
    int offMS;
};

static struct led_request g_notification;
static struct led_request g_battery;
static int g_screen_on = 1;

static int write_int(const char *path, int value)
{
    char buf[52];
    int fd, len;
    ssize_t n;

    fd = open(path, O_RDWR);
    if (fd < 0)
        return -errno;

    len = sprintf(buf, "%d\n", value);
    n = write(fd, buf, len);
    close(fd);
    return (n == -1) ? -errno : 0;
}

static int write_blink(int color, int onMS, int offMS)
{
    char buf[52];
    int fd, len;
    ssize_t n;

    fd = open(LP5521_LED_BLINK, O_RDWR);
    if (fd < 0) {
        ALOGE("write_int failed to open %s", LP5521_LED_BLINK);
        return -errno;
    }

    len = sprintf(buf, "0x06%x %d %d\n", color & 0x00ffffff, onMS, offMS);
    n = write(fd, buf, len);
    close(fd);
    return (n == -1) ? -errno : 0;
}

static int read_int(const char *path)
{
    char buf[16];
    int fd;
    ssize_t n;

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0)
        return -1;

    buf[n] = '\0';
    return atoi(buf);
}

static int rgb_brightness(int r, int g, int b)
{
    return (77 * r + 150 * g + 29 * b) >> 8;
}

static int set_lg_led_locked(int flashMode, int color, int onMS, int offMS);
static void update_rgb_locked(void);

static int set_light_backlight(struct light_device_t *dev,
        const struct light_state_t *state)
{
    int color = state->color & 0x00ffffff;
    int brightness = rgb_brightness((color >> 16) & 0xff,
            (color >> 8) & 0xff, color & 0xff);
    int ret;

    pthread_mutex_lock(&g_lock);
    ret = write_int(LCD_FILE, brightness);
    if (g_screen_on != (brightness > 0)) {
        g_screen_on = brightness > 0;
        update_rgb_locked();
    }
    pthread_mutex_unlock(&g_lock);
    return ret;
}

static int set_light_buttons(struct light_device_t *dev,
        const struct light_state_t *state)
{
    int on = (state->color & 0x00ffffff) != 0;
    int ret;

    pthread_mutex_lock(&g_lock);
    ret = write_int(BUTTON_FILE, on ? 255 : 0);
    pthread_mutex_unlock(&g_lock);
    return ret;
}

/* The stock lg_notifications switch; called with g_lock held. */
static int set_lg_led_locked(int flashMode, int color, int onMS, int offMS)
{
    int r = (color >> 16) & 0xff;
    int g = (color >> 8) & 0xff;
    int b = color & 0xff;

    ALOGD("flashMode = %d, colorARGB = %08x, onMS = %d, offMS = %d",
            flashMode, color, onMS, offMS);

    switch (flashMode) {
    case LG_FLASH_OFF:
        write_int(LP5521_LED_PATTERN, 0);
        write_int(LP5521_RED_FILE, 0);
        write_int(LP5521_GREEN_FILE, 0);
        return write_int(LP5521_BLUE_FILE, 0);
    case LG_FLASH_BLINK:
        return write_blink(color, onMS, offMS);
    case LG_FLASH_SOLID:
        write_int(LP5521_RED_FILE, r);
        write_int(LP5521_GREEN_FILE, g);
        return write_int(LP5521_BLUE_FILE, b);
    case LG_FLASH_PATTERN:
        ALOGD("[pattern_id] = %d", b);
        return write_int(LP5521_LED_PATTERN, b);
#ifdef LP5521_HAS_CURRENT_INDEX
    case LG_FLASH_CURRENT_INDEX:
        ALOGD("[current_index] = %d", b);
        return write_int(LP5521_LED_CURRENT_INDEX, b);
#endif
    default:
        return -EINVAL;
    }
}

static int set_light_lg_notifications(struct light_device_t *dev,
        const struct light_state_t *state)
{
    int ret;

    pthread_mutex_lock(&g_lock);
    ret = set_lg_led_locked(state->flashMode, state->color,
            state->flashOnMS, state->flashOffMS);
    pthread_mutex_unlock(&g_lock);
    return ret;
}

static int is_lit(const struct led_request *req)
{
    return (req->color & 0x00ffffff) != 0;
}

static int is_blinking(const struct led_request *req)
{
    return req->flashMode == LIGHT_FLASH_TIMED &&
            req->onMS > 0 && req->offMS > 0;
}

/* Whether init's boot or shutdown handling owns the LED right now. */
static int led_owned_by_init(void)
{
    char value[PROPERTY_VALUE_MAX];

    property_get("sys.shutdown.requested", value, "");
    if (value[0] != '\0')
        return 1;
    property_get("sys.boot_completed", value, "0");
    return strcmp(value, "1") != 0;
}

/* Picks what the LED shows; called with g_lock held. */
static void update_rgb_locked(void)
{
    /* Unknown at first: init changes the LED behind the HAL's back at
     * boot, so the first request must reach the chip even if it is
     * "off". */
    static struct led_request shown = { -1, -1, -1, -1 };
    struct led_request next = { 0, LG_FLASH_OFF, 0, 0 };

    if (led_owned_by_init())
        return;

    if (is_lit(&g_notification)) {
        next = g_notification;
        if (is_blinking(&g_notification)) {
            next.flashMode = LG_FLASH_BLINK;
        } else {
            next.flashMode = LG_FLASH_SOLID;
            next.onMS = next.offMS = 0;
        }
    } else if (!g_screen_on && is_lit(&g_battery) && !is_blinking(&g_battery)) {
        int r = (g_battery.color >> 16) & 0xff;
        int g = (g_battery.color >> 8) & 0xff;

        /* CM turns green from 90%; LG shows "full" only at 100%. */
        next.flashMode = LG_FLASH_PATTERN;
        next.color = (g > r && read_int(BATTERY_CAPACITY_FILE) >= 100) ?
                LG_PATTERN_CHARGING_FULL : LG_PATTERN_CHARGING;
    }

    if (!memcmp(&next, &shown, sizeof(next)))
        return;

    /* A running SRAM pattern keeps going until led_pattern is reset. */
    if (shown.flashMode == LG_FLASH_PATTERN && next.flashMode != LG_FLASH_OFF)
        set_lg_led_locked(LG_FLASH_OFF, 0, 0, 0);

    set_lg_led_locked(next.flashMode, next.color, next.onMS, next.offMS);
    shown = next;
}

static void set_request_locked(struct led_request *req,
        const struct light_state_t *state)
{
    req->color = state->color;
    req->flashMode = state->flashMode;
    req->onMS = state->flashOnMS;
    req->offMS = state->flashOffMS;
    update_rgb_locked();
}

static int set_light_notifications(struct light_device_t *dev,
        const struct light_state_t *state)
{
    pthread_mutex_lock(&g_lock);
    set_request_locked(&g_notification, state);
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int set_light_battery(struct light_device_t *dev,
        const struct light_state_t *state)
{
    pthread_mutex_lock(&g_lock);
    set_request_locked(&g_battery, state);
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int set_light_noop(struct light_device_t *dev,
        const struct light_state_t *state)
{
    return 0;
}

static int close_lights(struct light_device_t *dev)
{
    free(dev);
    return 0;
}

static int open_lights(const struct hw_module_t *module, const char *name,
        struct hw_device_t **device)
{
    int (*fn)(struct light_device_t *, const struct light_state_t *);
    struct light_device_t *dev;

    if      (!strcmp(name, LIGHT_ID_BACKLIGHT))     fn = set_light_backlight;
    else if (!strcmp(name, LIGHT_ID_BUTTONS))       fn = set_light_buttons;
    else if (!strcmp(name, LIGHT_ID_NOTIFICATIONS)) fn = set_light_notifications;
    else if (!strcmp(name, "lg_notifications"))     fn = set_light_lg_notifications;
    else if (!strcmp(name, LIGHT_ID_BATTERY))       fn = set_light_battery;
    else if (!strcmp(name, LIGHT_ID_ATTENTION))     fn = set_light_noop;
    else if (!strcmp(name, "lg_attention"))         fn = set_light_noop;
    else return -EINVAL;

    dev = calloc(1, sizeof(*dev));
    if (!dev)
        return -ENOMEM;

    dev->common.tag     = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module  = (struct hw_module_t *)module;
    dev->common.close   = (int (*)(struct hw_device_t *))close_lights;
    dev->set_light      = fn;

    *device = (struct hw_device_t *)dev;
    return 0;
}

static struct hw_module_methods_t lights_methods = {
    .open = open_lights,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag           = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id            = LIGHTS_HARDWARE_MODULE_ID,
    .name          = "MT6575 LP5521 lights HAL",
    .author        = "CM10 mt6575-common",
    .methods       = &lights_methods,
};
