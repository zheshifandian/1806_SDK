#ifndef _UCI_CONFIG_H_
#define _UCI_CONFIG_H_

#include <pthread.h>
#include <uci.h>
#include "list.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#include "CWProtocol.h"
#include "capwap_common.h"

struct wifi_attr {
	int enabled;
	char *ssid;
	int hide;
	int encryption;
	char *password;
	int isolation;
	int bandwidth_limit;
	int bandwidth_upload;
	int bandwidth_download;
	int channel;
	char *country_code;
	char *bandwidth;
	int weak_sta_signal_enable;
	int prohibit_sta_signal_enable;
	int weak_sta_signal;
	int prohibit_sta_signal;
	/* private */
};

#define WIFI_ATTR_SIZE (sizeof(struct wifi_attr))

struct wifi {
	struct wifi_attr band[WIFI_NUM];
};

#define ENCRYPTION_OPEN		0
#define ENCRYPTION_WPA2_PSK	2

// Device status
#define DEV_OFFLINE	0
#define DEV_RUNNING	1
#define DEV_SETTING	2
#define DEV_SET_ERROR	3

struct device_attr {
	char *name;
	char *model;
	char *hardware_version;
	char *firmware_version;
	char *mac;
	int status;
	char *group;
	int ap_alive_time;
	int client_alive_time;
	int client_idle_time;
	int led;
	int updating;
	const char *custom_wifi;
	struct wifi wifi;

	/* private */
	struct capwap_wtp *wtp;
	struct uci_section *section;
	pthread_mutex_t mutex;
	struct list_head list;
	struct list_head save_list;
};

struct group_attr {
	char *name;
	struct wifi wifi;

	/* private */
	//struct list_head list;
};

struct attr_convert {
	const char *option;
	char *(*to_uci)(void *);
	void (*to_attr)(void *, const char *);
};

#define DEV_CONFIG "capwap_devices"
#define GROUP_CONFIG "ap_groups"
#define DEFAULT_WTP_NAME "86AP"

#define FREE_STRING(s) \
	do {               \
		if (s)         \
			free(s);   \
		s = NULL;      \
	} while (0)

/*
 * If custom_wifi == "group", wifi attr comes from ap_group, return true in this situation;
 * return false otherwise.
 */
bool use_group_wifi_attr(struct device_attr *attr);

/*
 * 1. Save attr changes to config file or
 * 2. When a new device first connect, we use this function to save this device to uci config
 *    file. This will create new uci section and add this device to device_list.
 */
int save_device_config(struct device_attr *attr, int locked);

/*
 * Given a group name and return this group's attr.
 * Return NULL if not found.
 */
struct group_attr *find_group_attr_by_name(const char *name);

/*
 * If a device is added to a group, set this device's wifi_attr to the group's wifi_attr.
 * This function will change this device's custom_wifi and group options.
 */
void set_group_attr_to_device(struct device_attr *device, struct group_attr *group);

/*
 * Convert a uci config to device_attr
 */
// static int uci_to_dev_attr(struct uci_section *section, struct device_attr *attr);

/*
 * We save all the known device_attr in a list, find the device_attr with the same
 * mac address as passed in.
 */
struct device_attr *find_dev_attr_by_mac(const char *mac);
struct device_attr *find_dev_attr_by_raw_mac(uint8_t mac[6]);

/*
 * Read devices' uci config file, and convert each section to a device_attr,
 * and add these attr to device_list.
 */
int load_device_configs(void);

char *get_dev_uci_attr(struct device_attr *attr, const char *name);

/*
 * Read groups' uci config file, and convert each section to a group_attr,
 * and add these attr to group_list.
 *
 * int load_group_configs(void);
 */
/*
 * Malloc new memory of device_attr, and set wifi_attr to "default" group.
 * This function doesn't add this device_attr to device_list.
 */
struct device_attr *dev_attr_alloc();

/**
 * Reload a device config.
 * Must be used with attr->mutex hold.
 */
void reload_dev_attr(struct device_attr *dev);

/**
 * Delete a device config.
 * Must be used with attr->mutex hold.
 */
int delete_device_config(struct device_attr *attr);

void dev_attr_free(struct device_attr *attr);
void wifi_attr_free(struct wifi *attr);
void group_attr_free(struct group_attr *attr);

int dev_attr_mutex_lock(struct device_attr *attr);
int dev_attr_mutex_unlock(struct device_attr *attr);

/*
 * Alloc uci context, load device and group config files, convert known configs
 * to attr, and add them to their own list(device_list or group_list).
 */
int uci_interface_init(void);

void uci_interface_free(void);

/* The following functions may not needed by others.(Not sure for that) */
void uci_option_to_wifi_attr(struct uci_option *o, struct wifi *wifi);
void uci_to_group_attr(struct uci_section *section, struct group_attr *attr);

#endif /* _UCI_CONFIG_H_ */
