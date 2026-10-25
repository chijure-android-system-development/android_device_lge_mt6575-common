# Symbols that the stock LG/MTK blobs expect from MTK's own system
# libraries (liblog, libstagefright). init.mt6575.rc puts this library in
# LD_PRELOAD; the 4.1 linker searches preloads for every library's
# undefined symbols, so CM's libraries stay untouched.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE         := libmtk_symbols
LOCAL_SRC_FILES      := xlog.c awesomeplayer.c
LOCAL_SHARED_LIBRARIES := liblog

include $(BUILD_SHARED_LIBRARY)
