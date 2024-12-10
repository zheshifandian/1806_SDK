#ifndef _CONFIG_FILE_H_
#define _CONFIG_FILE_H_

#include "wtp.h"

#define HOSTAPD_CONFIG_BIN "/usr/sbin/hostapd_config"
#define WTP_CONFIG_FILE_PREFIX "/var/run/hostapd-"

#define WIFI_2G_PATH "platform/11000000.wifi-lb"
#define WIFI_5G_PATH "platform/11400000.wifi-hb"

/**
 * We keep a mac list in the memmory to keep track of existing macs.
 * In this way, if we received a mac address which is already in the list,
 * we don't have to add again.
 */
int mac_list_add(uint8_t *mac, int check);
void mac_list_del(uint8_t *mac);
void set_mobility_domain(uint16_t val);

/**
 * This struct contains necessary informations for a single wifi setting.
 */
struct hostapd_config {
	int channel;
	int wpa;
	int ignore_broadcast_ssid;
	int prohibit_weak_sig_sta_enable;
	int sta_min_dbm;
	int disassociate_weak_sig_sta_enable;
	int weak_sta_signal;
	int ieee80211r;
	const char *path;
	const char *hw_mode;
	const char *ctrl_interface;
	const char *interface;
	char *ht_mode;
	char *ssid;
	char *country_code;
	char *wpa_passphrase;
};

void hostapd_config_free(struct hostapd_config *conf);
int hostapd_config_save(struct hostapd_config *conf);
int hostapd_config_init(struct capwap_ap *ap);

#endif // _CONFIG_FILE_H_
