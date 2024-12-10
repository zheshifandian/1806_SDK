#ifndef __ANDLINK__LOG__H
#define __ANDLINK__LOG__H
#include <syslog.h>
#define LOG(X,...) syslog(LOG_CRIT,X,##__VA_ARGS__)
#endif
