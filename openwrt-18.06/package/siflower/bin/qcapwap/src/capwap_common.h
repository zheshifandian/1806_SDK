#if !defined(_CAPWAP_COMMON_H_)
#define _CAPWAP_COMMON_H_

#ifndef BIT
#define BIT(n)	(1UL << (n))
#endif

#define MALLOC(n) calloc(1, n)
#define FREE(p)                  \
	do {                     \
		if (p)           \
			free(p); \
		p = NULL;        \
	} while (0)

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define AC_NAME "AC"

#define NETWORK_IF "br-lan"

#define WIFI_NUM 2
#define WIFI_2G 0
#define WIFI_5G 1

#define INET_ADDRMAXLEN 64

/**
 * Description: Add support of contory code in wifi config
 */
#define VERSION_00100 0x100
/**
 * Description: Add support of prohibiting and deauthing sta whose signal is too weak
 */
#define VERSION_00101 0x101
/**
 * Description: Rewritten capwap, use hostapd to control wifi
 */
#define VERSION_00200 0x200
/**
 * Description: Add AP roaming.
 */
#define VERSION_00201 0x201
/**
 * Description: Add bandwidth and auto-channel.
 */
#define VERSION_00202 0x202
/**
 * Description: AP send update response immediately, and use wtp event to indicate
 * download over.
 */
#define VERSION_00203 0x203

#define AC_VERSION	VERSION_00203
#define WTP_VERSION	VERSION_00203

#define SUPPORT_VERSION_NUM(version)	((version) >= VERSION_00100)
#define SUPPORT_COUNTRY_CODE(version)	((version) >= VERSION_00100)
#define SUPPORT_WEAK_SIGNAL(version)	((version) >= VERSION_00101)
#define SUPPORT_HOSTAPD(version)	((version) >= VERSION_00200)
#define SUPPORT_AP_ROAM(version)	((version) >= VERSION_00201)
#define SUPPORT_AUTO_CHANNEL(version)	((version) >= VERSION_00202)
#define SUPPORT_BANDWIDTH(version)	((version) >= VERSION_00202)
#define SUPPORT_NEW_UPDATE(version)	((version) >= VERSION_00203)

#define VENDOR_ID	23456

#define OPENWRT_VERSION		"openwrt"
#define HARDWARE_VERSION	"86v"
#define SOFTWARE_VERSION	"2.0.0"
#define MODULE_VERSION		"86v"

/**
 * A struct used to assemble tlv values.
 * @data: a buffer pointer
 * @len: how many data are avaliable in the buffer
 * This struct doesn't contain the buffer's raw length, so the
 * user must be careful to not exceed the limit.
 */
struct message {
	size_t len;
	void *data;
};

#include <netinet/in.h>

int hwaddr_aton(const char *txt, uint8_t *addr);
int get_version(char *buff, int len);
int get_hardware(char *buff, int len);
int get_model(char *buff, int len);
in_addr_t get_ipv4_addr(const char *if_name);
int get_boardcast_addr(const char *if_name, struct sockaddr *addr);
int get_mac_addr(const char* interface, uint8_t *macAddr);
int cat_simple_file(const char *fname, char *buff, int len);
char *execute_command(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

#define LED_STATUS_FILE "/tmp/wtp_ledstatus"

void save_status(const char *file_name, const char *status);
#define WTP_SAVE_STATUS(status) save_status("/tmp/wtpstatus", status)
#define AC_SAVE_STATUS(status) save_status("/tmp/acstatus", status)
#define LED_SAVE_STATUS(status)                               \
	do {                                                  \
		char __buf[8] = {0};                          \
		snprintf(__buf, sizeof(__buf), "%d", status); \
		save_status(LED_STATUS_FILE, __buf);          \
	} while (0)

#include "list.h"
struct mac_element {
	struct list_head list;
	uint8_t mac[6];
};

enum roam_event {
	SKIP,
	ADD,
	DEL,
};

struct mac_event {
	enum roam_event event;
	uint8_t *mac;
};

#endif // _CAPWAP_COMMON_H_
