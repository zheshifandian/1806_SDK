#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <syslog.h>
#include <time.h>
#include <arpa/inet.h>

#include "network.h"
#include "CWLog.h"

static const int log_class[] = {
	[L_CRIT] = LOG_CRIT,
	[L_WARNING] = LOG_WARNING,
	[L_NOTICE] = LOG_NOTICE,
	[L_INFO] = LOG_INFO,
	[L_DEBUG] = LOG_DEBUG
};

#ifdef WRITE_STD_OUTPUT
static const char* log_str[] = {
	[L_CRIT] = "CRITICAL",
	[L_WARNING] = "WARNING",
	[L_NOTICE] = "NOTICE",
	[L_INFO] = "INFO",
	[L_DEBUG] = "DEBUG"
};
#endif

extern int gLoggingLevel;
extern int gEnabledLog;
void log_message(int priority, const char *format, ...)
{
	va_list vl;

	if (!format)
		return;

	if (gEnabledLog && (priority <= gLoggingLevel)) {
#ifdef WRITE_STD_OUTPUT
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		va_start(vl, format);
		printf("%d-%02d-%02d %02d:%02d:%02d [capwap] %s - ", tm.tm_year + 1900,
		       tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
		       log_str[priority]);
		vprintf(format, vl);
		va_end(vl);
		printf("\n");
#endif
		openlog(NAME, 0, LOG_DAEMON);
		va_start(vl, format);
		vsyslog(log_class[priority], format, vl);
		va_end(vl);
		closelog();
	}
}
