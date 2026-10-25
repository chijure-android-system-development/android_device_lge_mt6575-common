/*
 * MTK cutils/xlog.h for the BlueAngel Bluetooth sources (frameworks/base
 * core/jni, BOARD_HAVE_BLUETOOTH_MTK). XLOG* map to the plain ALOG* macros;
 * the binary __xlog_buf_printf the blobs need comes from libmtk_symbols.
 */
#ifndef CUTILS_XLOG_H
#define CUTILS_XLOG_H

#include <cutils/log.h>

#ifndef XLOGV
#define XLOGV ALOGV
#endif
#ifndef XLOGD
#define XLOGD ALOGD
#endif
#ifndef XLOGI
#define XLOGI ALOGI
#endif
#ifndef XLOGW
#define XLOGW ALOGW
#endif
#ifndef XLOGE
#define XLOGE ALOGE
#endif

#endif
