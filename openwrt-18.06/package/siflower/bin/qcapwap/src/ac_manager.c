#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ac_interface.h"
#include "capwap_message.h"
#include "ac_mainloop.h"
#include "ac_manager.h"
#include "ac_ieee80211.h"
#include "network.h"
#include "uci_config.h"
#include "common.h"
#include "ac_log.h"

#define MAX_MAC_ADDR_LENGTH 18

static LIST_HEAD(mac_list);
static int mac_list_num = 0;
static pthread_rwlock_t mac_list_lock = PTHREAD_RWLOCK_INITIALIZER;

static struct mac_element *mac_element_new(uint8_t *mac)
{
	struct mac_element *ele;

	ele = malloc(sizeof(*ele));
	if (!ele)
		return NULL;
	memcpy(ele->mac, mac, 6);
	return ele;
}

static struct mac_element *find_ac_list(uint8_t *mac)
{
	struct mac_element *ele;

	list_for_each_entry(ele, &mac_list, list) {
		if (!memcmp(mac, ele->mac, 6))
			return ele;
	}
	return NULL;
}

static int ac_list_add(uint8_t *mac)
{
	struct mac_element *ele;
	int ret = 0;

	pthread_rwlock_wrlock(&mac_list_lock);
	ele = find_ac_list(mac);
	if (!ele) {
		ele = mac_element_new(mac);
		if (!ele) {
			CWLog(NONE, "No memory for new mac element");
			goto out;
		}
		list_add_tail(&ele->list, &mac_list);
		mac_list_num++;
		ret = 1;
	}
out:
	pthread_rwlock_unlock(&mac_list_lock);
	return ret;
}

static int ac_list_del(uint8_t *mac)
{
	struct mac_element *ele;
	int ret = 0;

	pthread_rwlock_wrlock(&mac_list_lock);
	ele = find_ac_list(mac);
	if (ele) {
		list_del(&ele->list);
		mac_list_num--;
		ret = 1;
	}
	pthread_rwlock_unlock(&mac_list_lock);
	free(ele);

	return ret;
}

void ac_modify_mac_list(struct capwap_wtp *wtp)
{
	struct mac_event event[WIFI_NUM] = {0};
	uint8_t *mac;
	int i;

	if (!SUPPORT_AP_ROAM(wtp->version))
		return;

	for (i = 0; i < WIFI_NUM; i++) {
		mac = wtp->wifi[i].bssid;
		if (wtp->attr->wifi.band[i].enabled) {
			CWLog(wtp, "ac_list_add:"MACSTR, MAC2STR(mac));
			if (ac_list_add(mac))
				event[i].event = ADD;
			else
				event[i].event = SKIP;
		} else {
			CWLog(wtp, "ac_list_del:"MACSTR, MAC2STR(mac));
			if (ac_list_del(mac))
				event[i].event = DEL;
			else
				event[i].event = SKIP;
		}
		event[i].mac = mac;
	}
	capwap_wtp_send_ap_roam_event(wtp, event, WIFI_NUM);
}

void capwap_wtp_remove_from_mac_list(struct capwap_wtp *wtp)
{
	int i;

	for (i = 0; i < WIFI_NUM; i++)
		ac_list_del(wtp->wifi[i].bssid);
}

static void *capwap_assemble_mac_list(struct mac_element **ele)
{
	int len, offset = 0, i = 0;
	uint8_t *data;

	len = 7 * mac_list_num;
	data = malloc(len);
	if (!data) {
		return NULL;
	}
	list_for_each_entry_continue((*ele), &mac_list, list) {
		*(data + offset) = ADD;
		offset++;
		memcpy(data + offset, (*ele)->mac, 6);
		offset += 6;
		// Assemble no more than 150 mac lists at once
		if (++i > 150)
			break;
	}
	return data;
}

int capwap_wtp_send_mac_list(struct capwap_wtp *wtp)
{
	struct cw_ctrlmsg *msg;
	struct mac_element *ele;
	int err;

	CWLog(NONE, "assemble %d mac list", mac_list_num);
	pthread_rwlock_rdlock(&mac_list_lock);
	ele = list_first_entry(&mac_list, struct mac_element, list);
	do {
		msg = cwmsg_ctrlmsg_new(CW_TYPE_AP_ROAM_REQUEST, get_seq_num(wtp));
		if (!msg || cwmsg_assemble_mobility_domain(msg, mobility_domain) ||
		    cwmsg_assemble_ap_roam(msg, capwap_assemble_mac_list(&ele), 7 * mac_list_num)) {
			cwmsg_ctrlmsg_destroy(msg);
			pthread_rwlock_unlock(&mac_list_lock);
			return -ENOMEM;
		}
		err = capwap_send_request(wtp, msg);
		if (err) {
			pthread_rwlock_unlock(&mac_list_lock);
			return err;
		}
	} while (&ele->list != &mac_list);
	pthread_rwlock_unlock(&mac_list_lock);
	return 0;
}

static char *raw_mac_to_string(uint8_t mac[6])
{
	char *s = malloc(MAX_MAC_ADDR_LENGTH);

	if (!s)
		return NULL;
	memset(s, 0, MAX_MAC_ADDR_LENGTH);
	snprintf(s, MAX_MAC_ADDR_LENGTH, "%02hhx_%02hhx_%02hhx_%02hhx_%02hhx_%02hhx",
			 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return s;
}

static int ac_init_dev_attr(struct capwap_wtp *wtp)
{
	struct device_attr *attr = wtp->attr;

	if (!attr)
		return -EINVAL;

	attr->hardware_version = strdup(wtp->descriptor.hardware_version);
	attr->firmware_version = strdup(wtp->descriptor.software_version);
	attr->model = strdup(wtp->board_data.model);
	attr->mac = raw_mac_to_string(wtp->board_data.mac);
	if (!attr->hardware_version || !attr->firmware_version || !attr->model || !attr->mac)
		return -ENOMEM;

	attr->status = 0;
	attr->ap_alive_time = CW_NEIGHBORDEAD_INTERVAL_DEFAULT;
	// TODO: What's client idle time?
	attr->client_idle_time = 10;
	// Default led status is on
	// attr->led = wtp->vendor_spec.led_status;
	attr->led = 1;
	attr->updating = wtp->vendor_spec.update_status;
	attr->wtp = wtp;

	return 0;
}

static int ac_update_dev_attr(struct capwap_wtp *wtp)
{
	struct device_attr *attr;
	struct group_attr *group;

	if (!wtp || !wtp->attr)
		return -EINVAL;

	attr = wtp->attr;
	attr->wtp = wtp;
	FREE_STRING(attr->hardware_version);
	attr->hardware_version = strdup(wtp->descriptor.hardware_version);
	FREE_STRING(attr->firmware_version);
	attr->firmware_version = strdup(wtp->descriptor.software_version);
	FREE_STRING(attr->model);
	attr->model = strdup(wtp->board_data.model);
	if (!attr->hardware_version || !attr->firmware_version || !attr->model)
		return -ENOMEM;

	// attr->led = wtp->vendor_spec.led_status;
	attr->updating = wtp->vendor_spec.update_status;

	group = find_group_attr_by_name(attr->group);
	if (!group) {
		group = find_group_attr_by_name("default");
		if (!group)
			return -EINVAL;
		set_group_attr_to_device(attr, group);
	} else {
		wifi_attr_free(&group->wifi);
		FREE_STRING(group->name);
	}
	group_attr_free(group);

	return 0;
}

int ac_init_wtp_attr(struct capwap_wtp *wtp)
{
	int err;

	wtp->attr = find_dev_attr_by_raw_mac(wtp->board_data.mac);
	if (!wtp->attr) {
		wtp->attr = dev_attr_alloc();
		err = ac_init_dev_attr(wtp);
		save_device_config(wtp->attr, 0);
	} else {
		err = ac_update_dev_attr(wtp);
	}
	return err;
}

static void ac_update_wifi_info(struct wifi_info *wifi_info, struct wifi_attr *wifi_attr)
{
	wifi_info->wlan_id = WLAN_IFACE_ID;
	wifi_info->capability = BIT(0) | BIT(10);
	if (wifi_attr->encryption != ENCRYPTION_OPEN && wifi_attr->password)
		wifi_info->capability |= BIT(4);
	// KEY, Group TSC, QoS not used
	// Auth Type: 0 open system
	// Mac Mode: 0 LocalMAC
	// Tunnel Mode: 2 802.11 Tunnel
	wifi_info->tunnel_mode = 2;
	// Suppress SSID:1 no
	wifi_info->suppress_ssid = !wifi_attr->hide;
	wifi_info->channel = wifi_attr->channel;
	strncpy(wifi_info->ssid, wifi_attr->ssid, SSID_MAX_LEN);
	if (wifi_attr->country_code) {
		memcpy(wifi_info->country_code, wifi_attr->country_code, sizeof(wifi_info->country_code));
	} else {
		wifi_info->country_code[0] = 'C';
		wifi_info->country_code[1] = 'N';
	}
	if (wifi_attr->bandwidth) {
		memcpy(wifi_info->ht_mode, wifi_attr->bandwidth, strlen(wifi_attr->bandwidth) + 1);
	} else {
		memset(wifi_info->ht_mode, 0, sizeof(wifi_info->ht_mode));
	}
	if (wifi_attr->encryption != ENCRYPTION_OPEN && wifi_attr->password) {
		wifi_info->wpa = wifi_attr->encryption;
		strncpy(wifi_info->password, wifi_attr->password, sizeof(wifi_info->password));
	} else {
		wifi_info->wpa = 0;
	}
	// Weak signal
	if(wifi_attr->weak_sta_signal) {
		wifi_info->weak_sta_signal_enable = 1;
		wifi_info->weak_sta_signal = wifi_attr->weak_sta_signal;
	} else {
		wifi_info->weak_sta_signal_enable = 0;
		wifi_info->weak_sta_signal = 0;
	}

	if(wifi_attr->prohibit_sta_signal) {
		wifi_info->prohibit_sta_signal_enable = 1;
		wifi_info->prohibit_sta_signal = wifi_attr->prohibit_sta_signal;
	} else {
		wifi_info->prohibit_sta_signal_enable = 0;
		wifi_info->prohibit_sta_signal = 0;
	}
}

/**
 * Send WLAN_CONFIGURATION_REQUEST to add wlan, normally we should call this function
 * twice to add both 2G and 5G.
 * This function only send requests.
 */
int ac_add_wlan(struct capwap_wtp *wtp)
{
	struct wifi_info *wifi_info;
	struct wifi_attr *wifi_attr;
	int band, err;

	for (band = 0; band < WIFI_NUM; band++) {
		wifi_info = &wtp->wifi[band];
		wifi_attr = &wtp->attr->wifi.band[band];
		if (wifi_info->setted)
			continue;
		if (!wifi_attr->enabled) {
			err = capwap_send_delete_wlan(wtp, band);
		} else {
			ac_update_wifi_info(wifi_info, wifi_attr);
			err = capwap_send_add_wlan(wtp, band);
		}

		if (err)
			return err;
	}
	// If both wifi are disabled for old WTPs, or we didn't change wifi_attr for new WTPs in set_device_config,
	// we'll get two setted here. Set attr->status to DEV_RUNNING in such situations.
	if (wtp->wifi[WIFI_2G].setted && wtp->wifi[WIFI_5G].setted) {
		wtp->attr->status = DEV_RUNNING;
		save_device_config(wtp->attr, 0);
	}
	return 0;
}

int ac_update_wifi(struct capwap_wtp *wtp, int i)
{
	if (!SUPPORT_HOSTAPD(wtp->version))
		return 0;
	wtp->wifi[i].setted = 0;
	wtp->attr->status = DEV_SETTING;

	return 0;
}

static int capwap_send_configure_update(struct capwap_wtp *wtp)
{
	struct cw_ctrlmsg *msg;

	msg = cwmsg_ctrlmsg_new(CW_TYPE_CONFIGURE_UPDATE_REQUEST, get_seq_num(wtp));
	if (!msg)
		return -ENOMEM;

	set_ap_idle_timeout(wtp, wtp->attr->ap_alive_time);
	wtp->vendor_spec.led_status = wtp->attr->led;
	wtp->vendor_spec.update_status = 0;
	if (cwmsg_assemble_timers(msg, wtp) || cwmsg_assemble_idle_timeout(msg, wtp) ||
	    cwmsg_assemble_vendor_config(msg, wtp)) {
		cwmsg_ctrlmsg_destroy(msg);
		return -ENOMEM;
	}
	return capwap_send_request(wtp, msg);
}

int ac_set_dev_attr(struct capwap_wtp *wtp, bool changed)
{
	int err = 0;

	if (!changed)
		return 0;

	err = capwap_send_configure_update(wtp);
	if (err)
		return err;
	capwap_set_state(wtp, SET);
	wtp->attr->status = DEV_SETTING;
	save_device_config(wtp->attr, 0);

	return 0;
}

int ac_reset_device(struct capwap_wtp *wtp)
{
	struct cw_ctrlmsg *reset;

	reset = cwmsg_ctrlmsg_new(CW_TYPE_RESET_REQUEST, get_seq_num(wtp));
	if (!reset)
		return -ENOMEM;
	return capwap_send_request(wtp, reset);
}

int ac_do_wtp_command(struct capwap_wtp *wtp, const char *command)
{
	struct cw_ctrlmsg *msg;

	msg = cwmsg_ctrlmsg_new(CW_TYPE_WTP_EXECUTE_COMMAND_REQUEST, get_seq_num(wtp));
	if (!msg)
		return -ENOMEM;
	if (cwmsg_assemble_wtp_command(msg, command))
		return -ENOMEM;
	return capwap_send_request(wtp, msg);
}
