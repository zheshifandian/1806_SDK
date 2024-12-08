#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <json-c/json.h>

#include "capwap_common.h"
#include "common.h"
#include "wtp.h"
#include "config_file.h"
#include "CWLog.h"

static LIST_HEAD(mac_list);
static uint16_t mobility_domain;

static struct mac_element *mac_list_find(uint8_t *mac)
{
	struct mac_element *ele;

	list_for_each_entry(ele, &mac_list, list) {
		if (!memcmp(ele->mac, mac, ETH_ALEN))
			return ele;
	}
	return NULL;
}

void mac_list_del(uint8_t *mac)
{
	struct mac_element *ele;

	ele = mac_list_find(mac);
	if (ele) {
		list_del(&ele->list);
		free(ele);
	}
}

/**
 * Add given mac address to mac_list.
 * @mac: the mac address we want to add
 * @check: whether to check if the mac is already in the list.
 * 	   If set to 1, won't add if it's already in.
 * Return: 0 if not added, 1 if added.
 */
int mac_list_add(uint8_t *mac, int check)
{
	struct mac_element *ele;

	if (check && mac_list_find(mac))
		return 0;
	ele = malloc(sizeof(*ele));
	if (!ele)
		return 0;
	memcpy(ele->mac, mac, ETH_ALEN);
	list_add(&ele->list, &mac_list);
	return 1;
}

void set_mobility_domain(uint16_t val)
{
	mobility_domain = val;
}

static json_bool hostapd_check_noscan(struct hostapd_config *conf)
{
	if (strcmp(conf->ht_mode, "HT40") == 0)
		return TRUE;
	if (strcmp(conf->interface, "wlan1") == 0)
		return TRUE;
	return FALSE;
}

static void hostapd_config_validate(struct hostapd_config *conf)
{
	if (conf->ht_mode && conf->ht_mode[0] != '\0')
		return;

	free(conf->ht_mode);
	if (conf->hw_mode[2] == 'a') {
		// 5G
		if (conf->channel == 165)
			conf->ht_mode = strdup("VHT20");
		else
			conf->ht_mode = strdup("VHT80");
	} else {
		// 2.4G
		conf->ht_mode = strdup("HT20");
	}
}

/**
 * Send json command to hostapd_config.sh to generate hostapd config files.
 * The json format is refered from the input of /lib/netifd/wireless/mac80211.sh on normal routers.
 */
int hostapd_config_save(struct hostapd_config *conf)
{
	struct json_object *data, *config, *interface;
	struct json_object *tmp;
	struct mac_element *ele;
	int ret, len;
	char *command;
	char channel[5] = {0};
	char roam[32];
	uint8_t mac[6];

	hostapd_config_validate(conf);

	ret = get_mac_addr(conf->interface, mac);
	if (ret)
		return ret;
	data = json_object_new_object();
	config = json_object_new_object();
	interface = json_object_new_object();
	if (!data || !config || !interface)
		return -ENOMEM;
	json_object_object_add(config, "beacon_int", json_object_new_int(200));
	json_object_object_add(config, "path", json_object_new_string(conf->path));
	json_object_object_add(config, "hwmode", json_object_new_string(conf->hw_mode));
	json_object_object_add(config, "htmode", json_object_new_string(conf->ht_mode));
	json_object_object_add(config, "noscan", json_object_new_boolean(hostapd_check_noscan(conf)));
	json_object_object_add(config, "country", json_object_new_string(conf->country_code));
	if (conf->channel)
		snprintf(channel, sizeof(channel), "%d", conf->channel);
	else
		snprintf(channel, sizeof(channel), "auto");
	json_object_object_add(config, "channel", json_object_new_string(channel));
	if (conf->disassociate_weak_sig_sta_enable) {
		json_object_object_add(config, "disassociate_weak_sig_sta_enable", json_object_new_boolean(TRUE));
		json_object_object_add(config, "weak_sta_signal", json_object_new_int(conf->weak_sta_signal));
	}
	if (conf->prohibit_weak_sig_sta_enable) {
		json_object_object_add(config, "prohibit_weak_sig_sta_enable", json_object_new_boolean(TRUE));
		json_object_object_add(config, "sta_min_dbm", json_object_new_int(conf->sta_min_dbm));
	}

	// interface
	json_object_object_add(interface, "ifname", json_object_new_string(conf->interface));
	json_object_object_add(interface, "mode", json_object_new_string("ap"));
	if (conf->wpa) {
		json_object_object_add(interface, "encryption", json_object_new_string("psk2+ccmp"));
		json_object_object_add(interface, "key", json_object_new_string(conf->wpa_passphrase));
	}
	json_object_object_add(interface, "isolate", json_object_new_boolean(FALSE));
	json_object_object_add(interface, "hidden", json_object_new_boolean(conf->ignore_broadcast_ssid));
	json_object_object_add(interface, "ssid", json_object_new_string(conf->ssid));
	tmp = json_object_new_array();
	json_object_array_add(tmp, json_object_new_string(NETWORK_IF));
	json_object_object_add(interface, "network", tmp);
	json_object_object_add(interface, "ieee80211r", json_object_new_boolean(conf->ieee80211r));
	if (conf->ieee80211r) {
		json_object_object_add(interface, "pmk_r1_push", json_object_new_boolean(conf->ieee80211r));
		snprintf(roam, sizeof(roam), MACSTR, MAC2STR(mac));
		json_object_object_add(interface, "nasid", json_object_new_string(roam));
		snprintf(roam, sizeof(roam), COMPACT_MACSTR, MAC2STR(mac));
		json_object_object_add(interface, "r1_key_holder", json_object_new_string(roam));
		snprintf(roam, sizeof(roam), "%x", mobility_domain);
		json_object_object_add(interface, "mobility_domain", json_object_new_string(roam));
		tmp = json_object_new_array();
		list_for_each_entry(ele, &mac_list, list) {
			snprintf(roam, sizeof(roam), MACSTR ",", MAC2STR(ele->mac));
			json_object_array_add(tmp, json_object_new_string(roam));
		}
		json_object_object_add(interface, "r1mac", tmp);
	}
	// Fixed format bellow, usually no need to modify.
	tmp = json_object_new_object();
	json_object_object_add(tmp, "config", interface);
	json_object_object_add(tmp, "bridge", json_object_new_string(NETWORK_IF));
	interface = tmp;
	tmp = json_object_new_object();
	json_object_object_add(tmp, "0", interface);
	json_object_object_add(data, "interfaces", tmp);
	json_object_object_add(data, "config", config);

	// execute shell script to generate config file.
	len = snprintf(NULL, 0, HOSTAPD_CONFIG_BIN" '%s' %s", json_object_to_json_string(data), conf->interface) + 1;
	command = MALLOC(len);
	if (!command) {
		json_object_put(data);
		return -ENOMEM;
	}
	snprintf(command, len, HOSTAPD_CONFIG_BIN" '%s' %s", json_object_to_json_string(data), conf->interface);
	printf("execute:%s\n", command);
	ret = system(command);
	free(command);
	json_object_put(data);

	return WEXITSTATUS(ret);
}

void hostapd_config_free(struct hostapd_config *conf)
{
	if (!conf)
		return;

	free(conf->ssid);
	free(conf->country_code);
	free(conf->wpa_passphrase);
	free(conf->ht_mode);
	free(conf);
}

/**
 * Init some necessary configs for 2G and 5G. These configs are different for 2G
 * and 5G, and won't be set by AC.
 */
int hostapd_config_init(struct capwap_ap *ap)
{
	ap->config[WIFI_2G] = MALLOC(sizeof(struct hostapd_config));
	ap->config[WIFI_5G] = MALLOC(sizeof(struct hostapd_config));
	if (!ap->config[WIFI_2G] || !ap->config[WIFI_5G])
		return -ENOMEM;

	ap->config[WIFI_2G]->path = WIFI_2G_PATH;
	ap->config[WIFI_2G]->hw_mode = "11g";
	ap->config[WIFI_2G]->interface = "wlan0";
	ap->config[WIFI_2G]->ctrl_interface = "/var/run/hostapd";
	ap->config[WIFI_2G]->ieee80211r = 0;

	ap->config[WIFI_5G]->path = WIFI_5G_PATH;
	ap->config[WIFI_5G]->hw_mode = "11a";
	ap->config[WIFI_5G]->interface = "wlan1";
	ap->config[WIFI_5G]->ctrl_interface = "/var/run/hostapd";
	ap->config[WIFI_5G]->ieee80211r = 0;

	return 0;
}
