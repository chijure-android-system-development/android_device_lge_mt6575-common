LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE        := nvram_audio_init
LOCAL_MODULE_TAGS   := optional
LOCAL_SRC_FILES     := nvram_audio_init.c
LOCAL_SHARED_LIBRARIES := libdl libc
LOCAL_MODULE_CLASS  := EXECUTABLES
include $(BUILD_EXECUTABLE)
