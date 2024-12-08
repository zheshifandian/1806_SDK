#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "capwap_message.h"
#include "network.h"
#include "CWProtocol.h"
#include "ac_mainloop.h"
#include "ac_ieee80211.h"
#include "ac_manager.h"
#include "common.h"
#include "ac_log.h"

/**
 * We use a RB-tree to save the station informations on AC.
 * Sations are sorted by their mac address, and save information of which band
 * of which WTP is the station on.
 * Each WTP also has a station list to save the stations connected in.
 */
static struct rb_root station_list = RB_ROOT;
static pthread_rwlock_t station_lock = PTHREAD_RWLOCK_INITIALIZER;

static struct station *find_station(uint8_t mac[ETH_ALEN])
{
	struct rb_root *root = &station_list;
	struct rb_node *node = root->rb_node;

	while (node) {
		struct station *sta = rb_entry(node, struct station, node);
		int result;

		result = memcmp(mac, sta->mac, ETH_ALEN);
		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return sta;
	}
	return NULL;
}

// Opencapwap don't support radio id, if our new station connected through Opencapwap,
// keep radio id unchanged.
static void station_update(struct station *old, struct station *new)
{
	if (SUPPORT_HOSTAPD(new->wtp->version))
		return;
	if (new->radio_id == -1)
		new->radio_id = old->radio_id;
}

static int station_insert(struct station *sta)
{
	struct rb_root *root = &station_list;
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	list_add_tail(&sta->list, &sta->wtp->stations);
	pthread_rwlock_rdlock(&station_lock);
	/* Figure out where to put new node */
	while (*new) {
		struct station *this = rb_entry(*new, struct station, node);
		int result = memcmp(sta->mac, this->mac, ETH_ALEN);

		parent = *new;
		if (result < 0)
			new = &((*new)->rb_left);
		else if (result > 0)
			new = &((*new)->rb_right);
		else {
			station_update(this, sta);
			list_del(&this->list);
			rb_replace_node(&this->node, &sta->node, &station_list);
			pthread_rwlock_unlock(&station_lock);
			free(this);
			return 0;
		}
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&sta->node, parent, new);
	rb_insert_color(&sta->node, root);

	pthread_rwlock_unlock(&station_lock);
	return 0;
}

/**
 * station_delete() - Delete a station according to mac, radio_id and wtp
 *
 * @sta: specific the mac, radio_id and wtp of the station need to be delete.
 *
 * Since there are a lot of APs, the rb_tree may be inserted or deleted by many threads at the
 * same time, so the search and delete operations must be atomic. There is also a chance that
 * the station connected to another AP immediately after disconnected from the previous one,
 * which we may receive the station delete request after the new connected request, and we
 * need make sure that we won't delete this station in this situation.
 *
 * Return: 1 for we have delete the station, 0 for not found or attribute not match.
 */
static bool station_delete(struct station *sta)
{
	struct station *old;
	bool deleted = false;

	pthread_rwlock_wrlock(&station_lock);
	old = find_station(sta->mac);
	if (old && old->wtp == sta->wtp && old->radio_id == sta->radio_id) {
		rb_erase(&old->node, &station_list);
		list_del(&old->list);
		free(old);
		deleted = true;
	}
	pthread_rwlock_unlock(&station_lock);
	return deleted;
}

/**
 * Delete a station from WTP's station list, also delete it from RB-tree
 */
void capwap_wtp_delete_stations(struct capwap_wtp *wtp)
{
	struct station *sta, *tmp;

	pthread_rwlock_wrlock(&station_lock);
	list_for_each_entry_safe(sta, tmp, &wtp->stations, list) {
		rb_erase(&sta->node, &station_list);
		list_del(&sta->list);
		free(sta);
	}
	pthread_rwlock_unlock(&station_lock);
}

/**
 * A helper function to iterate over the stations' RB-tree.
 * @fn: the function to called on each station. The first argument is the station struct,
 * 	the second argument is user passed in.
 * @arg: the second argument passed to fn.
 * @write: whether the function need to write the station structs. If this is true, this
 * 	function will take write lock to the whole RB-tree; otherwise it will only take
 * 	a read lock, which can allow parallel operations.
 */
void station_for_each(void (*fn)(struct station *, void *), void *arg, bool write)
{
	struct station *sta, *tmp;

	if (write)
		pthread_rwlock_wrlock(&station_lock);
	else
		pthread_rwlock_rdlock(&station_lock);
	rbtree_postorder_for_each_entry_safe(sta, tmp, &station_list, node)
		fn(sta, arg);
	pthread_rwlock_unlock(&station_lock);
}

/**
 * Destory the RB-tree
 */
void station_list_destory(void)
{
	struct station *sta, *tmp;

	pthread_rwlock_wrlock(&station_lock);
	rbtree_postorder_for_each_entry_safe(sta, tmp, &station_list, node)
		free(sta);
	pthread_rwlock_unlock(&station_lock);
}

void print_stations(void)
{
	struct station *sta, *tmp;
	char *band;

	pthread_rwlock_rdlock(&station_lock);
	rbtree_postorder_for_each_entry_safe(sta, tmp, &station_list, node) {
		if (sta->radio_id == WIFI_5G)
			band = "5G";
		else if (sta->radio_id == WIFI_2G)
			band = "2.4G";
		else if (sta->radio_id == -1)
			band = "unknown";
		CWLog(NONE, "sta:" MACSTR ", ap:%s, %s", MAC2STR(sta->mac), sta->wtp->ip_addr,
		      band);
	}
	pthread_rwlock_unlock(&station_lock);
}

/**
 * Opencapwap distinguish 2G and 5G by bssids, this function is for compatibilities.
 */
static struct wifi_info *find_wifi_info_by_bssid(struct capwap_wtp *wtp, uint8_t *bssid)
{
	int i;

	for (i = 0; i < WIFI_NUM; i++) {
		if (!memcmp(wtp->wifi[i].bssid, bssid, sizeof(wtp->wifi[i].bssid))) {
			return &wtp->wifi[i];
		}
	}
	return NULL;
}

static int cwmsg_assemble_station_cmd(struct cw_ctrlmsg *msg, uint16_t type, uint8_t radio_id, uint8_t *mac)
{
	struct message message;

	message.len = 0;
	message.data = malloc(8);
	if (!msg || !message.data)
		return -ENOMEM;

	cwmsg_put_u8(&message, radio_id);
	cwmsg_put_u8(&message, ETH_ALEN);
	cwmsg_put_raw(&message, mac, ETH_ALEN);
	return cwmsg_ctrlmsg_add_element(msg, type, &message, TLV_NOCPY);
}

static int cwmsg_assemble_80211_station(struct cw_ctrlmsg *msg, struct association_resp *assoc, uint8_t radio_id, uint8_t wlan_id)
{
	struct message message;
	uint16_t msg_len = 13 + assoc->rate_len;

	message.len = 0;
	message.data = malloc(msg_len);
	if (!msg || !message.data)
		return -ENOMEM;

	cwmsg_put_u8(&message, radio_id);
	cwmsg_put_u16(&message, assoc->assid & (~(BIT(14) | BIT(15))));
	cwmsg_put_u8(&message, 0);
	cwmsg_put_raw(&message, assoc->dst_addr, ETH_ALEN);
	cwmsg_put_u16(&message, assoc->capability);
	cwmsg_put_u8(&message, wlan_id);
	cwmsg_put_raw(&message, assoc->rates, assoc->rate_len);
	return cwmsg_ctrlmsg_add_element(msg, CW_ELEM_IEEE80211_STATION, &message, TLV_NOCPY);
}

int capwap_send_delete_wlan(struct capwap_wtp *wtp, int band)
{
	struct cw_ctrlmsg *msg;

	if (!SUPPORT_HOSTAPD(wtp->version)) {
		// For old WTPs, we don't send delete wlan command, so we won't
		// receive CONFIGURATION_RESPONSE. Mark setted here.
		wtp->wifi[band].setted = 1;
		return 0;
	}

	msg = cwmsg_ctrlmsg_new(CW_TYPE_WLAN_CONFIGURATION_REQUEST, get_seq_num(wtp));
	if (!msg || cwmsg_assemble_del_wlan(msg, band)) {
		cwmsg_ctrlmsg_destroy(msg);
		return -ENOMEM;
	}
	return capwap_send_request(wtp, msg);
}

int capwap_send_add_wlan(struct capwap_wtp *wtp, int band)
{
	struct cw_ctrlmsg *msg;
	struct wifi_info *wifi_info = &wtp->wifi[band];

	msg = cwmsg_ctrlmsg_new(CW_TYPE_WLAN_CONFIGURATION_REQUEST, get_seq_num(wtp));
	if (!msg || cwmsg_assemble_add_wlan(msg, wifi_info, wtp) ||
	    cwmsg_assemble_add_rsn(msg, wifi_info, wtp) ||
	    cwmsg_assemble_htmode(msg, band, wifi_info->ht_mode, wtp)) {
		cwmsg_ctrlmsg_destroy(msg);
		return -ENOMEM;
	}
	return capwap_send_request(wtp, msg);
}

int cwmsg_send_add_station(struct capwap_wtp *wtp, struct association_resp *assoc, struct wifi_info *wifi_info)
{
	struct cw_ctrlmsg *msg = cwmsg_ctrlmsg_new(CW_TYPE_STATION_CONFIGURATION_REQUEST, get_seq_num(wtp));

	CWLog(wtp, "Assembling add station");
	if (!msg || cwmsg_assemble_station_cmd(msg, CW_ELEM_ADD_STATION_CW_TYPE, wifi_info->radio_id, assoc->dst_addr) ||
	    cwmsg_assemble_80211_station(msg, assoc, wifi_info->radio_id, wifi_info->wlan_id))
		return -ENOMEM;
	CWLog(wtp, "Send add station");
	return capwap_send_request(wtp, msg);
}

void capwap_manage_80211_frame(struct capwap_wtp *wtp, void *buff, uint16_t len)
{
	uint16_t frame_control;
	struct association_resp *assoc;
	struct wifi_info *wifi_info;
	struct station *sta;
	int err;

	if (!buff)
		return;
	cwmsg_parse_raw(&frame_control, sizeof(frame_control), buff, sizeof(uint16_t));
	CWLog(wtp, "frame control = %x", frame_control);
	if (WLAN_FC_GET_STYPE(frame_control) == WLAN_FC_STYPE_ASSOC_RESP) {
		assoc = buff;
		wifi_info = find_wifi_info_by_bssid(wtp, assoc->bssid);
		if (!wifi_info) {
			CWWarningLog(wtp, "BSSID not match");
			return;
		}
		CWDebugLog(wtp, "Receive associate response");
		err = cwmsg_send_add_station(wtp, assoc, wifi_info);
		if (err)
			return;
		// station management
		sta = MALLOC(sizeof(*sta));
		if (!sta)
			return;
		sta->wtp = wtp;
		sta->radio_id = wifi_info->radio_id;
		memcpy(sta->mac, assoc->dst_addr, ETH_ALEN);
		station_insert(sta);
	}
}

/**
 * This is used for Parent Control.
 * Whenever a new station is online, we have to notify the Parent Control system of this station,
 * the information we need to send is mac address, interface name and online or offline.
 */
struct wifi_custom_data {
	uint8_t mac[6];
	int online;
	char ifname[20];
} __attribute__((packed));

#define CUSTOM_WIFI_LISTEN_PORT 7892
static void broadcast_sta_online(uint8_t *mac, char *ifname, uint32_t online)
{
	int sock;
	struct sockaddr_in servaddr;
	struct wifi_custom_data custom_data = {0};

	memcpy(&custom_data.mac, mac, 6);
	custom_data.online = online;
	memcpy(custom_data.ifname, ifname, strlen(ifname));

	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		CWLog(NONE, "custom wifi socket error:");
		return;
	}
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(CUSTOM_WIFI_LISTEN_PORT);
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (sendto(sock, &custom_data, sizeof(custom_data), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		CWLog(NONE, "custom wifi send error");
		goto SOCKET_ERR;
	}

SOCKET_ERR:
	close(sock);
	return;
}

int capwap_station_online(struct capwap_wtp *wtp, void *value, uint16_t value_len)
{
	struct station *sta;
	uint32_t status;
	uint8_t len;

	sta = MALLOC(sizeof(*sta));
	if (!sta)
		return -ENOMEM;

	sta->wtp = wtp;
	status = cwmsg_parse_u32(value);
	value += sizeof(uint32_t);
	len = cwmsg_parse_u8(value++);
	cwmsg_parse_raw(sta->mac, sizeof(sta->mac), value, len);
	value += len;
	if (SUPPORT_HOSTAPD(wtp->version))
		sta->radio_id = cwmsg_parse_u8(value++);
	else
		sta->radio_id = -1;
	len = cwmsg_parse_u8(value++);
	cwmsg_parse_raw(sta->if_name, sizeof(sta->if_name), value, len);

	CWLog(wtp, "station %02hhx_%02hhx_%02hhx_%02hhx_%02hhx_%02hhx online(%d)", sta->mac[0],
	      sta->mac[1], sta->mac[2], sta->mac[3], sta->mac[4], sta->mac[5], status);
	if (status) {
		station_insert(sta);
		broadcast_sta_online(sta->mac, sta->if_name, status);
	} else { //offline
		if (station_delete(sta))
			broadcast_sta_online(sta->mac, sta->if_name, status);
		free(sta);
	}

	return 0;
}

int capwap_station_delete(struct capwap_wtp *wtp, void *value, uint16_t value_len)
{
	struct station *sta;
	uint8_t len;

	sta = MALLOC(sizeof(*sta));
	if (!sta)
		return -ENOMEM;

	sta->wtp = wtp;
	sta->radio_id = cwmsg_parse_u8(value++);
	len = cwmsg_parse_u8(value++);
	cwmsg_parse_raw(sta->mac, sizeof(sta->mac), value, len);

	if (station_delete(sta)) {
		CWLog(wtp, "station %02hhx_%02hhx_%02hhx_%02hhx_%02hhx_%02hhx delete",
		      sta->mac[0], sta->mac[1], sta->mac[2], sta->mac[3], sta->mac[4], sta->mac[5]);
		broadcast_sta_online(sta->mac, sta->if_name, 0);
	} else {
		CWLog(wtp, "station %02hhx_%02hhx_%02hhx_%02hhx_%02hhx_%02hhx not delete, "
		      "maybe it's already online on another AP or already deleted.",
		      sta->mac[0], sta->mac[1], sta->mac[2], sta->mac[3], sta->mac[4], sta->mac[5]);
	}
	free(sta);

	return 0;
}

int capwap_handle_wlan_response(struct capwap_wtp *wtp, int result)
{
	// Keep in mind that 2G is send before 5G and this function will be called twice.
	if (!wtp->wifi[WIFI_2G].setted) {
		wtp->wifi[WIFI_2G].setted = 1;
	} else if (!wtp->wifi[WIFI_5G].setted) {
		wtp->wifi[WIFI_5G].setted = 1;
	}

	// If wlan set error, exit.
	if (result != CW_PROTOCOL_SUCCESS) {
		CWLog(wtp, "wlan set error with %d", result);
		wtp->attr->status = DEV_SET_ERROR;
		save_device_config(wtp->attr, 0);
		capwap_set_state(wtp, QUIT);
		return result;
	}

	// Both 2G and 5G has been setted, check if we need to update roam mac list and
	// then set status to RUN.
	if (wtp->wifi[WIFI_2G].setted && wtp->wifi[WIFI_5G].setted) {
		ac_modify_mac_list(wtp);
		capwap_set_state(wtp, RUN);
		wtp->attr->status = DEV_RUNNING;
		save_device_config(wtp->attr, 0);
	}
	return 0;
}
