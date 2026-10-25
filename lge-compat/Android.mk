LOCAL_PATH := $(call my-dir)

# Minimal stand-ins for the LG framework classes MtkBt.apk links against.
# LG's MDM (device management) is not present, so it is reported as off.
include $(CLEAR_VARS)
LOCAL_MODULE      := com.lge.compat
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES   := $(call all-java-files-under, src)
include $(BUILD_JAVA_LIBRARY)

# Stock MediaTek Bluetooth app: provides OPP/PBAP/MAP/FTP/PAN/HID/AVRCP on
# top of BlueAngel. Like the stock ROM, it replaces AOSP's Bluetooth.apk,
# whose RFCOMM server sockets BlueAngel does not implement.
include $(CLEAR_VARS)
LOCAL_MODULE             := MtkBt
LOCAL_MODULE_TAGS        := optional
LOCAL_MODULE_CLASS       := APPS
LOCAL_MODULE_SUFFIX      := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_CERTIFICATE        := PRESIGNED
LOCAL_SRC_FILES          := ../../../../vendor/lge/$(TARGET_DEVICE)/proprietary/app/MtkBt.apk
LOCAL_OVERRIDES_PACKAGES := Bluetooth
LOCAL_REQUIRED_MODULES   := com.lge.compat
include $(BUILD_PREBUILT)
