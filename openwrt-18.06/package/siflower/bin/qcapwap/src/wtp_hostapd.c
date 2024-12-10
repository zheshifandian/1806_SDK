#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/prctl.h>
#include <event2/event.h>

#include "common.h"
#include "capwap_common.h"
#include "CWProtocol.h"
#include "capwap_message.h"
#include "network.h"
#include "config_file.h"
#include "wtp.h"
#include "wpa_ctrl.h"
#include "wtp_hostapd.h"
#include "CWLog.h"

// static int chan_to_freq(int chan)
// {
// 	if (chan > 0 && chan < 15)
// 		return 2407 + 5 * chan;
// 	else if (chan >= 36)
// 		return 5000 + 5 * chan;
// 	else
// 		return -1;
// }

static int capwap_wait_unisock(const char *sock_path)
{
	struct stat s = {0};
	int timeout = 3 * 1000 * 1000;

	while ((stat(sock_path, &s) < 0) ||
	       !S_ISSOCK(s.st_mode)) {
		usleep(100 * 1000);
		timeout -= 100 * 1000;
	}
	return timeout > 0 ? 0 : -ETIMEDOUT;
}

static int capwap_send_station_delete_event(struct capwap_ap *ap, const char *mac, int band)
{
	struct cw_ctrlmsg *msg;
	struct message message;
	uint8_t *buff;
	uint16_t len = 2 + ETH_ALEN;

	msg = cwmsg_ctrlmsg_new(CW_TYPE_WTP_EVENT_REQUEST, wtp_get_seq_num(ap));
	buff = malloc(len);
	if (!msg || !buff)
		return -ENOMEM;
	buff[0] = (uint8_t)band;
	buff[1] = ETH_ALEN;
	if (hwaddr_aton(mac, &buff[2])) {
		free(buff);
		return -EINVAL;
	}
	message.data = buff;
	message.len = len;
	if (cwmsg_ctrlmsg_add_element(msg, CW_ELEM_DELETE_STATION_CW_TYPE, &message, TLV_NOCPY)) {
		cwmsg_ctrlmsg_destroy(msg);
		return -ENOMEM;
	}
	CWLog("%s: %s", __func__, mac);
	return wtp_send_request(ap, msg);
}

static int capwap_send_station_online_event(struct capwap_ap *ap, const char *mac, const char *if_name, int band)
{
	struct message message;
	struct cw_ctrlmsg *msg;

	msg = cwmsg_ctrlmsg_new(CW_TYPE_WTP_EVENT_REQUEST, wtp_get_seq_num(ap));
	message.data = MALLOC(64);
	if (!msg || !message.data)
		return -ENOMEM;
	message.len = 0;
	cwmsg_put_u32(&message, 1);
	cwmsg_put_u8(&message, ETH_ALEN);
	if (hwaddr_aton(mac, message.data + message.len)) {
		free(message.data);
		return -EINVAL;
	}
	message.len += ETH_ALEN;
	if (SUPPORT_HOSTAPD(ap->ac_version))
		cwmsg_put_u8(&message, (uint8_t)band);
	cwmsg_put_u8(&message, strlen(if_name));
	cwmsg_put_raw(&message, if_name, strlen(if_name));

	if (cwmsg_ctrlmsg_add_element(msg, CW_ELEM_STATION_ONLINE_CW_TYPE, &message, TLV_NOCPY)) {
		cwmsg_ctrlmsg_destroy(msg);
		return -ENOMEM;
	}
	CWLog("%s: %s", __func__, mac);
	return wtp_send_request(ap, msg);
}

static int capwap_get_interface_config(struct capwap_ap *ap, int band, uint8_t *bssid)
{
	char *request = "GET_CONFIG";
	char *pos;
	void *result;
	size_t result_len = 4096;
	int err;

	result = malloc(result_len);
	if (!result)
		return -ENOMEM;
	err = wpa_ctrl_request(ap->request[band], request, strlen(request), result, &result_len, NULL);
	if (err < 0) {
		CWLog("Request get config failed");
		free(result);
		return err;
	}
	pos = strstr(result, "bssid=");
	if (!pos) {
		free(result);
		return -EINVAL;
	}
	pos += 6;
	err = hwaddr_aton(pos, bssid);
	free(result);
	return err;
}

static int capwap_send_wlan_config_response(struct capwap_ap *ap, uint8_t band, uint8_t *bssid, int err)
{
	struct message message;
	struct cw_ctrlmsg *msg;

	msg = cwmsg_ctrlmsg_new(CW_TYPE_WLAN_CONFIGURATION_RESPONSE, ap->resp_seq_num);
	message.data = MALLOC(8);
	if (!msg || !message.data)
		return -ENOMEM;
	cwmsg_put_u8(&message, band);
	cwmsg_put_u8(&message, 0);
	cwmsg_put_raw(&message, bssid, ETH_ALEN);
	if (cwmsg_assemble_result_code(msg, err) ||
	    cwmsg_ctrlmsg_add_element(msg, CW_ELEM_IEEE80211_ASSIGNED_WTP_BSSID_CW_TYPE, &message, TLV_NOCPY)) {
		cwmsg_ctrlmsg_destroy(msg);
		return -ENOMEM;
	}
	return capwap_send_response(ap->ctrl_sock, msg);
}

static void capwap_hostapd_event(int fd, short what, void *arg) {
	struct capwap_ap *ap = arg;
	char *buff, *pos;
	size_t buff_len = 4 * 1024;
	int band;

	if (what & EV_READ) {
		buff = malloc(buff_len);
		if (!buff)
			return;
		for (band = 0; band < WIFI_NUM; band++) {
			if (ap->monitor[band] && wpa_ctrl_get_fd(ap->monitor[band]) == fd)
				break;
		}
		if (band >= WIFI_NUM)
			return;
		if (wpa_ctrl_recv(ap->monitor[band], buff, &buff_len)) {
			CWLog("Receive hostapd event failed");
			free(buff);
			return;
		}
		buff[buff_len] = '\0';
		CWLog("EVENT:%s", buff);
		if (*buff == '<') {
			// skip priority
			pos = strchr(buff, '>');
			if (pos)
				pos++;
			else
				pos = buff;
		}

		if (!strncmp(pos, AP_STA_CONNECTED, strlen(AP_STA_CONNECTED))) {
			// STA connected
			pos = strchr(pos, ' ');
			if (!pos) {
				free(buff);
				return;
			}
			pos++;
			if (capwap_send_station_online_event(ap, pos, ap->config[band]->interface, band))
				CWLog("Send station %s online event failed", pos);
		} else if (!strncmp(pos, AP_STA_DISCONNECTED, strlen(AP_STA_DISCONNECTED))) {
			// STA disconnected
			pos = strchr(pos, ' ');
			if (!pos) {
				free(buff);
				return;
			}
			pos++;
			if (capwap_send_station_delete_event(ap, pos, band))
				CWLog("Send station %s offline event failed", pos);
		} else {
			CWLog("Un-care event:%s", pos);
		}
		free(buff);
		return;
	}
}

static int capwap_hostapd_connect(struct capwap_ap *ap, int band) {
	char sock_path[128];
	int err;

	snprintf(sock_path, sizeof(sock_path), "%s/%s", ap->config[band]->ctrl_interface, ap->config[band]->interface);

	if (capwap_wait_unisock(sock_path))
		return -EINVAL;

	ap->request[band] = wpa_ctrl_open(sock_path);
	if (!ap->request[band]) {
		CWLog("open hostapd ctrl interface %d failed", band);
		return -EINVAL;
	}
	ap->monitor[band] = wpa_ctrl_open(sock_path);
	if (!ap->monitor[band]) {
		err = -EINVAL;
		goto err_monitor;
	}
	if (wpa_ctrl_attach(ap->monitor[band])) {
		CWLog("Failed to attach to interface %d", band);
		err = -EINVAL;
		goto err_attach;
	}
	ap->ap_ev[band] = event_new(ap->base, wpa_ctrl_get_fd(ap->monitor[band]), EV_READ | EV_PERSIST, capwap_hostapd_event, ap);
	if (event_add(ap->ap_ev[band], NULL) < 0) {
		err = -ENOMEM;
		goto err_add;
	}
	return 0;

err_add:
	event_free(ap->ap_ev[band]);
	ap->ap_ev[band] = NULL;
err_attach:
	wpa_ctrl_close(ap->monitor[band]);
	ap->monitor[band] = NULL;
err_monitor:
	wpa_ctrl_close(ap->request[band]);
	ap->request[band] = NULL;

	return err;
}

static int hostapd_find_phy(struct capwap_ap *ap)
{
	DIR *pDir;
	struct dirent *ent;
	char *buff;
	int vht_cap, cap_5ghz;

	FREE(ap->phy_name[WIFI_2G]);
	FREE(ap->phy_name[WIFI_5G]);
	pDir = opendir("/sys/class/ieee80211");
	while((ent = readdir(pDir)) != NULL) {
		if (strncmp(ent->d_name, "phy", 3))
			continue;
		buff = execute_command("iw phy %s info | grep -c 'VHT Capabilities'", ent->d_name);
		if (!buff)
			continue;
		vht_cap = atoi(buff);
		free(buff);
		buff = execute_command("iw phy %s info | grep -c 'Band 2'", ent->d_name);
		if (!buff)
			continue;
		cap_5ghz = atoi(buff);
		free(buff);
		if (vht_cap > 0 && cap_5ghz > 0)
			ap->phy_name[WIFI_5G] = strdup(ent->d_name);
		else
			ap->phy_name[WIFI_2G] = strdup(ent->d_name);
	}
	closedir(pDir);
	if (!ap->phy_name[WIFI_2G] && !ap->phy_name[WIFI_5G]) {
		CWLog("Can't find supported phy, maybe no wifi driver is installed");
		return -ENOTSUP;
	}

	return 0;
}

int capwap_start_hostapd(struct capwap_ap *ap)
{
	pid_t pid;

	if (remove(HOSTAPD_GLOABLE_INTERFACE) && errno != ENOENT) {
		CWLog("Hostapd gloable interface maybe already in use, %s", strerror(errno));
		return -EBUSY;
	}

	if (hostapd_config_init(ap))
		return -ENOMEM;

	// Start hostapd
	pid = fork();
	if (pid < 0) {
		CWCritLog("start hostapd failed: %s", strerror(errno));
		return -1;
	} else if (pid == 0) { // Child
		prctl(PR_SET_PDEATHSIG, SIGINT);
		if (execlp("hostapd", "hostapd", "-g", HOSTAPD_GLOABLE_INTERFACE, NULL) < 0)
			CWCritLog("Start hostapd fail in child process");
		exit(127);
	}

	if (capwap_wait_unisock(HOSTAPD_GLOABLE_INTERFACE))
		return -EFAULT;

	ap->hostapd = wpa_ctrl_open(HOSTAPD_GLOABLE_INTERFACE);
	if (!ap->hostapd) {
		CWLog("Can't open hostapd interface");
		return -EINVAL;
	}

	return pid;
}

int capwap_hostapd_update(struct capwap_ap *ap, int band, uint16_t cmd)
{
	char conf[32] = {0};
	char buf[64] = {0};
	int buf_len;
	char result[16] = {0};
	size_t result_len = sizeof(result);
	int err = 0;

	if (ap->request[band]) {
		buf_len = snprintf(buf, sizeof(buf), "REMOVE %s", ap->config[band]->interface);
		CWLog("%s", buf);
		err = wpa_ctrl_request(ap->hostapd, buf, buf_len, result, &result_len, NULL);
		if (err < 0) {
			CWLog("Send ctrl request fail");
			return err;
		}
		if (strncmp(result, "OK", 2)) {
			CWLog("%s failed", buf);
			return -EINVAL;
		}
		event_free(ap->ap_ev[band]);
		ap->ap_ev[band] = NULL;
		wpa_ctrl_close(ap->request[band]);
		ap->request[band] = NULL;
		wpa_ctrl_close(ap->monitor[band]);
		ap->monitor[band] = NULL;
	}

	if (cmd == CW_ELEM_IEEE80211_ADD_WLAN_CW_TYPE) {
		// Phy name maybe changed, so we have to get phy name here.
		err = hostapd_find_phy(ap);
		if (err)
			return err;
		if (hostapd_config_save(ap->config[band]) < 0) {
			CWLog("Save wifi config fail");
			return -EIO;
		}
		snprintf(conf, sizeof(conf), WTP_CONFIG_FILE_PREFIX"%s.conf", ap->config[band]->interface);
		buf_len = snprintf(buf, sizeof(buf), "ADD bss_config=%s:%s", ap->phy_name[band], conf);
		err = wpa_ctrl_request(ap->hostapd, buf, buf_len, result, &result_len, NULL);
		if (err < 0) {
			CWLog("Request ADD interface failed");
			return err;
		}
		if (strncmp(result, "OK", 2)) {
			CWLog("ADD %s:%s failed", ap->phy_name[band], conf);
			return -EINVAL;
		}
		err = capwap_hostapd_connect(ap, band);
		if (err < 0)
			return err;
	}
	return 0;
}

int wtp_handle_wlan_request(struct capwap_ap *ap, struct cw_ctrlmsg *msg)
{
	struct tlv_box *box = NULL;
	uint16_t elem_type, elem_len, cmd = 0;
	void *elem_value;
	int band = -1, len = 0, err = 0;
	int result_code = CW_PROTOCOL_SUCCESS;
	uint8_t bssid[ETH_ALEN];

	cwmsg_ctrlmsg_for_each_elem (msg, elem_type, elem_len, elem_value) {
		switch (elem_type) {
		case CW_ELEM_IEEE80211_ADD_WLAN_CW_TYPE:
			// parse radio_id
			band = cwmsg_parse_u8(elem_value);
			// there should be only one type in a single wlan request
			if (band >= WIFI_NUM || cmd) {
				result_code = CW_PROTOCOL_FAILURE_INCORRECT_DATA;
				goto send;
			}
			// 6 = radio_id + wlan_id + capability + key_index + key_status
			elem_value += 6;
			// parse key length
			len = cwmsg_parse_u16(elem_value);
			// u16 + key length + group_tsc + qos + auth_type + mac_mode + tunnel_mode
			elem_value += 2 + len + 10;
			ap->config[band]->ignore_broadcast_ssid = cwmsg_parse_u8(elem_value++) ? 0 : 1;
			ap->config[band]->channel = cwmsg_parse_u8(elem_value++);
			if (SUPPORT_COUNTRY_CODE(ap->ac_version)) {
				FREE(ap->config[band]->country_code);
				ap->config[band]->country_code = cwmsg_parse_string(elem_value, 2);
				elem_value += 2;
			} else {
				ap->config[band]->country_code = strdup("CN");
			}
			if (SUPPORT_WEAK_SIGNAL(ap->ac_version)) {
				ap->config[band]->disassociate_weak_sig_sta_enable = cwmsg_parse_u8(elem_value++);
				ap->config[band]->prohibit_weak_sig_sta_enable = cwmsg_parse_u8(elem_value++);
				ap->config[band]->weak_sta_signal = cwmsg_parse_u32(elem_value);
				elem_value += 4;
				ap->config[band]->sta_min_dbm = cwmsg_parse_u32(elem_value);
				elem_value += 4;
			} else {
				ap->config[band]->disassociate_weak_sig_sta_enable = 0;
				ap->config[band]->prohibit_weak_sig_sta_enable = 0;
			}
			FREE(ap->config[band]->ssid);
			ap->config[band]->ssid = cwmsg_parse_string(elem_value, elem_len - 32 - len);
			cmd = CW_ELEM_IEEE80211_ADD_WLAN_CW_TYPE;
			break;
		case CW_ELEM_VENDOR_SPEC_HTMODE:
			band = cwmsg_parse_u8(elem_value++);
			ap->config[band]->ht_mode = cwmsg_parse_string(elem_value, elem_len - 1);
			break;
		case CW_ELEM_VENDOR_SPEC_PAYLOAD_BW_CW_TYPE:
			// This contains wifi encryption method and password.
			box = tlv_box_create(SERIAL_WITH_ID);
			if (!box || tlv_box_parse(box, elem_value, elem_len)) {
				result_code = CW_PROTOCOL_FAILURE_INCORRECT_DATA;
				goto send;
			}
			break;
		case CW_ELEM_IEEE80211_DELETE_WLAN_CW_TYPE:
			band = cwmsg_parse_u8(elem_value);
			if (band >= WIFI_NUM || cmd) {
				result_code = CW_PROTOCOL_FAILURE_INCORRECT_DATA;
				goto send;
			}
			cmd = CW_ELEM_IEEE80211_DELETE_WLAN_CW_TYPE;
			break;
		}
	}
	if (box && band < 0) {
		result_code = CW_PROTOCOL_FAILURE_INCORRECT_DATA;
		goto send;
	}
	// parse again to get wpa and password
	if (box) {
		tlv_box_for_each_tlv (box, elem_type, elem_len, elem_value) {
			switch (elem_type) {
			case CW_ELEM_VENDOR_SPEC_PAYLOAD_WPA:
				ap->config[band]->wpa = cwmsg_parse_u8(elem_value);
				break;
			case CW_ELEM_VENDOR_SPEC_PAYLOAD_WPA_PASSPHRASE:
				FREE(ap->config[band]->wpa_passphrase);
				ap->config[band]->wpa_passphrase = cwmsg_parse_string(elem_value, elem_len);
				break;
			}
		}
	} else {
		ap->config[band]->wpa = 0;
		FREE(ap->config[band]->wpa_passphrase);
	}
	if (SUPPORT_AP_ROAM(ap->ac_version))
		ap->config[band]->ieee80211r = 1;
	// Add or delete wifi interface
	err = capwap_hostapd_update(ap, band, cmd);
	if (err < 0) {
		CWLog("Hostapd update failed %d", err);
		result_code = err;
	}

send:
	if (box)
		tlv_box_destroy(box);
	if (result_code != CW_PROTOCOL_SUCCESS)
		ap->state = SULK;
	if (cmd == CW_ELEM_IEEE80211_ADD_WLAN_CW_TYPE && result_code == CW_PROTOCOL_SUCCESS) {
		// Send response with bssid. Get bssid first.
		err = capwap_get_interface_config(ap, band, bssid);
		return capwap_send_wlan_config_response(ap, band, bssid, err);
	}

	err = wtp_send_result_code(ap, CW_TYPE_WLAN_CONFIGURATION_RESPONSE, result_code);
	if (result_code != CW_PROTOCOL_SUCCESS)
		return result_code;
	return err;
}

void capwap_close_hostapd(struct capwap_ap *ap)
{
	int i;

	if (!ap)
		return;
	for (i = 0; i < WIFI_NUM; i++) {
		FREE(ap->phy_name[i]);
		hostapd_config_free(ap->config[i]);
		if (ap->ap_ev[i]) {
			event_free(ap->ap_ev[i]);
			ap->ap_ev[i] = NULL;
		}
		if (ap->request[i]) {
			wpa_ctrl_close(ap->request[i]);
			ap->request[i] = NULL;
		}
		if (ap->monitor[i]) {
			wpa_ctrl_close(ap->monitor[i]);
			ap->monitor[i] = NULL;
		}
	}
	if (ap->hostapd) {
		wpa_ctrl_close(ap->hostapd);
		ap->hostapd = NULL;
	}
}

void wtp_restart_hostapd(evutil_socket_t fd, short what, void *arg)
{
	struct capwap_ap *ap = arg;
	int i;

	if (!(what & EV_TIMEOUT))
		return;
	CWLog("Restart wifi to use new ap roam list");
	for (i = 0; i < WIFI_NUM; i++) {
		if (!ap->request[i])
			continue;
		capwap_hostapd_update(ap, i, CW_ELEM_IEEE80211_ADD_WLAN_CW_TYPE);
	}
}

/**
 * Add new mac address for ap roam.
 * Davy.Pan Added a new hostapd command to update roam mac list in hostapd,
 * send "R1KH XX_XX_XX_XX_XX_XX XX_XX_XX_XX_XX_XX ..." to hostapd to do this.
 * There can be multiple mac addresses in a single request, just split each
 * other with a space.
 *
 * Note: we don't have to delete mac from list if the other AP is offline, as
 * if it's offline, we can not connect to it's wifi, obviously.
 */
static int hostapd_update_ap_roam(struct capwap_ap *ap, char *mac, int len)
{
	char result[16] = {0};
	char *buff;
	int buff_len, i, err;
	size_t result_len = sizeof(result);

	buff_len = strlen("R1KH ") + len;
	buff = malloc(buff_len);
	if (!buff)
		return -ENOMEM;
	snprintf(buff, buff_len, "R1KH %s", mac);
	CWLog("%s", buff);
	for (i = 0; i < WIFI_NUM; i++) {
		if (!ap->request[i])
			continue;
		err = wpa_ctrl_request(ap->request[i], buff, buff_len, result, &result_len, NULL);
		if (err < 0) {
			CWLog("Send ctrl request fail");
			free(buff);
			return err;
		}
		if (strncmp(result, "OK", 2)) {
			CWLog("%s failed", buff);
			CWLog("%s", result);
			free(buff);
			return -EINVAL;
		}
	}
	free(buff);
	return 0;
}

static int wtp_parse_ap_roam(struct capwap_ap *ap, struct cw_ctrlmsg *msg, char **mac, int *mac_len)
{
	uint16_t elem_type;
	uint16_t elem_len;
	void *elem_value;
	uint16_t len = 0;
	uint8_t type;
	int add = 0;
	int buff_len = 0;

	CWLog("receive ap roam mac list");
	*mac_len = 0;
	cwmsg_ctrlmsg_for_each_elem (msg, elem_type, elem_len, elem_value) {
		switch (elem_type) {
		case CW_ELEM_MOBILITY_DOMAIN:
			set_mobility_domain(cwmsg_parse_u16(elem_value));
			break;
		case CW_ELEM_AP_ROAM_LIST:
			*mac_len += elem_len / 7 * 18;
			if (*mac_len != elem_len / 7 * 18)
				*mac = realloc(*mac, *mac_len + elem_len / 7 * 18);
			else
				*mac = malloc(*mac_len);
			if (!*mac)
				return 0;
			for (len = 0; len < elem_len;) {
				type = cwmsg_parse_u8(elem_value + len);
				len++;
				if (type == ADD) {
					// snprintf returns strlen(str)
					add += mac_list_add(elem_value + len, ap->state == RUN);
					buff_len += snprintf(*mac + buff_len, *mac_len - buff_len, MACSTR ",",
							     MAC2STR((uint8_t *)elem_value + len));
				}
				else if (type == DEL)
					mac_list_del(elem_value + len);
				len += 6;
			}
			break;
		default:
			break;
		}
	}
	CWLog("Add %d new mac addr", add);
	return add;
}

int wtp_handle_ap_roam(struct capwap_ap *ap, struct cw_ctrlmsg *msg)
{
	char *mac_str = NULL;
	int mac_len;
	int ret;

	ret = wtp_parse_ap_roam(ap, msg, &mac_str, &mac_len);
	if (ap->state == AP_ROAM) {
		ap->state = RUN;
		free(mac_str);
		return 0;
	}

	if (ret && mac_str)
		ret = hostapd_update_ap_roam(ap, mac_str, mac_len);
	free(mac_str);
	return ret;
}
