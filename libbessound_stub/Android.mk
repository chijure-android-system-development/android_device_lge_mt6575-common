LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE        := libbessound_mtk
LOCAL_MODULE_TAGS   := optional
LOCAL_SRC_FILES     := libbessound_stub.c
LOCAL_SHARED_LIBRARIES := libc
LOCAL_MODULE_CLASS  := SHARED_LIBRARIES
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)
