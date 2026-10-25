LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE         := libmt6575-ril
LOCAL_SRC_FILES      := mt6575_ril.c
LOCAL_CFLAGS         := -DRIL_SHLIB
LOCAL_SHARED_LIBRARIES := liblog libdl

include $(BUILD_SHARED_LIBRARY)
