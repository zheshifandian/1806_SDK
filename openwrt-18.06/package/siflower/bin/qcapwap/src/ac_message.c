#include <stddef.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <event2/event.h>
#include <string.h>

#include "ac_mainloop.h"
#include "capwap_message.h"
#include "capwap_common.h"
#include "CWProtocol.h"
#include "network.h"
#include "tlv.h"
#include "ac_log.h"

int cwmsg_assemble_ac_version(struct cw_ctrlmsg *msg, struct capwap_wtp *wtp)
{
	uint32_t version = htonl(AC_VERSION);

	if (!SUPPORT_VERSION_NUM(wtp->version))
		return 0;

	return cwmsg_ctrlmsg_add_raw_element(msg, CW_ELEM_VENDOR_SPEC_AC_VERSION, sizeof(version), &version);
}

int cwmsg_assemble_idle_timeout(struct cw_ctrlmsg *msg, struct capwap_wtp *wtp)
{
	uint32_t idle_timeout;

	idle_timeout = htonl(get_idle_timeout(wtp));
	return cwmsg_ctrlmsg_add_raw_element(msg, CW_ELEM_IDLE_TIMEOUT_CW_TYPE,
					     sizeof(idle_timeout), &idle_timeout);
}

int cwmsg_assemble_timers(struct cw_ctrlmsg *msg, struct capwap_wtp *wtp)
{
	uint8_t timers[2];

	timers[0] = 3;
	timers[1] = get_echo_interval(wtp);

	return cwmsg_ctrlmsg_add_raw_element(msg, CW_ELEM_CW_TIMERS_CW_TYPE, sizeof(timers),
					     timers);
}

int cwmsg_assemble_wtp_fallback(struct cw_ctrlmsg *msg, struct capwap_wtp *wtp)
{
	uint8_t led = wtp->attr->led;

	return cwmsg_ctrlmsg_add_raw_element(msg, CW_ELEM_WTP_FALLBACK_CW_TYPE, sizeof(led),
					     &led);
}

int cwmsg_parse_wlan_config_response(struct cw_ctrlmsg *msg, struct capwap_wtp *wtp)
{
	uint16_t elem_type;
	uint16_t elem_len;
	void *elem_value;
	struct wifi_info *wifi;
	int result = CW_PROTOCOL_SUCCESS;
	uint8_t radio_id, wlan_id;

	cwmsg_ctrlmsg_for_each_elem(msg, elem_type, elem_len, elem_value) {
		switch (elem_type) {
		case CW_ELEM_RESULT_CODE_CW_TYPE:
			result = cwmsg_parse_u32(elem_value);
			break;
		case CW_ELEM_IEEE80211_ASSIGNED_WTP_BSSID_CW_TYPE:
			radio_id = cwmsg_parse_u8(elem_value++);
			wlan_id = cwmsg_parse_u8(elem_value++);
			wifi = find_wifi_info(wtp, radio_id, wlan_id);
			if (wifi)
				cwmsg_parse_raw(wifi->bssid, sizeof(wifi->bssid), elem_value,
						elem_len - 2);
			break;
		}
	}
	return result;
}

int cwmsg_assemble_add_wlan(struct cw_ctrlmsg *msg, struct wifi_info *wifi, struct capwap_wtp *wtp)
{
	struct message message;
	int default_channel[WIFI_NUM] = {1, 36};
	uint16_t msg_len;

	if (!msg || !wifi || !wtp)
		return -EINVAL;

	msg_len = 24 + strlen(wifi->ssid);
	if (SUPPORT_COUNTRY_CODE(wtp->version))
		msg_len += sizeof(wifi->country_code);
	if (SUPPORT_WEAK_SIGNAL(wtp->version))
		msg_len += 10;
	message.len = 0;
	message.data = MALLOC(msg_len);
	if (!message.data)
		return -ENOMEM;

	cwmsg_put_u8(&message, wifi->radio_id);
	cwmsg_put_u8(&message, wifi->wlan_id);
	cwmsg_put_u16(&message, wifi->capability);
	cwmsg_put_u8(&message, wifi->key_index);
	cwmsg_put_u8(&message, wifi->key_status);
	cwmsg_put_u16(&message, sizeof(wifi->key));
	cwmsg_put_raw(&message, wifi->key, sizeof(wifi->key));
	cwmsg_put_raw(&message, wifi->group_tsc, sizeof(wifi->group_tsc));
	cwmsg_put_u8(&message, wifi->qos);
	cwmsg_put_u8(&message, wifi->auth_type);
	cwmsg_put_u8(&message, wifi->mac_mode);
	cwmsg_put_u8(&message, wifi->tunnel_mode);
	cwmsg_put_u8(&message, wifi->suppress_ssid);
	if (!SUPPORT_AUTO_CHANNEL(wtp->version) && (wifi->channel == 0))
		cwmsg_put_u8(&message, default_channel[wifi->radio_id]);
	else
		cwmsg_put_u8(&message, wifi->channel);
	if (SUPPORT_COUNTRY_CODE(wtp->version))
		cwmsg_put_raw(&message, wifi->country_code, sizeof(wifi->country_code));
	if (SUPPORT_WEAK_SIGNAL(wtp->version)) {
		cwmsg_put_u8(&message, wifi->weak_sta_signal_enable);
		cwmsg_put_u8(&message, wifi->prohibit_sta_signal_enable);
		cwmsg_put_u32(&message, (uint32_t)wifi->weak_sta_signal);
		cwmsg_put_u32(&message, (uint32_t)wifi->prohibit_sta_signal);
	}
	cwmsg_put_raw(&message, wifi->ssid, strlen(wifi->ssid));

	return  cwmsg_ctrlmsg_add_element(msg, CW_ELEM_IEEE80211_ADD_WLAN_CW_TYPE, &message, TLV_NOCPY);
}

int cwmsg_assemble_add_rsn(struct cw_ctrlmsg *msg, struct wifi_info *wifi, struct capwap_wtp *wtp)
{
	struct message message;
	struct tlv_box *vendor;
	int err;

	if (wifi->wpa == 0)
		return 0;

	message.data = &wifi->wpa;
	message.len = sizeof(wifi->wpa);
	vendor = tlv_box_create(SERIAL_WITH_ID);
	if (!vendor || tlv_box_put_raw(vendor, CW_ELEM_VENDOR_SPEC_PAYLOAD_WPA, &message, 0) ||
	    tlv_box_put_string(vendor, CW_ELEM_VENDOR_SPEC_PAYLOAD_WPA_PASSPHRASE, wifi->password, TLV_NOFREE)) {
		tlv_box_destroy(vendor);
		return -ENOMEM;
	    }

	err = cwmsg_put_vendor_spec(msg, vendor);
	tlv_box_destroy(vendor);
	return err;
}

int cwmsg_assemble_htmode(struct cw_ctrlmsg *msg, int band, char *htmode, struct capwap_wtp *wtp)
{
	struct message message;

	if (!SUPPORT_BANDWIDTH(wtp->version))
		return 0;

	message.len = 0;
	message.data = malloc(strlen(htmode) + 2);
	if (!message.data)
		return -ENOMEM;
	cwmsg_put_u8(&message, band);
	cwmsg_put_raw(&message, htmode, strlen(htmode));
	return cwmsg_ctrlmsg_add_element(msg, CW_ELEM_VENDOR_SPEC_HTMODE, &message, TLV_NOCPY);
}

int cwmsg_assemble_del_wlan(struct cw_ctrlmsg *msg, uint8_t band)
{
	return cwmsg_ctrlmsg_add_raw_element(msg, CW_ELEM_IEEE80211_DELETE_WLAN_CW_TYPE, sizeof(band), &band);
}

int cwmsg_assemble_mobility_domain(struct cw_ctrlmsg *msg, uint16_t domain)
{
	return cwmsg_ctrlmsg_add_raw_element(msg, CW_ELEM_MOBILITY_DOMAIN, sizeof(domain), &domain);
}

int cwmsg_assemble_ap_roam(struct cw_ctrlmsg *msg, void *payload, int len)
{
	struct message message;

	message.data = payload;
	message.len = len;
	return cwmsg_ctrlmsg_add_element(msg, CW_ELEM_AP_ROAM_LIST, &message, TLV_NOCPY);
}
