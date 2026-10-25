LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := camera.mt6575
LOCAL_SRC_FILES := camera_wrapper.c
LOCAL_SHARED_LIBRARIES := libdl liblog
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

include $(BUILD_SHARED_LIBRARY)
