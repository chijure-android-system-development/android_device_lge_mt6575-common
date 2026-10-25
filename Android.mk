LOCAL_PATH := $(call my-dir)

ifneq ($(filter vee4ss vee5ss,$(TARGET_DEVICE)),)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
