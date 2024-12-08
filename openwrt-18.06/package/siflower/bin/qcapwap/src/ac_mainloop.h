#ifndef _AC_MAINLOOP_H_
#define _AC_MAINLOOP_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/un.h>
#include <event2/event.h>

#include "capwap_message.h"
#include "CWProtocol.h"
#include "uci_config.h"
#include "capwap_common.h"
#include "ac_interface.h"

enum wtp_state {
	IDLE,
	DISCOVERY,
	JOIN,
	CONFIGURE,
	IMAGE_DATA,
	DATA_CHECK,
	AP_ROAM,
	AP_ROAM_UPDATE,
	RUN,
	SET,
	UPDATE,
	RESET,
	QUIT,
	SULK,
};

/**
 * This structure represents the whole AC.
 * @wtp_list: every struct capwap_wtp is listed in this list, to iterate over all the wtps.
 * @if_path: AC unix socket path
 * @base: event_base for AC main thread.
 * @ctrl_ev: this listens for discovery request.
 * @interface: for WUM commands.
 */
struct capwap_ac {
	struct list_head wtp_list;
	char if_path[32];
	struct event_base *base;
	struct event *ctrl_ev;
	struct interface interface;
};

/**
 * This structure should have all the information we need for the wtp.
 * We don't hold message-format elements, instead we use the data parsed from the message.
 */
struct capwap_wtp {
	struct sockaddr_storage ctrl_addr;
	struct sockaddr_storage data_addr;
	struct sockaddr_un wum_addr;
	struct wtp_client *wtp_client;
	char ip_addr[INET_ADDRMAXLEN];
	char if_path[32];
	int wtp_addr_len;
	int ctrl_sock;
	int data_sock;
	struct timeval ctrl_tv;
	struct event_base *ev_base;
	struct event *ctrl_ev;
	struct event *data_ev;
	struct event *timer_ev;
	struct event *ac_ev;
	struct interface interface;
	struct list_head stations;

	bool wait;
	struct cw_ctrlmsg *req;
	uint32_t resp_type;
	uint8_t retry_count;

	enum wtp_state state;
	char *location;
	char *name;
	struct cw_wtp_board_data board_data;
	struct cw_wtp_descriptor descriptor;
	struct cw_wtp_vendor_spec vendor_spec;
	uint8_t session_id[CW_SESSION_ID_LENGTH];
	uint32_t version;
	struct wifi_info wifi[WIFI_NUM];

	struct device_attr *attr;
	uint8_t seq_num;
	uint8_t expect_seq_num;
	uint8_t resp_seq_num;
};

void capwap_set_state(struct capwap_wtp *wtp, enum wtp_state state);
// void capwap_reinit(struct capwap_wtp *wtp);
int capwap_send_request(struct capwap_wtp *wtp, struct cw_ctrlmsg *msg);
int capwap_do_send_request(struct capwap_wtp *wtp, struct cw_ctrlmsg *msg);

void set_ap_idle_timeout(struct capwap_wtp *wtp, uint8_t timeout);
uint8_t get_echo_interval(struct capwap_wtp *wtp);
uint32_t get_idle_timeout(struct capwap_wtp *wtp);
uint8_t get_seq_num(struct capwap_wtp *wtp);
struct wifi_info *find_wifi_info(struct capwap_wtp *wtp, uint8_t radio_id, uint8_t wlan_id);

int capwap_discovery_state(int sock, struct cw_ctrlhdr *ctrl_hdr, struct wtp_client *addr, struct capwap_ac *ac);
int capwap_idle_to_join(struct capwap_wtp *wtp, struct cw_ctrlmsg *join_req);
int capwap_join_to_configure(struct capwap_wtp *wtp, struct cw_ctrlmsg *con_req);
int capwap_configure_to_data_check(struct capwap_wtp *wtp, struct cw_ctrlmsg *data_check_req);
int capwap_run(struct capwap_wtp *wtp, struct cw_ctrlmsg *ctrlmsg);

void capwap_data_channel(evutil_socket_t sock, short what, void *arg);

int cwmsg_assemble_ac_version(struct cw_ctrlmsg *msg, struct capwap_wtp *wtp);
int cwmsg_assemble_add_wlan(struct cw_ctrlmsg *msg, struct wifi_info *wifi, struct capwap_wtp *wtp);
int cwmsg_assemble_add_rsn(struct cw_ctrlmsg *msg, struct wifi_info *wifi, struct capwap_wtp *wtp);
int cwmsg_assemble_htmode(struct cw_ctrlmsg *msg, int band, char *htmode, struct capwap_wtp *wtp);
int cwmsg_assemble_del_wlan(struct cw_ctrlmsg *msg, uint8_t band);

#endif // _AC_MAINLOOP_H_
