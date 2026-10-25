LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE         := libstagefrighthw
LOCAL_SRC_FILES      := omx_wrapper.cpp plugin_entry.cpp
LOCAL_C_INCLUDES     := $(TOP)/frameworks/native/include/media/openmax \
                        $(TOP)/frameworks/native/include/media/hardware
LOCAL_SHARED_LIBRARIES := liblog libdl libutils

include $(BUILD_SHARED_LIBRARY)
