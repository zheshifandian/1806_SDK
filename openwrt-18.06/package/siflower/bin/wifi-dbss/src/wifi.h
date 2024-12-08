#ifndef _WIFI_H_
#define _WIFI_H_

#include <libubox/ulog.h>

#define DEBUG(level, fmt, ...) do { \
	if (debug >= level) { \
		ulog(LOG_DEBUG, fmt, ## __VA_ARGS__); \
	} } while (0)

#define LOG   ULOG_INFO
#define ERROR ULOG_ERR

extern unsigned int debug;

int wifi_ap_init(struct ubus_context *ubus_ctx, uint32_t enable_sta_rec);
void wifi_ap_deinit_ctx(struct ubus_context *ubus_ctx);

#endif
