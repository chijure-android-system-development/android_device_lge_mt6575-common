LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE         := audio.primary.mt6575
LOCAL_SRC_FILES      := audio_hw_mtk.c
LOCAL_SHARED_LIBRARIES := liblog libdl
LOCAL_MODULE_PATH    := $(TARGET_OUT_SHARED_LIBRARIES)/hw

include $(BUILD_SHARED_LIBRARY)
