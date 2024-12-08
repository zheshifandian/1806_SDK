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

// Avoid compiler warnings
#define NONE	((struct capwap_wtp *)0)

// print wtp's ip address in front of the debug messages
#define CWDebugLog(wtp, format, ...) log_message(L_INFO, "%s: " format, (wtp) ? (wtp)->ip_addr : "", ## __VA_ARGS__)
#define CWLog(wtp, format, ...) log_message(L_INFO, "%s: " format, (wtp) ? (wtp)->ip_addr : "", ## __VA_ARGS__)
#define CWNoticeLog(wtp, format, ...) log_message(L_NOTICE, "%s: " format, (wtp) ? (wtp)->ip_addr : "", ## __VA_ARGS__)
#define CWWarningLog(wtp, format, ...) log_message(L_WARNING, "%s: " format, (wtp) ? (wtp)->ip_addr : "", ## __VA_ARGS__)
#define CWCritLog(wtp, format, ...) log_message(L_CRIT, "%s: " format, (wtp) ? (wtp)->ip_addr : "", ## __VA_ARGS__)

#endif
