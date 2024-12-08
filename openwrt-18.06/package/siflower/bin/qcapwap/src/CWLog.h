#ifndef __CAPWAP_CWLog_HEADER__
#define __CAPWAP_CWLog_HEADER__

#include <stdlib.h>

#define DEFAULT_LOGGING_LEVEL 3

enum {
	L_CRIT,
	L_WARNING,
	L_NOTICE,
	L_INFO,
	L_DEBUG
};

void log_message(int priority, const char *format, ...)
    __attribute__((format(printf, 2, 3)));

#define NAME	"capwap"

#define CWDebugLog(format, ...) log_message(L_INFO, format, ## __VA_ARGS__)
#define CWLog(format, ...) log_message(L_INFO, format, ## __VA_ARGS__)
#define CWNoticeLog(format, ...) log_message(L_NOTICE, format, ## __VA_ARGS__)
#define CWWarningLog(format, ...) log_message(L_WARNING, format, ## __VA_ARGS__)
#define CWCritLog(format, ...) log_message(L_CRIT, format, ## __VA_ARGS__)

#endif
