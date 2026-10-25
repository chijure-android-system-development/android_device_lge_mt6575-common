# BoardConfig shared by the MT6575 LGs (L4 II vee4ss, L5 II vee5ss).
# Each device's BoardConfig.mk includes this first and adds its kernel,
# partitions, recovery fstab and OTA assert.

# ---- SoC / CPU (MediaTek MT6575) ----
TARGET_BOARD_PLATFORM := mt6575
BOARD_VENDOR := lge

TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_VARIANT := cortex-a9
TARGET_ARCH_LOWMEM := true

TARGET_NO_BOOTLOADER := true
TARGET_NO_RADIOIMAGE := true

# ---- Boot image ----
BOARD_KERNEL_BASE     := 0x10000000
BOARD_KERNEL_PAGESIZE := 2048
BOARD_KERNEL_CMDLINE  := androidboot.hardware=mt6575
BOARD_MKBOOTIMG_ARGS  := --ramdisk_offset 0x01000000 --tags_offset 0x00000100

# mtkbootimg: adds the MediaTek headers in front of kernel and ramdisk
BOARD_CUSTOM_BOOTIMG    := true
BOARD_CUSTOM_MKBOOTIMG  := $(HOST_OUT_EXECUTABLES)/mtkbootimg$(HOST_EXECUTABLE_SUFFIX)
BOARD_CUSTOM_BOOTIMG_MK := device/lge/mt6575-common/bootimg.mk

TARGET_RELEASETOOLS_EXTENSIONS := device/lge/mt6575-common

TARGET_PROVIDES_INIT_RC := true

# ---- WiFi / Bluetooth combo (MT6620) ----
BOARD_WLAN_DEVICE           := MT6620
WPA_SUPPLICANT_VERSION      := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_mt66xx
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_mt66xx
WIFI_DRIVER_MODULE_PATH     := /system/lib/modules/wlan.ko
WIFI_DRIVER_MODULE_NAME     := wlan

BOARD_HAVE_BLUETOOTH        := true
BOARD_HAVE_BLUETOOTH_MTK    := true

# ---- Filesystem ----
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_FLASH_BLOCK_SIZE     := 131072
BOARD_HAS_LARGE_FILESYSTEM := true
BOARD_VOLD_MAX_PARTITIONS  := 30

# ---- Recovery ----
BOARD_HAS_NO_SELECT_BUTTON := true
BOARD_HAS_NO_MISC_PARTITION := true
BOARD_SUPPRESS_SECURE_ERASE := true
BOARD_SUPPRESS_EMMC_WIPE := true
BOARD_USES_MMCUTILS := true
TARGET_RECOVERY_PIXEL_FORMAT := "RGBA_8888"

# ---- Camera ----
# MT6575 uses the Camera1 HAL (v1); use LegacyCamera sources in Gallery2
BOARD_USES_LEGACY_CAMERA := true

# ---- ADB root (useful while porting) ----
ADDITIONAL_DEFAULT_PROPERTIES += ro.secure=0

# ---- Charger mode (MTK passes androidboot.mode=charger) ----
BOARD_GLOBAL_CFLAGS += -DCHARGERMODE_CMDLINE_NAME='"androidboot.mode"' \
                       -DCHARGERMODE_CMDLINE_VALUE='"charger"'
