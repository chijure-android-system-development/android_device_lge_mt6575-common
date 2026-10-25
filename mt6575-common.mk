# Product configuration shared by the MT6575 LGs (L4 II vee4ss, L5 II
# vee5ss). Each device's device.mk inherits this and adds its display
# density, dalvik heap, media profiles and device-only blobs.

# Full Android base platform (includes core.mk + fonts + librs_jni + audio HALs + keyboards)
$(call inherit-product, $(SRC_TARGET_DIR)/product/generic_no_telephony.mk)

COMMON_PATH := device/lge/mt6575-common

DEVICE_PACKAGE_OVERLAYS += $(COMMON_PATH)/overlay

# Runtime libraries needed for Android boot (not in generic_no_telephony.mk)
PRODUCT_PACKAGES += \
    libkeystore_client \
    sensorservice

# ---- Init y ramdisk ----
PRODUCT_COPY_FILES += \
    $(COMMON_PATH)/rootdir/root/init.rc:root/init.rc \
    $(COMMON_PATH)/rootdir/root/init.mt6575.common.rc:root/init.mt6575.common.rc \
    $(COMMON_PATH)/rootdir/root/fstab.mt6575:root/fstab.mt6575 \
    $(COMMON_PATH)/rootdir/root/ueventd.mt6575.rc:root/ueventd.mt6575.rc \
    system/core/rootdir/init.usb.rc:root/init.usb.rc \
    system/core/rootdir/init.trace.rc:root/init.trace.rc \
    $(COMMON_PATH)/rootdir/system/etc/init.d/05serialno:system/etc/init.d/05serialno \
    $(COMMON_PATH)/rootdir/system/etc/init.d/10mtk-nodes:system/etc/init.d/10mtk-nodes \
    $(COMMON_PATH)/rootdir/system/etc/init.d/20sensors:system/etc/init.d/20sensors \
    $(COMMON_PATH)/rootdir/system/etc/vold.fstab:system/etc/vold.fstab \
    $(COMMON_PATH)/rootdir/system/etc/audio_policy.conf:system/etc/audio_policy.conf

# ---- GSM: APN list, SPN overrides and SIM Toolkit (as vendor/cm/config/gsm.mk) ----
# The APN list is CM's plus Peru's carriers (Claro and Movistar from the
# stock firmware, Entel), so it is copied here instead of gsm.mk's.
PRODUCT_COPY_FILES += \
    $(COMMON_PATH)/rootdir/system/etc/apns-conf.xml:system/etc/apns-conf.xml \
    vendor/cm/prebuilt/common/etc/spn-conf.xml:system/etc/spn-conf.xml

PRODUCT_PACKAGES += \
    Stk

# ---- Keymaps stock MTK/LG ----
PRODUCT_COPY_FILES += \
    $(COMMON_PATH)/rootdir/system/usr/keylayout/mtk-kpd.kl:system/usr/keylayout/mtk-kpd.kl \
    $(COMMON_PATH)/rootdir/system/usr/keylayout/mtk-kpd.kl:system/usr/keylayout/Vendor_2454_Product_6575.kl \
    $(COMMON_PATH)/rootdir/system/usr/keylayout/ACCDET.kl:system/usr/keylayout/ACCDET.kl \
    $(COMMON_PATH)/rootdir/system/usr/keychars/mtk-kpd.kcm:system/usr/keychars/mtk-kpd.kcm

# ---- Permisos de hardware ----
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml

# ---- Propiedades del sistema ----
PRODUCT_PROPERTY_OVERRIDES += \
    ro.opengles.version=131072 \
    ro.bluetooth.PANenable=1 \
    ro.btstack=blueangel \
    wifi.interface=wlan0 \
    wifi.tethering.interface=ap0 \
    wifi.direct.interface=p2p0 \
    wifi.supplicant_scan_interval=20 \
    ro.mediatek.wlan.wsc=1 \
    ro.mediatek.wlan.p2p=1 \
    mediatek.wlan.ctia=0 \
    mediatek.wlan.chip=MT6620 \
    mediatek.wlan.module.postfix=_mt6620 \
    wlan.chip.vendor=mtk \
    wlan.chip.version=mt6620 \
    wifi.lge.patch=true \
    wlan.lge.concurrency=SCC \
    wlan.lge.supportsimaka=YES \
    gps.solution.combo.chip=1 \
    rild.libpath=/system/lib/libmt6575-ril.so \
    telephony.lteOnGsmDevice=0 \
    ro.com.android.dataroaming=false \
    ro.config.low_ram=true \
    config.disable_atlas=true \
    persist.service.adb.enable=1 \
    ro.zram.default=25

PRODUCT_TAGS += dalvik.gc.type-precise

# ---- Paquetes ----
# Stock blobs both vendor trees provide, plus the device-tree HALs.
PRODUCT_PACKAGES += \
    libacdk \
    libagpssupl \
    libaudiocompensationfilter \
    libaudiocustparam \
    libaudiosetting \
    libbessound_mtk \
    libbluetooth_mtk \
    libbluetoothdrv \
    libbluetoothem_mtk \
    libbtcust \
    libbtcusttable \
    libbtpcm \
    libcustom_nvram \
    libextjsr82 \
    libmtk_devinfo \
    libmtk_drvb \
    libmtkbtextadpa2dp \
    libnvram \
    libnvram_daemon_callback \
    camera.mt6575 \
    Gallery2 \
    Torch \
    lights.mt6575 \
    audio.primary.mt6575 \
    libmtk_symbols \
    power.mt6575 \
    rild \
    libmt6575-ril \
    libril \
    mtk_agpsd \
    nvram_agent_binder \
    nvram_audio_init

# ---- Bluetooth: stock MtkBt.apk replaces Bluetooth.apk, as on stock ----
# com.lge.compat holds the LG framework classes MtkBt links against; its
# jar is on the BOOTCLASSPATH in init.rc.
PRODUCT_PACKAGES += \
    MtkBt \
    com.lge.compat
