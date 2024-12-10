#ifndef _WTP_H_
#define _WTP_H_

#include <sys/socket.h>
#include <event2/event.h>
#include <stdbool.h>

#include "CWProtocol.h"
#include "capwap_message.h"
#include "capwap_common.h"

#ifndef SF_UPDATE_BIN
#define SF_UPDATE_BIN "/usr/sbin/wtp_do_update"
#endif
#ifndef SF_DOWNLOAD_BIN
#define SF_DOWNLOAD_BIN "/usr/sbin/wtp_do_download"
#endif

#define WTP_CONTROL_PORT 12225

enum ap_state {
	CW_DISCOVERY,
	CW_JOIN,
	CW_CONF,
	CW_DATA_CHECK,
	RUN,	// idle run
	SET,	// config set or wifi setting
	CMD,	// executing command
	WAIT,	// we have send a request and waiting for response
	UPDATE,	// update firmware
	SULK,	// sleep and exit
	QUIT,	// exit
	AP_ROAM,// update ap roam list
};

struct hostapd_config;

/**
 * A struct represents this wtp, it saves the current state of the wtp.
 *
 * @ac_addr: AC address struct, this is the socket address to be used with sockets apis
 * @ac_addr_len: AC address length, for socket api usage.
 * @base: event base, all the events are added to this base.
 * @ctrl_ev: this event represents the network socket fd which communicates with AC.
 * @sig_ev: used to receive led event, which will be triggered when user press the button.
 * @cmd_ev: when AC send a wtp command request, this event will be created to collect the
 * 	    command's output and termination.
 * @cmd_file: After receive a wtp command request, use this struct to store popen()'s return value.
 * @timeout: the interval value of echo request.
 * @ctrl_sock: socket fd to communicate with AC.
 * @hostapd: Hostapd global interface
 * @phy_name: Phy name
 * @ap_ev: struct wpa_ctrl monitor event
 * @config: necessary configs for each wifi
 * @request: Used to send command to wlan interface
 * @monitor: Used to receive wlan interface messages
 * @update: a helper struct to store update informations
 * @board_data: to store necessary informations for BOARD_DATA element
 * @descriptor: to store necessary informations for DESCRIPTOR element
 * @vendor: to store necessary informations for VENDOR element
 * @state: current state of this wtp
 * @wait: whether is waiting for a response
 * @req: used to save request messages. If the previous request message was lost
 * 	and need to retransfer, we can use this to retransfer. Remember free this
 * 	after receive the response.
 * @version: this wtp software's version
 * @ac_version: AC software version
 * @resp_type: used to store expected response type
 * @session_id: random 16 bytes data, for Opencapwap compatibility
 * @seq_num: For CAPWAP control messages that need to be retransmitted, incremented in order.
 * @resp_seq_num: store received message's seq_num, and will be used as the response message's seq_num.
 * @retry_count: count retransfer times.
 * @led_status: led status, 1 for on, 0 for off.
 * @discovery_time: intervals for retransfer discovery request.
 * @max_discoveries: max retransfer count for discovery request, exit after reach this limit.
 */
struct capwap_ap {
	struct sockaddr_storage ac_addr;
	int ac_addr_len;
	struct event_base *base;
	struct event *ctrl_ev;
	struct event *sig_ev;
	struct event *cmd_ev;
	FILE *cmd_file;
	struct timeval timeout;
	int ctrl_sock;
	struct wpa_ctrl *hostapd;
	char *phy_name[WIFI_NUM];
	struct event *ap_ev[WIFI_NUM];
	struct hostapd_config *config[WIFI_NUM];
	struct wpa_ctrl *request[WIFI_NUM];
	struct wpa_ctrl *monitor[WIFI_NUM];

	struct ap_update update;
	struct cw_wtp_board_data board_data;
	struct cw_wtp_descriptor descriptor;
	struct cw_wtp_vendor_spec vendor;
	enum ap_state state;
	bool wait;
	struct cw_ctrlmsg *req;
	uint32_t version;
	uint32_t ac_version;
	uint32_t resp_type;
	uint8_t session_id[CW_SESSION_ID_LENGTH];
	uint8_t seq_num;
	uint8_t resp_seq_num;
	uint8_t retry_count;
	uint8_t led_status;
	uint8_t discovery_time;
	uint8_t max_discoveries;
};

/**
 * Increase seq_num with 1, and return it.
 * Note: Increase seq_num before get because we need seq_num to compare
 * with the response's seq_num, and this function should only be caled
 * when send request messages.
 */
uint8_t wtp_get_seq_num(struct capwap_ap *ap);
/**
 * Send a capwap request, add a timer to start waiting for response.
 * @msg: Serialize the message and send it, so just put tlv struct in this
 * 	message in enough before calling this function.
 */
int wtp_send_request(struct capwap_ap *ap, struct cw_ctrlmsg *msg);
/**
 * A wrapper function to send response with only RESULT_CODE element in it
 * @type: the response message type
 * @result: result code
 */
int wtp_send_result_code(struct capwap_ap *ap, uint32_t type, int result);

#endif // _WTP_H_
