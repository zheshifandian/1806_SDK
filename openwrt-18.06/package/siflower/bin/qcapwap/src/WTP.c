#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <event2/event.h>

#include "capwap_common.h"
#include "CWProtocol.h"
#include "capwap_message.h"
#include "wtp_compat_message.h"
#include "network.h"
#include "wtp.h"
#include "wtp_hostapd.h"
#include "config_file.h"
#include "CWLog.h"

int gLoggingLevel = DEFAULT_LOGGING_LEVEL;
int gEnabledLog = 1;

uint8_t wtp_get_seq_num(struct capwap_ap *ap)
{
	ap->seq_num++;
	return ap->seq_num;
}

static int get_led() {
	char buff[8] = {0};

	if (cat_simple_file("/sys/class/leds/led1/brightness", buff, sizeof(buff)))
		return -1;

	return atoi(buff);
}

static int get_update()
{
	char *update = "/tmp/update_status";
	char buff[8] = {0};

	if (cat_simple_file(update, buff, sizeof(buff)))
		return 0;
	return atoi(buff);
}

static int wtp_send_discovery_request(struct capwap_ap *ap)
{
	struct sockaddr board_addr;
	struct cw_ctrlmsg *msg;
	int err;

	if (get_boardcast_addr(NETWORK_IF, &board_addr)) {
		CWLog("Can't get boardcast address");
		return -ENETUNREACH;
	}
	sock_set_port_cw((void *)&board_addr, CW_CONTROL_PORT);
	msg = cwmsg_ctrlmsg_new(CW_TYPE_DISCOVERY_REQUEST, wtp_get_seq_num(ap));
	if (!msg || cwmsg_assemble_wtp_descriptor(msg, &ap->descriptor) ||
	    cwmsg_assemble_board_data(msg, &ap->board_data))
		return -ENOMEM;
	err = capwap_send_ctrl_message_unconnected(ap->ctrl_sock, msg, (void *)&board_addr, sizeof(struct sockaddr_in));
	cwmsg_ctrlmsg_destroy(msg);
	return err;
}

int wtp_send_request(struct capwap_ap *ap, struct cw_ctrlmsg *msg)
{
	struct timeval t = {2, 0};
	int err;

	if (!msg)
		return -EINVAL;
	err = capwap_send_ctrl_message(ap->ctrl_sock, msg);
	if (!err) {
		ap->wait = true;
		ap->resp_type = cwmsg_ctrlmsg_get_type(msg) + 1;
		ap->req = msg;
		// Reset timeout after send a request
		event_add(ap->ctrl_ev, &t);
	}
	return err;
}

static int wtp_resend(struct capwap_ap *ap)
{
	struct timeval t = {2, 0};
	ap->retry_count++;

	if (ap->state == CW_DISCOVERY) {
		event_add(ap->ctrl_ev, &ap->timeout);
		return wtp_send_discovery_request(ap);
	}

	if (!ap || !ap->wait || !ap->req || !ap->resp_type)
		return -EINVAL;
	event_add(ap->ctrl_ev, &t);
	return capwap_send_ctrl_message(ap->ctrl_sock, ap->req);
}

static int wtp_send_control_request(struct capwap_ap *ap, uint16_t type)
{
	struct cw_ctrlmsg *msg;

	msg = cwmsg_ctrlmsg_new(type, wtp_get_seq_num(ap));
	if (!msg)
		return -ENOMEM;

	return wtp_send_request(ap, msg);
}

static int wtp_parse_discovery_response(struct capwap_ap *ap, struct cw_ctrlmsg *msg)
{
	uint16_t elem_type;
	uint16_t elem_len;
	void *elem_value;
	struct sockaddr_in ac_addr = {0};
	struct cw_elem_ipv4_addr addr = {0};

	cwmsg_ctrlmsg_for_each_elem (msg, elem_type, elem_len, elem_value) {
		switch (elem_type) {
			case CW_ELEM_CW_CONTROL_IPV4_ADDRESS_CW_TYPE:
			cwmsg_parse_raw(&addr, sizeof(addr), elem_value, elem_len);
			ac_addr.sin_family = AF_INET;
			ac_addr.sin_addr.s_addr = addr.addr;
			ac_addr.sin_port = htons(CW_CONTROL_PORT);
			if (connect(ap->ctrl_sock, (void *)&ac_addr, sizeof(ac_addr)) < 0)
				return -errno;
			break;
		}
	}
	return 0;
}

static int wtp_send_join_request(struct capwap_ap *ap)
{
	struct cw_ctrlmsg *msg;
	uint32_t version = htonl(ap->version);

	ap->vendor.led_status = get_led();
	msg = cwmsg_ctrlmsg_new(CW_TYPE_JOIN_REQUEST, wtp_get_seq_num(ap));
	if (!msg || cwmsg_assemble_board_data(msg, &ap->board_data) ||
	    cwmsg_assemble_wtp_descriptor(msg, &ap->descriptor) ||
	    cwmsg_assemble_string(msg, CW_ELEM_LOCATION_DATA_CW_TYPE, "Wall", TLV_NOFREE) ||
	    cwmsg_assemble_string(msg, CW_ELEM_WTP_NAME_CW_TYPE, "86AP", TLV_NOFREE) ||
	    cwmsg_assemble_wtp_ipv4_addr(msg) ||
	    cwmsg_assemble_radio_info(msg) ||
	    cwmsg_assemble_frame_tunnel_mode(msg) ||
	    cwmsg_assemble_vendor_spec(msg, &ap->vendor) ||
	    cwmsg_ctrlmsg_add_raw_element(msg, CW_ELEM_VENDOR_SPEC_WTP_VERSION, sizeof(ap->version), &version) ||
	    cwmsg_ctrlmsg_add_raw_element(msg, CW_ELEM_SESSION_ID_CW_TYPE, sizeof(ap->session_id), ap->session_id)) {
		cwmsg_ctrlmsg_destroy(msg);
		return -ENOMEM;
	}
	return wtp_send_request(ap, msg);
}

static int wtp_parse_join_response(struct capwap_ap *ap, struct cw_ctrlmsg *msg)
{
	uint16_t elem_type;
	uint16_t elem_len __attribute__((unused));
	void *elem_value;
	uint32_t result;

	cwmsg_ctrlmsg_for_each_elem (msg, elem_type, elem_len, elem_value) {
		switch (elem_type) {
		case CW_ELEM_RESULT_CODE_CW_TYPE:
			result = cwmsg_parse_u32(elem_value);
			break;
		case CW_ELEM_VENDOR_SPEC_AC_VERSION:
			ap->ac_version = cwmsg_parse_u32(elem_value);
			break;
		}
	}
	return result == CW_PROTOCOL_SUCCESS ? 0 : -EINVAL;
}

static int wtp_send_config_request(struct capwap_ap *ap)
{
	struct cw_ctrlmsg *msg;
	uint8_t radio[WIFI_NUM] = {WIFI_2G, WIFI_5G};

	if (!SUPPORT_HOSTAPD(ap->ac_version))
		return cwmsg_send_configure_request(ap);

	msg = cwmsg_ctrlmsg_new(CW_TYPE_CONFIGURE_REQUEST, wtp_get_seq_num(ap));
	if (!msg || cwmsg_ctrlmsg_add_raw_element(msg, CW_ELEM_IEEE80211_MULTI_DOMAIN_CAPABILITY_CW_TYPE,
						  sizeof(radio), radio))
		return -ENOMEM;

	return wtp_send_request(ap, msg);
}

static int wtp_parse_configure_response(struct capwap_ap *ap, struct cw_ctrlmsg *msg)
{
	uint16_t elem_type;
	uint16_t elem_len __attribute__((unused));
	void *elem_value;
	uint32_t idle_timeout __attribute__((unused));

	cwmsg_ctrlmsg_for_each_elem (msg, elem_type, elem_len, elem_value) {
		switch (elem_type) {
		case CW_ELEM_CW_TIMERS_CW_TYPE:
			ap->discovery_time = cwmsg_parse_u8(elem_value++);
			ap->timeout.tv_sec = cwmsg_parse_u8(elem_value);
			break;
		case CW_ELEM_IDLE_TIMEOUT_CW_TYPE:
			idle_timeout = cwmsg_parse_u32(elem_value);
			break;
		case CW_ELEM_WTP_FALLBACK_CW_TYPE:
			ap->led_status = cwmsg_parse_u8(elem_value);
			LED_SAVE_STATUS(ap->led_status);
			break;
		}
	}
	return 0;
}

int wtp_send_change_state_request(struct capwap_ap *ap)
{
	struct cw_ctrlmsg *msg;

	msg = cwmsg_ctrlmsg_new(CW_TYPE_CHANGE_STATE_EVENT_REQUEST, wtp_get_seq_num(ap));
	if (!msg || cwmsg_assemble_result_code(msg, CW_PROTOCOL_SUCCESS))
		return -ENOMEM;

	return wtp_send_request(ap, msg);
}

int wtp_send_echo_request(struct capwap_ap *ap)
{
	return wtp_send_control_request(ap, CW_TYPE_ECHO_REQUEST);
}

// Can only be used with response
int wtp_send_result_code(struct capwap_ap *ap, uint32_t type, int result)
{
	struct cw_ctrlmsg *msg;

	msg = cwmsg_ctrlmsg_new(type, ap->resp_seq_num);
	if (!msg || cwmsg_assemble_result_code(msg, result))
		return -ENOMEM;

	return capwap_send_response(ap->ctrl_sock, msg);
}

void wtp_send_reset_response(struct capwap_ap *ap)
{
	wtp_send_result_code(ap, CW_TYPE_RESET_RESPONSE, CW_PROTOCOL_SUCCESS);
}

void wtp_send_command_string(struct capwap_ap *ap, char *str)
{
	struct cw_ctrlmsg *msg;

	msg = cwmsg_ctrlmsg_new(CW_TYPE_WTP_EXECUTE_COMMAND_RESPONSE, ap->resp_seq_num);
	if (!msg || cwmsg_ctrlmsg_add_raw_element(msg, CW_ELEM_RESULT_STRING, strlen(str) + 1, str))
		return;
	capwap_send_response(ap->ctrl_sock, msg);
	return;
}

void wtp_send_command_result(struct capwap_ap *ap, int result)
{
	wtp_send_result_code(ap, CW_TYPE_WTP_EXECUTE_COMMAND_RESPONSE, result);
}

static void wtp_send_download_event(struct capwap_ap *ap, int result)
{
	struct cw_ctrlmsg *msg;

	if (!SUPPORT_NEW_UPDATE(ap->ac_version))
		return;

	result = htonl(result);
	msg = cwmsg_ctrlmsg_new(CW_TYPE_WTP_EVENT_REQUEST, wtp_get_seq_num(ap));
	if (!msg || cwmsg_ctrlmsg_add_raw_element(msg, CW_ELEM_VENDOR_SPEC_DOWNLOAD_STATUS, 4, &result)) {
		cwmsg_ctrlmsg_destroy(msg);
		return;
	}
	wtp_send_request(ap, msg);
}

void wtp_send_update_response(struct capwap_ap *ap, int result)
{
	if (result) {
		FREE(ap->update.md5);
		FREE(ap->update.path);
		FREE(ap->update.version);
	}
	wtp_send_result_code(ap, CW_TYPE_CONFIGURE_UPDATE_RESPONSE, result);
}

static void wtp_handle_update_cmd(struct capwap_ap *ap, int err)
{
	if (err) {
		ap->state = RUN;
		WTP_SAVE_STATUS("RUN");
	}
	if (SUPPORT_NEW_UPDATE(ap->ac_version)) {
		wtp_send_download_event(ap, err);
	} else {
		wtp_send_update_response(ap, err);
		if (!err) {
			execlp(SF_UPDATE_BIN, SF_UPDATE_BIN, ap->update.version, NULL);
			CWLog("Execute update bin failed");
		}
	}
}

void wtp_handle_sub_cmd(evutil_socket_t fd, short what, void *arg)
{
	struct capwap_ap *ap = arg;
	char buff[128];
	int err = 0;

	if (what & EV_READ) {
		if (fgets(buff, sizeof(buff), ap->cmd_file)) {
			// This is the only case we should send command_string
			if ((ap->state == CMD) && SUPPORT_HOSTAPD(ap->ac_version)) {
				// CWLog("send command string:%s", buff);
				wtp_send_command_string(ap, buff);
				return;
			} else {
				CWLog("got command output:%s", buff);
			}
		}
	}
	if (what & EV_TIMEOUT) {
		// TODO: kill subprogress
		CWLog("wtp command timeout");
		err = -ETIMEDOUT;
	} else if (ap->cmd_file) {
		err = pclose(ap->cmd_file);
		err = WEXITSTATUS(err);
	} else {
		err = -EFAULT;
	}

	ap->cmd_file = NULL;
	CWLog("send command result:%d", err);
	if (ap->state == UPDATE)
		wtp_handle_update_cmd(ap, err);
	else{
		ap->state = RUN;
		wtp_send_command_result(ap, err);
	}

	event_free(ap->cmd_ev);

	if (!SUPPORT_NEW_UPDATE(ap->ac_version))
		ap->state = RUN;
}

int wtp_execute_command(struct capwap_ap *ap, char *command)
{
	FILE *file;
	struct timeval timeout = {300, 0};

	CWLog("execute:%s", command);
	file = popen(command, "r");
	if (!file)
		return -EINVAL;
	ap->cmd_ev = event_new(ap->base, fileno(file), EV_READ | EV_PERSIST, wtp_handle_sub_cmd, ap);
	event_add(ap->cmd_ev, &timeout);
	ap->cmd_file = file;
	return 0;
}

int wtp_exec_command_request(struct capwap_ap *ap, struct cw_ctrlmsg *msg)
{
	uint16_t elem_type;
	uint16_t elem_len;
	void *elem_value;
	struct tlv_box *box;
	struct tlv *cmd_tlv;
	char *command;
	int err = 0;

	box = tlv_box_create(SERIAL_WITH_ID);
	if (!box)
		return -ENOMEM;
	cwmsg_ctrlmsg_for_each_elem(msg, elem_type, elem_len, elem_value) {
		switch (elem_type) {
		case CW_ELEM_VENDOR_SPEC_PAYLOAD_BW_CW_TYPE:
			tlv_box_parse(box, elem_value, elem_len);
			break;
		}
	}
	cmd_tlv = tlv_box_find_type(box, CW_ELEM_VENDOR_SPEC_PAYLOAD_WTP_CMD);
	command = MALLOC(cmd_tlv->length + 1);
	memcpy(command, cmd_tlv->value, cmd_tlv->length);
	err = wtp_execute_command(ap, command);
	if (!err)
		ap->state = CMD;
	tlv_box_destroy(box);
	free(command);
	return err;
}

int wtp_do_download(struct capwap_ap *ap)
{
	char *command;
	int err;

	if (!ap->update.path || !ap->update.md5 || !ap->update.version)
		return -EINVAL;
	command = malloc(512);
	if (!command)
		return -ENOMEM;
	snprintf(command, 512, SF_DOWNLOAD_BIN" %s %s", ap->update.path, ap->update.md5);
	err = wtp_execute_command(ap, command);
	free(command);
	return err;
}

int wtp_parse_config_update(struct capwap_ap *ap, struct cw_ctrlmsg *msg)
{
	uint16_t elem_type;
	uint16_t elem_len;
	void *elem_value;
	struct tlv_box *vendor;
	uint32_t idle_timeout __attribute__((unused));
	int err = 0;

	cwmsg_ctrlmsg_for_each_elem (msg, elem_type, elem_len, elem_value) {
		switch (elem_type) {
		case CW_ELEM_CW_TIMERS_CW_TYPE:
			ap->discovery_time = cwmsg_parse_u8(elem_value++);
			ap->timeout.tv_sec = cwmsg_parse_u8(elem_value);
			break;
		case CW_ELEM_IDLE_TIMEOUT_CW_TYPE:
			idle_timeout = cwmsg_parse_u32(elem_value);
			break;
		case CW_ELEM_VENDOR_SPEC_PAYLOAD_CW_TYPE:
			err = cwmsg_parse_ap_update(&ap->update, elem_value, elem_len);
			if (err)
				return err;
			ap->state = UPDATE;
			break;
		case CW_ELEM_VENDOR_SPEC_PAYLOAD_BW_CW_TYPE:
			vendor = tlv_box_create(SERIAL_WITH_ID);
			if (!vendor || tlv_box_parse(vendor, elem_value, elem_len)) {
				tlv_box_destroy(vendor);
				return -ENOMEM;
			}
			tlv_box_for_each_tlv (vendor, elem_type, elem_len, elem_value) {
				switch (elem_type) {
				case CW_ELEM_VENDOR_SPEC_PAYLOAD_LED:
					ap->led_status = cwmsg_parse_u8(elem_value);
					LED_SAVE_STATUS(ap->led_status);
					break;
				case CW_ELEM_VENDOR_SPEC_PAYLOAD_CLIENT_ALIVE:
				default:
					break;
				}
			}
			tlv_box_destroy(vendor);
		}
	}
	return 0;
}

static int wtp_handle_packet(struct capwap_ap *ap, struct cw_ctrlmsg *msg)
{
	uint32_t type = cwmsg_ctrlmsg_get_type(msg);
	int err;

	CWLog("receive %d message", type);
	if (ap->wait && type == ap->resp_type) {
		if (cwmsg_ctrlmsg_get_seqnum(msg) != ap->seq_num)
			return -EBADMSG;
		CWLog("stop retransfer");
		ap->wait = false;
		cwmsg_ctrlmsg_destroy(ap->req);
		ap->req = NULL;
		ap->resp_type = 0;
		ap->retry_count = 0;
	} else if (ap->wait && ((type & 0x1) == 0) && type != ap->resp_type) {
		// Received a different response, ignore it
		return -EBADMSG;
	}
	switch (type) {
	case CW_TYPE_ECHO_RESPONSE:
		// Do not trigger state machine when receive echo response
		return -EBADMSG;
	case CW_TYPE_AP_ROAM_REQUEST:
		err = wtp_handle_ap_roam(ap, msg);
		wtp_send_result_code(ap, CW_TYPE_AP_ROAM_RESPONSE, err);
		break;
	case CW_TYPE_CONFIGURE_UPDATE_REQUEST:
		err = wtp_parse_config_update(ap, msg);
		if (err || ap->state == RUN)
			wtp_send_result_code(ap, CW_TYPE_CONFIGURE_UPDATE_RESPONSE, err);
		break;
	case CW_TYPE_RESET_REQUEST:
		ap->state = SULK;
		wtp_send_reset_response(ap);
		break;
	case CW_TYPE_WLAN_CONFIGURATION_REQUEST:
		if (wtp_handle_wlan_request(ap, msg))
			ap->state = QUIT;
		break;
	case CW_TYPE_WTP_EVENT_RESPONSE:
		if (ap->state == UPDATE) {
			execlp(SF_UPDATE_BIN, SF_UPDATE_BIN, ap->update.version, NULL);
			CWLog("Execute update bin failed");
		}
		break;
	case CW_TYPE_WTP_EXECUTE_COMMAND_REQUEST:
		err = wtp_exec_command_request(ap, msg);
		if (err)
			wtp_send_command_result(ap, err);
		break;
	case CW_TYPE_DISCOVERY_RESPONSE:
		if (ap->state != CW_DISCOVERY)
			break;
		if (wtp_parse_discovery_response(ap, msg))
			return -EINVAL;
		ap->state = CW_JOIN;
		break;
	case CW_TYPE_JOIN_RESPONSE:
		if (ap->state != CW_JOIN)
			break;
		if (wtp_parse_join_response(ap, msg))
			return -EINVAL;
		ap->state = CW_CONF;
		break;
	case CW_TYPE_CONFIGURE_RESPONSE:
		if (ap->state != CW_CONF)
			break;
		if (wtp_parse_configure_response(ap, msg))
			return -EINVAL;
		ap->state = CW_DATA_CHECK;
		break;
	case CW_TYPE_CHANGE_STATE_EVENT_RESPONSE:
		if (SUPPORT_AP_ROAM(ap->ac_version))
			ap->state = AP_ROAM;
		else
			ap->state = RUN;
		break;
	}
	return 0;
}

static bool wtp_retransfer_exceed(struct capwap_ap *ap)
{
	if (ap->state == CW_DISCOVERY)
		return ap->retry_count >= ap->max_discoveries;

	return ap->retry_count > 3;
}

// This function is only needed in switch(ap->state) block in wtp_main_loop().
static void wtp_set_state(struct capwap_ap *ap, enum ap_state state)
{
	ap->state = state;
	event_active(ap->ctrl_ev, EV_WRITE, 0);
}

/**
 * This function will be called whenever a new packet is received in capwap control port,
 * or a timeout occurs to retransfer requests.
 */
void wtp_main_loop(evutil_socket_t fd, short what, void *arg)
{
	uint8_t *buff;
	int buff_len = 4 * 1024;
	struct capwap_ap *ap = arg;
	struct cw_ctrlmsg *ctrlmsg = NULL;
	int msg_len;
	int err = 0;

	if (what & EV_READ) {
		buff = malloc(buff_len);
		if (!buff)
			return;
		msg_len = capwap_recv_ctrl_message(ap->ctrl_sock, buff, buff_len);
		if (msg_len <= 0) {
			CWWarningLog("receive error");
			free(buff);
			return;
		}
		ctrlmsg = cwmsg_ctrlmsg_malloc();
		err = cwmsg_ctrlmsg_parse(ctrlmsg, buff, msg_len);
		if (err) {
			free(buff);
			cwmsg_ctrlmsg_free(ctrlmsg);
			CWWarningLog("Capwap message parse error(%d)", err);
			return;
		}
		ap->resp_seq_num = cwmsg_ctrlmsg_get_seqnum(ctrlmsg);
		// deal with received messages, save necessary data from the message,
		// and then change ap->state for further process in state machine below.
		err = wtp_handle_packet(ap, ctrlmsg);
		cwmsg_ctrlmsg_free(ctrlmsg);
		free(buff);
		// reset timeout
		event_add(ap->ctrl_ev, &ap->timeout);
		if (err == -EBADMSG)
			return;
		else if (err)
			ap->state = QUIT;
	} else if (what & EV_TIMEOUT) {
		if (!ap->wait && (ap->state == RUN || ap->state == UPDATE)) {
			// We are not waiting for packets, this means we are idle,
			// send echo request to keep alive.
			if (wtp_send_echo_request(ap))
				ap->state = QUIT;
			else
				return;
		} else if (ap->wait) {
			if (wtp_retransfer_exceed(ap)) {
				ap->state = QUIT;
			} else {
				wtp_resend(ap);
				CWLog("Retransfer %d", ap->retry_count);
				return;
			}
		} else {
			ap->state = QUIT;
		}
	}

	CWLog("ap->state = %d", ap->state);
	switch (ap->state) {
	case RUN:
		WTP_SAVE_STATUS("run");
		break;
	case CW_DISCOVERY:
		if (wtp_send_discovery_request(ap))
			wtp_set_state(ap, QUIT);
		else {
			ap->wait = true;
			ap->resp_type = CW_TYPE_DISCOVERY_RESPONSE;
		}
		break;
	case CW_JOIN:
		if (wtp_send_join_request(ap))
			wtp_set_state(ap, QUIT);
		break;
	case CW_CONF:
		if (wtp_send_config_request(ap))
			wtp_set_state(ap, QUIT);
		break;
	case CW_DATA_CHECK:
		if (wtp_send_change_state_request(ap))
			wtp_set_state(ap, QUIT);
		break;
	case UPDATE:
		WTP_SAVE_STATUS("update");
		err = wtp_do_download(ap);
		if (SUPPORT_NEW_UPDATE(ap->ac_version) || err)
			wtp_send_update_response(ap, err);
		if (err) {
			wtp_set_state(ap, RUN);
			WTP_SAVE_STATUS("run");
		}
		break;
	case SULK:
		sleep(3);
	case QUIT:
		CWLog("wtp QUIT");
		WTP_SAVE_STATUS("quit");
		event_base_loopbreak(ap->base);
		break;
	default:
		return;
	}
}

void wtp_handle_signal(evutil_socket_t fd, short what, void *arg)
{
	struct capwap_ap *ap = arg;
	struct cw_ctrlmsg *msg;
	uint32_t led;

	// get led status by read /sys/class/leds/led1/brightness
	led = get_led();
	LED_SAVE_STATUS(led);
	CWLog("Got signal led change to %d", led);
	led = htonl(led);
	// tell AC the led status
	msg = cwmsg_ctrlmsg_new(CW_TYPE_WTP_EVENT_REQUEST, wtp_get_seq_num(ap));
	if (!msg || cwmsg_ctrlmsg_add_raw_element(msg, CW_ELEM_VENDOR_SPEC_PAYLOAD_CW_TYPE, 4, &led)) {
		cwmsg_ctrlmsg_destroy(msg);
		return;
	}
	if (wtp_send_request(ap, msg) < 0)
		ap->state = QUIT;
}

static int wtp_init(struct capwap_ap *ap)
{
	int i;

	srandom(time(NULL));
	for (i = 0; i < CW_SESSION_ID_LENGTH; i++)
		ap->session_id[i] = (uint8_t)random();
	ap->version = WTP_VERSION;
	ap->vendor.led_status = get_led();
	ap->vendor.update_status = get_update();
	ap->discovery_time = 1;
	ap->max_discoveries = 10;

	ap->descriptor.encryp_wbid = 1;
	ap->descriptor.num_encrypt = 1;
	ap->descriptor.radio_in_use = WIFI_NUM;
	if (get_model(ap->board_data.model, sizeof(ap->board_data.model)))
		strncpy(ap->board_data.model, MODULE_VERSION, sizeof(ap->board_data.model));
	if (get_hardware(ap->descriptor.hardware_version, sizeof(ap->descriptor.hardware_version)))
		strncpy(ap->descriptor.hardware_version, HARDWARE_VERSION, sizeof(ap->descriptor.hardware_version));
	if (get_version(ap->descriptor.software_version, sizeof(ap->descriptor.software_version)))
		strncpy(ap->descriptor.software_version, OPENWRT_VERSION, sizeof(ap->descriptor.software_version));
	if (get_mac_addr(NETWORK_IF, ap->board_data.mac))
		return -EINVAL;
	return 0;
}

int main(int argc, char const *argv[])
{
	struct capwap_ap *ap;
	int port = 0, yes = 1;

	CWLog("New WTP start");
	ap = MALLOC(sizeof(struct capwap_ap));
	if (!ap) {
		CWLog("No memory for new ap");
		return -1;
	}
	if (argc == 3) {
		/**
		 * argv[1]: local port
		 * argv[2]: AC address in string format
		 * This was used for starting by Opencapwap after JOIN state, but was then abandoned due to
		 * add compatibility for Opencapwap's discovery response and join response.
		 */
		ap->ac_addr_len = sizeof(ap->ac_addr);
		port = atoi(argv[1]);
		if (evutil_parse_sockaddr_port(argv[2], (struct sockaddr *)&ap->ac_addr, &ap->ac_addr_len)) {
			CWLog("Invalid IP address");
			return -1;
		}
		ap->ctrl_sock = capwap_init_socket(port, &ap->ac_addr, ap->ac_addr_len);
		ap->state = CW_CONF;
	} else {
		ap->ctrl_sock = capwap_init_socket(WTP_CONTROL_PORT, NULL, 0);
		ap->state = CW_DISCOVERY;
	}
	if (ap->ctrl_sock < 0) {
		CWLog("WTP init sock failed: %s", strerror(-ap->ctrl_sock));
		return -1;
	}
	// Allow boardcast
	setsockopt(ap->ctrl_sock, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));

	// Init parameters in struct capwap_ap
	if (wtp_init(ap) < 0) {
		CWLog("WTP init failed");
		return -1;
	}

	if (capwap_start_hostapd(ap) < 0) {
		CWLog("Start hostapd failed");
		return -1;
	}

	ap->base = event_base_new();
	if (!ap->base) {
		CWLog("Create event base fail");
		return -1;
	}

	// Add signal handle for SIGUSR1. We will deal with led events in this handle, and send led
	// status to AC.
	ap->sig_ev = evsignal_new(ap->base, SIGUSR1, wtp_handle_signal, ap);
	evsignal_add(ap->sig_ev, NULL);

	// Add network socket handler
	ap->ctrl_ev = event_new(ap->base, ap->ctrl_sock, EV_READ | EV_PERSIST, wtp_main_loop, ap);
	ap->timeout.tv_sec = ap->discovery_time;
	event_add(ap->ctrl_ev, &ap->timeout);

	// Active it to start state machine before we can receive packets.
	event_active(ap->ctrl_ev, EV_WRITE, 0);
	event_base_dispatch(ap->base);
	CWLog("exit");

	event_free(ap->ctrl_ev);
	event_free(ap->sig_ev);
	event_base_free(ap->base);
	capwap_close_hostapd(ap);
	free(ap);

	return 0;
}
