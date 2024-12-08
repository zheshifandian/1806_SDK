#ifndef _AC_IEEE80211_H_
#define _AC_IEEE80211_H_

#include <stdint.h>
#include <linux/if.h>
#include "ieee802_11_defs.h"
#include "capwap_common.h"
#include "ac_mainloop.h"
#include "rbtree.h"
#include "list.h"

#define WLAN_CAPABILITY_NUM_FIELDS 16
#define WLAN_KEY_LEN 4
#define WLAN_GROUP_TSC_LEN 6
#define CW_MSG_IEEE_ADD_WLAN_MIN_LEN 24
#define CW_MSG_IEEE_UPDATE_WLAN_MIN_LEN 23
#define CW_MSG_IEEE_STATION_LEN 13

#define MAC80211_BEACON_BODY_MANDATORY_MIN_LEN 12
#define MAC80211_MAX_PROBERESP_LEN 768

/* ++++++++ IE Frame Management ++++++++++ */
#define MGMT_FRAME_FIXED_LEN_BEACON 36
#define MGMT_FRAME_FIXED_LEN_PROBE_RESP 36
#define MGMT_FRAME_FIXED_LEN_AUTH 30
#define MGMT_FRAME_FIXED_LEN_ASSOCIATION 30
#define MGMT_FRAME_IE_FIXED_LEN 2

#define LEN_IE_FRAME_CONTROL 2
#define LEN_IE_DURATION 2
#define LEN_IE_BEACON_INT 2
#define LEN_IE_CAPABILITY 2
#define LEN_IE_LISTEN_INTERVAL 2
#define LEN_IE_SEQ_CTRL 2
#define LEN_IE_TIMESTAMP 8
#define LEN_IE_AUTH_ALG 2
#define LEN_IE_AUTH_TRANS 2
#define LEN_IE_STATUS_CODE 2
#define LEN_IE_REASON_CODE 2
#define LEN_IE_ASSOCIATION_ID 2

#define IE_TYPE_LEN 1
#define IE_SIZE_LEN 1

#define IE_TYPE_SSID 0
#define IE_TYPE_SUPP_RATES 1
#define IE_TYPE_DSSS 3
#define IE_TYPE_ERP 42
#define IE_TYPE_EXT_SUPP_RATES 50
#define IE_TYPE_BSS_MAX_IDLE_PERIOD 90

#define IE_AUTH_OPEN_SYSTEM 0

#define IE_STATUS_CODE_SUCCESS 0
#define IE_STATUS_CODE_FAILURE 1

#define CW_80211_MAX_SUPP_RATES 8

/* ++++++++ IE Frame Data ++++++++++ */
#define DATA_FRAME_FIXED_LEN_ACK 10

struct station {
	struct rb_node node;
	struct list_head list;
	struct capwap_wtp *wtp;
	uint8_t mac[ETH_ALEN];
	uint8_t bssid[ETH_ALEN];
	char if_name[IFNAMSIZ + 4]; // +4 for compatibility
	uint8_t radio_id;
	uint8_t wlan_id;
};

struct association_resp {
	uint16_t frame_control;
	uint16_t duration;
	uint8_t dst_addr[ETH_ALEN];
	uint8_t src_addr[ETH_ALEN];
	uint8_t bssid[ETH_ALEN];
	uint16_t seq_ctrl;
	uint16_t capability;
	uint16_t status_code;
	uint16_t assid;
	uint8_t sup_rates;
	uint8_t rate_len;
	uint8_t rates[0];
} __attribute__((packed));

void print_stations(void);
void station_for_each(void (*fn)(struct station *, void *), void *arg, bool write);
void capwap_wtp_delete_stations(struct capwap_wtp *wtp);

int capwap_send_delete_wlan(struct capwap_wtp *wtp, int band);
int capwap_send_add_wlan(struct capwap_wtp *wtp, int band);

void capwap_manage_80211_frame(struct capwap_wtp *wtp, void *buff, uint16_t len);
int capwap_station_online(struct capwap_wtp *wtp, void *value, uint16_t len);
int capwap_station_delete(struct capwap_wtp *wtp, void *value, uint16_t len);

int capwap_handle_wlan_response(struct capwap_wtp *wtp, int result);

#endif // _AC_IEEE80211_H_
