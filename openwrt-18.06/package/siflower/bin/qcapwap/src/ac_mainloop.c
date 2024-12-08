#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <event2/event.h>
#include <event2/listener.h>

#include "capwap_message.h"
#include "CWProtocol.h"
#include "network.h"
#include "ac_mainloop.h"
#include "ac_interface.h"
#include "ac_manager.h"
#include "ac_ieee80211.h"
#include "ac_log.h"

static int capwap_parse_join_request(struct capwap_wtp *wtp, struct cw_ctrlmsg *join_req)
{
	uint16_t elem_type;
	uint16_t elem_len;
	void *elem_value;
	int err = 0;

	cwmsg_ctrlmsg_for_each_elem(join_req, elem_type, elem_len, elem_value) {
		switch (elem_type) {
		case CW_ELEM_LOCATION_DATA_CW_TYPE:
			FREE_STRING(wtp->location);
			wtp->location = cwmsg_parse_string(elem_value, elem_len);
			break;
		case CW_ELEM_WTP_BOARD_DATA_CW_TYPE:
			if (cwmsg_parse_board_data(&wtp->board_data, elem_value, elem_len))
				goto error_request;
			break;
		case CW_ELEM_SESSION_ID_CW_TYPE:
			cwmsg_parse_raw(wtp->session_id, CW_SESSION_ID_LENGTH, elem_value, elem_len);
			break;
		case CW_ELEM_WTP_DESCRIPTOR_CW_TYPE:
			if (cwmsg_parse_wtp_descriptor(&wtp->descriptor, elem_value, elem_len)) {
				CWLog(wtp, "Invalid wtp descriptor");
				goto error_request;
			}
			break;
		case CW_ELEM_WTP_NAME_CW_TYPE:
			FREE_STRING(wtp->name);
			wtp->name = cwmsg_parse_string(elem_value, elem_len);
			break;
		case CW_ELEM_VENDOR_SPEC_WTP_VERSION:
			wtp->version = cwmsg_parse_u32(elem_value);
			CWLog(wtp, "version=%#x", wtp->version);
			break;
		case CW_ELEM_VENDOR_SPEC_PAYLOAD_BW_CW_TYPE:
			if (cwmsg_parse_vendor_spec(&wtp->vendor_spec, elem_value, elem_len)) {
				CWLog(wtp, "Invalid vendor spec payload");
				goto error_request;
			}
			break;
		case CW_ELEM_RESULT_CODE_CW_TYPE:
			err = cwmsg_parse_u32(elem_value);
			break;
		case CW_ELEM_LOCAL_IPV4_ADDRESS_CW_TYPE:
		case CW_ELEM_WTP_FRAME_TUNNEL_MODE_CW_TYPE:
		case CW_ELEM_WTP_MAC_TYPE_CW_TYPE:
		case CW_ELEM_IEEE80211_WTP_RADIO_INFORMATION_CW_TYPE:
		case CW_ELEM_ECN_SUPPORT_CW_TYPE:
			// CWDebugLog(wtp, "Unused Message Element(%d) in Join request", elem_type);
			break;
		default:
			CWLog(wtp, "Unrecognized Message Element(%d) in Join request", elem_type);
			break;
		}
	}

	return err;

error_request:
	FREE(wtp->location);
	FREE(wtp->name);
	return -EINVAL;
}

static int capwap_send_join_response(struct capwap_wtp *wtp, uint32_t result)
{
	struct cw_ctrlmsg *join_resp;
	int err;

	join_resp = cwmsg_ctrlmsg_new(CW_TYPE_JOIN_RESPONSE, wtp->resp_seq_num);
	if (!join_resp || cwmsg_assemble_ac_descriptor(join_resp) ||
	    cwmsg_assemble_result_code(join_resp, result) ||
	    cwmsg_assemble_string(join_resp, CW_ELEM_AC_NAME_CW_TYPE, AC_NAME, TLV_NOFREE) ||
	    cwmsg_assemble_ipv4_addr(join_resp, CW_ELEM_CW_CONTROL_IPV4_ADDRESS_CW_TYPE, NETWORK_IF) ||
	    cwmsg_assemble_ac_version(join_resp, wtp)) {
		cwmsg_ctrlmsg_free(join_resp);
		return -ENOMEM;
	}

	err = capwap_send_response(wtp->ctrl_sock, join_resp);

	return err;
}

static int capwap_parse_configure_request(struct capwap_wtp *wtp, struct cw_ctrlmsg *cfg_req)
{
	uint16_t elem_type;
	uint16_t elem_len;
	void *elem_value;

	cwmsg_ctrlmsg_for_each_elem(cfg_req, elem_type, elem_len, elem_value) {
		switch (elem_type) {
		case CW_ELEM_IEEE80211_MULTI_DOMAIN_CAPABILITY_CW_TYPE:
			if (cwmsg_parse_wifi_info(wtp, elem_value, elem_len))
				return -EINVAL;
			break;
		case CW_ELEM_AC_NAME_CW_TYPE:
		case CW_ELEM_AC_NAME_INDEX_CW_TYPE:
		case CW_ELEM_RADIO_ADMIN_STATE_CW_TYPE:
		case CW_ELEM_STATISTICS_TIMER_CW_TYPE:
		case CW_ELEM_WTP_REBOOT_STATISTICS_CW_TYPE:
		case CW_ELEM_IEEE80211_WTP_RADIO_INFORMATION_CW_TYPE:
		case CW_ELEM_IEEE80211_MAC_OPERATION_CW_TYPE:
		case CW_ELEM_IEEE80211_SUPPORTED_RATES_CW_TYPE:
		default:
			break;
		}
	}

	return 0;
}

static int capwap_send_configure_response(struct capwap_wtp *wtp)
{
	struct cw_ctrlmsg *cfg_resp;
	int err;

	cfg_resp = cwmsg_ctrlmsg_new(CW_TYPE_CONFIGURE_RESPONSE, wtp->resp_seq_num);
	if (!cfg_resp || cwmsg_assemble_timers(cfg_resp, wtp) ||
	    cwmsg_assemble_idle_timeout(cfg_resp, wtp) ||
	    cwmsg_assemble_wtp_fallback(cfg_resp, wtp)) {
		cwmsg_ctrlmsg_destroy(cfg_resp);
		return -ENOMEM;
	}

	err = capwap_send_response(wtp->ctrl_sock, cfg_resp);

	return err;
}

static int capwap_parse_data_check_request(struct capwap_wtp *wtp, struct cw_ctrlmsg *data_check_req)
{
	uint16_t elem_type;
	uint16_t elem_len __attribute__((unused));
	void *elem_value;
	uint32_t result;

	cwmsg_ctrlmsg_for_each_elem (data_check_req, elem_type, elem_len, elem_value) {
		switch (elem_type) {
		case CW_ELEM_RESULT_CODE_CW_TYPE:
			result = cwmsg_parse_u32(elem_value);
			if (result != CW_PROTOCOL_SUCCESS)
				CWLog(wtp, "data check result fail");
			break;
		case CW_ELEM_RADIO_OPERAT_STATE_CW_TYPE:
		default:
			break;
		}
	}

	return 0;
}

static int capwap_send_data_check_response(struct capwap_wtp *wtp)
{
	struct cw_ctrlmsg *data_check;
	int err;

	data_check = cwmsg_ctrlmsg_new(CW_TYPE_CHANGE_STATE_EVENT_RESPONSE, wtp->resp_seq_num);
	if (!data_check)
		return -EINVAL;

	err = capwap_send_response(wtp->ctrl_sock, data_check);
	return err;
}

int capwap_start_data_channel(struct capwap_wtp *wtp)
{
	int err;

	if (SUPPORT_HOSTAPD(wtp->version))
		return 0;

	sock_cpy_addr_port(&wtp->data_addr, &wtp->ctrl_addr);
	sock_set_port_cw(&wtp->data_addr, sock_get_port(&wtp->ctrl_addr) + 1);
	wtp->data_sock = capwap_init_socket(CW_DATA_PORT, &wtp->data_addr, wtp->wtp_addr_len);
	if (wtp->data_sock < 0)
		return wtp->data_sock;
	wtp->data_ev = event_new(wtp->ev_base, wtp->data_sock, EV_READ | EV_PERSIST, capwap_data_channel, wtp);
	err = event_add(wtp->data_ev, NULL);
	if (err)
		CWLog(wtp, "Start data channel failed with %d", err);

	return err;
}

void capwap_send_keepalive(struct capwap_wtp *wtp, void *buff, size_t len)
{
	struct iovec io;

	io.iov_base = buff;
	io.iov_len = len;
	capwap_send_message(wtp->data_sock, &io, 1, NULL, 0);
}

void capwap_data_channel(evutil_socket_t sock, short what, void *arg)
{
	struct capwap_wtp *wtp = arg;
	struct cw_protohdr protohdr;
	uint8_t buff[64];
	size_t msg_len;

	msg_len = capwap_recv_ctrl_message(sock, buff, sizeof(buff));
	if (msg_len <= 0) {
		CWWarningLog(wtp, "receive error");
		return;
	}
	cwmsg_protohdr_parse(buff, &protohdr);
	if (protohdr.head.b.K) {
		// Keep-alive response is always the same with resquest
		CWDebugLog(wtp, "Receive keep-alive");
		capwap_send_keepalive(wtp, buff, msg_len);
	} else if (protohdr.head.b.T) {
		CWDebugLog(wtp, "Receive 80211 frame");
		capwap_manage_80211_frame(wtp, buff + CAPWAP_HEADER_LEN(protohdr), msg_len);
	}
}

void capwap_send_echo_response(struct capwap_wtp *wtp)
{
	struct cw_ctrlmsg *echo = cwmsg_ctrlmsg_new(CW_TYPE_ECHO_RESPONSE, wtp->resp_seq_num);

	if (!echo)
		return;
	capwap_send_response(wtp->ctrl_sock, echo);
}

static int capwap_download_over(struct capwap_wtp *wtp, u32 result)
{
	struct timeval t = {.tv_sec = 20, };

	if (result) {
		CWLog(wtp, "update error %d", result);
		wtp->attr->updating = WTP_UPDATE_ERROR;
		event_add(wtp->timer_ev, &wtp->ctrl_tv);
		capwap_set_state(wtp, RUN);
	} else {
		wtp->attr->updating = WTP_UPDATE_TRANSFER_DONE;
		wtp->attr->status = DEV_SETTING;
		// Set state to run here to avoid wtp_event_response lose.
		event_add(wtp->timer_ev, &t);
		capwap_set_state(wtp, RUN);
	}
	save_device_config(wtp->attr, 0);
	if (capwap_interface_in_progress(wtp))
		capwap_send_interface_result(wtp, result);
	return 0;
}

int capwap_manage_wtp_event(struct capwap_wtp *wtp, struct cw_ctrlmsg *msg)
{
	uint16_t elem_type, elem_len;
	void *elem_value;
	struct cw_ctrlmsg *wtp_event;
	int err = 0;

	CWLog(wtp, "WTP EVENT:");
	cwmsg_ctrlmsg_for_each_elem(msg, elem_type, elem_len, elem_value) {
		switch (elem_type) {
		case CW_ELEM_STATION_ONLINE_CW_TYPE:
			err = capwap_station_online(wtp, elem_value, elem_len);
			break;
		case CW_ELEM_DELETE_STATION_CW_TYPE:
			err = capwap_station_delete(wtp, elem_value, elem_len);
			break;
		case CW_ELEM_VENDOR_SPEC_PAYLOAD_CW_TYPE:
			wtp->attr->led = cwmsg_parse_u32(elem_value);
			save_device_config(wtp->attr, 0);
			break;
		case CW_ELEM_VENDOR_SPEC_DOWNLOAD_STATUS:
			err = capwap_download_over(wtp, cwmsg_parse_u32(elem_value));
			break;
		default:
			break;
		}
	}
	if (err)
		return err;

	wtp_event = cwmsg_ctrlmsg_new(CW_TYPE_WTP_EVENT_RESPONSE, wtp->resp_seq_num);
	if (!wtp_event)
		return -ENOMEM;

	return capwap_send_response(wtp->ctrl_sock, wtp_event);
}

int capwap_manage_configure_update(struct capwap_wtp *wtp, struct cw_ctrlmsg *msg)
{
	uint16_t elem_type;
	uint16_t elem_len __attribute__((unused));
	void *elem_value;
	int result;

	cwmsg_ctrlmsg_for_each_elem(msg, elem_type, elem_len, elem_value) {
		switch (elem_type) {
		case CW_ELEM_RESULT_CODE_CW_TYPE:
		case CW_ELEM_RESULT_CODE_CW_TYPE_WITH_PAYLOAD:
			result = cwmsg_parse_u32(elem_value);
			break;
		}
	}

	if (wtp->state == UPDATE) {
		if (result != CW_PROTOCOL_SUCCESS) {
			wtp->attr->updating = WTP_UPDATE_ERROR;
			save_device_config(wtp->attr, 0);
		}
		if (!SUPPORT_NEW_UPDATE(wtp->version))
			return capwap_download_over(wtp, result);
	}

	return result;
}

int capwap_manage_wtp_command_response(struct capwap_wtp *wtp, struct cw_ctrlmsg *msg)
{
	uint16_t elem_type;
	uint16_t elem_len;
	void *elem_value;
	int result = 0, end = 0;

	cwmsg_ctrlmsg_for_each_elem(msg, elem_type, elem_len, elem_value) {
		switch (elem_type) {
		case CW_ELEM_RESULT_CODE_CW_TYPE:
			result = cwmsg_parse_u32(elem_value);
			end = 1;
			break;
		case CW_ELEM_RESULT_STRING:
			result = cpawap_send_interface_data(&wtp->interface, elem_value, elem_len);
			break;
		}
	}
	if (end)
		capwap_send_interface_result(wtp, result);
	return result;
}

void capwap_set_state(struct capwap_wtp *wtp, enum wtp_state state)
{
	wtp->state = state;
	if (state == RUN)
		event_add(wtp->ac_ev, NULL);
}

void capwap_wtp_quit(struct capwap_wtp *wtp, int err)
{
	CWLog(wtp, "quit due to err %d", err);
	capwap_set_state(wtp, QUIT);
	if (capwap_interface_in_progress(wtp))
		capwap_send_interface_result(wtp, err);
	event_base_loopbreak(wtp->ev_base);
}

int capwap_run(struct capwap_wtp *wtp, struct cw_ctrlmsg *ctrlmsg)
{
	int result = 0;

	switch (cwmsg_ctrlmsg_get_type(ctrlmsg)) {
	case CW_TYPE_ECHO_REQUEST:
		capwap_send_echo_response(wtp);
		// Do not trigger state machine when receive echo request
		result = -EBADMSG;
		break;
	case CW_TYPE_CONFIGURE_UPDATE_RESPONSE:
		return capwap_manage_configure_update(wtp, ctrlmsg);
	case CW_TYPE_WTP_EVENT_REQUEST:
		return capwap_manage_wtp_event(wtp, ctrlmsg);
	case CW_TYPE_WTP_EXECUTE_COMMAND_RESPONSE:
		return capwap_manage_wtp_command_response(wtp, ctrlmsg);
	case CW_TYPE_STATION_CONFIGURATION_RESPONSE:
		result = cwmsg_parse_result_code(ctrlmsg);
		if (result != CW_PROTOCOL_SUCCESS)
			CWWarningLog(wtp, "Station configuration failed");
		break;
	case CW_TYPE_RESET_RESPONSE:
		result = cwmsg_parse_result_code(ctrlmsg);
		if (result == CW_PROTOCOL_SUCCESS) {
			if (capwap_interface_in_progress(wtp))
				capwap_send_interface_result(wtp, CW_PROTOCOL_SUCCESS);
			wtp->attr->status = DEV_SETTING;
			capwap_set_state(wtp, QUIT);
		} else {
			CWLog(wtp, "reset failed");
			wtp->attr->status = DEV_SET_ERROR;
			save_device_config(wtp->attr, 0);
		}
		break;
	/****************** Not used types *********************/
	case CW_TYPE_CHANGE_STATE_EVENT_REQUEST:
	case CW_TYPE_DATA_TRANSFER_REQUEST:
	case CW_TYPE_CLEAR_CONFIGURATION_RESPONSE:
	default:
		return -EBADMSG;
	}

	return result;
}

int capwap_do_send_request(struct capwap_wtp *wtp, struct cw_ctrlmsg *msg)
{
	struct timeval t = {0};
	int err;

	if (!msg)
		return -EINVAL;
	if (!SUPPORT_NEW_UPDATE(wtp->version) && wtp->state == UPDATE)
		t.tv_sec = 60;
	else
		t.tv_sec = 8;
	wtp->expect_seq_num = cwmsg_ctrlmsg_get_seqnum(msg);
	CWLog(wtp, "%s: %d(%ld)", __func__, cwmsg_ctrlmsg_get_type(msg), t.tv_sec);
	err = capwap_send_ctrl_message(wtp->ctrl_sock, msg);
	if (!err) {
		wtp->wait = true;
		wtp->resp_type = cwmsg_ctrlmsg_get_type(msg) + 1;
		wtp->req = msg;
		// Reset timeout after send a request
		event_add(wtp->timer_ev, &t);
	} else {
		cwmsg_ctrlmsg_destroy(msg);
	}
	return err;
}

int capwap_send_request(struct capwap_wtp *wtp, struct cw_ctrlmsg *msg)
{
	struct capwap_interface_message message = {0};

	message.cmd = REQUEST_CMD;
	message.length = sizeof(msg);

	return capwap_send_interface_msg(wtp->wtp_client->main_pipe, &message, &msg);
}

static void capwap_resend_request(struct capwap_wtp *wtp)
{
	struct timeval t = {8, 0};

	if (!wtp || !wtp->wait || !wtp->req || !wtp->resp_type)
		return;
	capwap_send_ctrl_message(wtp->ctrl_sock, wtp->req);
	event_add(wtp->timer_ev, &t);
}

static int capwap_handle_packet(struct capwap_wtp *wtp, struct cw_ctrlmsg *ctrlmsg)
{
	uint32_t type = cwmsg_ctrlmsg_get_type(ctrlmsg);
	int err = -EBADMSG;

	if (wtp->wait && CAPWAP_MSG_IS_RESPONSE(ctrlmsg)) {
		if (cwmsg_ctrlmsg_get_seqnum(ctrlmsg) != wtp->expect_seq_num)
			return -EBADMSG;
		CWDebugLog(wtp, "stop retransfer");
		wtp->wait = false;
		cwmsg_ctrlmsg_destroy(wtp->req);
		wtp->req = NULL;
		wtp->resp_type = 0;
		wtp->retry_count = 0;
	}
	switch (type) {
	case CW_TYPE_DISCOVERY_REQUEST:
		capwap_wtp_quit(wtp, QUIT);
		break;
	case CW_TYPE_JOIN_REQUEST:
		if (wtp->state == IDLE) {
			err = capwap_parse_join_request(wtp, ctrlmsg);
			if (err)
				CWWarningLog(wtp, "join request parse failed with %d", err);
			capwap_set_state(wtp, JOIN);
			break;
		}
		break;
	case CW_TYPE_CONFIGURE_REQUEST:
		if (wtp->state == JOIN || wtp->state == CONFIGURE) {
			err = capwap_parse_configure_request(wtp, ctrlmsg);
			if (err)
				CWWarningLog(wtp, "Parse configure request failed with %d", err);
			capwap_set_state(wtp, CONFIGURE);
		}
		break;
	case CW_TYPE_CHANGE_STATE_EVENT_REQUEST:
		if (wtp->state == CONFIGURE || wtp->state == DATA_CHECK) {
			err = capwap_parse_data_check_request(wtp, ctrlmsg);
			if (err)
				CWWarningLog(wtp, "Parse data_check request failed with %d", err);
			capwap_set_state(wtp, DATA_CHECK);
		}
		break;
	case CW_TYPE_AP_ROAM_RESPONSE:
		if (wtp->state == AP_ROAM_UPDATE || wtp->state == AP_ROAM)
			err = cwmsg_parse_result_code(ctrlmsg);
		else	// Ignore error in RUN state.
			err = 0;
		break;
	case CW_TYPE_WLAN_CONFIGURATION_RESPONSE:
		err = capwap_handle_wlan_response(wtp,
			cwmsg_parse_wlan_config_response(ctrlmsg, wtp));
		break;
	default:
		err = capwap_run(wtp, ctrlmsg);
		break;
	}

	// After we got a response, add ac_ev to wait for a new request
	if (CAPWAP_MSG_IS_RESPONSE(ctrlmsg)) {
		CWLog(wtp, "event add");
		event_add(wtp->ac_ev, NULL);
	}
	return err;
}

// This function is called when a set error occurs.
static void capwap_wtp_set_error(struct capwap_wtp *wtp, int err)
{
	reload_dev_attr(wtp->attr);
	wtp->attr->status = DEV_SET_ERROR;
	capwap_wtp_quit(wtp, err);
}

static void capwap_main_fsm(struct capwap_wtp *wtp, int err)
{
	uint32_t result;

	// EBADMSG means we have received an unexpected message, ignore it
	if (err == -EBADMSG)
		return;

	switch (wtp->state) {
	case JOIN:
		result = err ? CW_PROTOCOL_FAILURE : CW_PROTOCOL_SUCCESS;
		if (capwap_send_join_response(wtp, result)) {
			CWWarningLog(wtp, "Send join response failed: %s", strerror(errno));
			return;
		}
		err = ac_init_wtp_attr(wtp);
		if (err) {
			CWLog(wtp, "Error init dev attr, quit");
			capwap_wtp_quit(wtp, err);
			break;
		}
		wtp->ctrl_tv.tv_sec = wtp->attr->ap_alive_time;
		break;
	case CONFIGURE:
		if (err)
			return;
		capwap_send_configure_response(wtp);
		break;
	case DATA_CHECK:
		if (err)
			return;
		err = capwap_send_data_check_response(wtp);
		if (err) {
			CWWarningLog(wtp, "Failed to send data check response: %d", err);
			return;
		}
		err = capwap_start_data_channel(wtp);
		if (err) {
			CWWarningLog(wtp, "Failed to start data channel: %d", err);
			capwap_wtp_quit(wtp, err);
			break;
		}
		err = capwap_init_wtp_interface(wtp);
		if (err) {
			CWLog(wtp, "Init interface %s error: %d", wtp->if_path, err);
			capwap_wtp_quit(wtp, err);
			break;
		}
		if (SUPPORT_AP_ROAM(wtp->version)) {
			err = capwap_wtp_send_mac_list(wtp);
			if (err) {
				CWLog(wtp, "Send mac list to it error: %d", err);
				capwap_wtp_quit(wtp, err);
				break;
			}
			capwap_set_state(wtp, AP_ROAM);
		} else {
			err = ac_add_wlan(wtp);
			capwap_set_state(wtp, RUN);
		}
		event_add(wtp->ac_ev, NULL);
		break;
	case AP_ROAM:
		if (err) {
			CWLog(wtp, "roam mac list set error");
			capwap_wtp_set_error(wtp, err);
		} else {
			err = ac_add_wlan(wtp);
			capwap_set_state(wtp, RUN);
		}
		break;
	case AP_ROAM_UPDATE:
		if (err)
			CWLog(wtp, "roam mac list update error");
		capwap_set_state(wtp, RUN);
		break;
	case SET:
		if (err) {
			CWLog(wtp, "config set error");
			capwap_wtp_set_error(wtp, err);
		} else {
			if (SUPPORT_HOSTAPD(wtp->version)) {
				err = ac_add_wlan(wtp);
				capwap_set_state(wtp, RUN);
			} else {
				ac_reset_device(wtp);
			}
		}
		break;
	case RUN:
		if (err)
			CWLog(wtp, "Got err %d in run state", err);
		break;
	case QUIT:
		CWLog(wtp, "Enter QUIT state");
		event_base_loopbreak(wtp->ev_base);
		break;
	case SULK:
		if (capwap_interface_in_progress(wtp))
			capwap_send_interface_result(wtp, err);
	default:
		break;
	}
}

static void capwap_control_port(evutil_socket_t sock, short what, void *arg)
{
	uint8_t buff[1024];
	struct capwap_wtp *wtp = arg;
	struct cw_ctrlmsg *ctrlmsg = NULL;
	int msg_len;
	int err = 0;

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (what & EV_READ) {
		msg_len = capwap_recv_ctrl_message(sock, buff, sizeof(buff));
		if (msg_len <= 0) {
			CWWarningLog(wtp, "receive error");
			goto out;
		}
		ctrlmsg = cwmsg_ctrlmsg_malloc();
		err = cwmsg_ctrlmsg_parse(ctrlmsg, buff, msg_len);
		if (err) {
			cwmsg_ctrlmsg_free(ctrlmsg);
			CWWarningLog(wtp, "Capwap message parse error(%d) of port %d", err, sock_get_port(&wtp->ctrl_addr));
			goto out;
		}
		wtp->resp_seq_num = cwmsg_ctrlmsg_get_seqnum(ctrlmsg);
		// Add again to reschedule timeout except: when we are waiting for a response and received a echo request
		if (!(cwmsg_ctrlmsg_get_type(ctrlmsg) == CW_TYPE_ECHO_REQUEST && wtp->wait)) {
			event_add(wtp->timer_ev, &wtp->ctrl_tv);
		}

		CWLog(wtp, "receive %d from port %d", cwmsg_ctrlmsg_get_type(ctrlmsg), sock_get_port(&wtp->ctrl_addr));
		err = capwap_handle_packet(wtp, ctrlmsg);
		cwmsg_ctrlmsg_free(ctrlmsg);

		capwap_main_fsm(wtp, err);
	}
out:
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
}

static void capwap_timeout_handle(evutil_socket_t sock, short what, void *arg) {
	struct capwap_wtp *wtp = arg;

	if (what & EV_TIMEOUT) {
		if (!wtp->wait) {
			CWLog(wtp, "timeout, consider offline...");
			if (wtp->attr && (!(wtp->attr->wtp && wtp->attr->wtp != wtp)))
				wtp->attr->status = DEV_OFFLINE;
			capwap_wtp_quit(wtp, -ETIMEDOUT);
		} else {
			// Retransfer
			if (wtp->retry_count > 3) {
				if (wtp->attr->wtp == wtp)
					wtp->attr->status = DEV_OFFLINE;
				capwap_wtp_quit(wtp, -ENOPKG);
			} else {
				wtp->retry_count++;
				capwap_resend_request(wtp);
				CWLog(wtp, "Retransfer %d times", wtp->retry_count);
			}
		}
	}
}

// void capwap_reinit(struct capwap_wtp *wtp)
// {
// 	int i;

// 	capwap_set_state(wtp, IDLE);
// 	wtp->ctrl_tv.tv_sec = 10;
// 	for (i = 0; i < WIFI_NUM; i++)
// 		wtp->wifi[i].setted = 0;
// 	if (wtp->data_ev) {
// 		event_free(wtp->data_ev);
// 		close(wtp->data_sock);
// 		wtp->data_ev = NULL;
// 	}
// 	if (wtp->wait) {
// 		wtp->wait = false;
// 		cwmsg_ctrlmsg_destroy(wtp->req);
// 		wtp->req = NULL;
// 		wtp->resp_type = 0;
// 		wtp->retry_count = 0;
// 	}
// 	capwap_destroy_wtp_interface(wtp);
// 	FREE(wtp->location);
// 	FREE(wtp->name);
// }

void capwap_quit_wtp(void *arg)
{
	struct capwap_wtp *wtp = arg;

	if (!wtp)
		return;
	if (wtp->wait) {
		wtp->wait = false;
		cwmsg_ctrlmsg_destroy(wtp->req);
		wtp->req = NULL;
		wtp->resp_type = 0;
		wtp->retry_count = 0;
	}
	if (wtp->attr) {
		wtp->attr->wtp = NULL;
		save_device_config(wtp->attr, 0);
	}

	if (wtp->ctrl_ev) {
		event_free(wtp->ctrl_ev);
		close(wtp->ctrl_sock);
		wtp->ctrl_ev = NULL;
	}
	if (wtp->timer_ev) {
		event_free(wtp->timer_ev);
		wtp->timer_ev = NULL;
	}
	if (wtp->data_ev) {
		event_free(wtp->data_ev);
		close(wtp->data_sock);
		wtp->data_ev = NULL;
	}
	if (wtp->ac_ev)
		event_free(wtp->ac_ev);
	capwap_destroy_wtp_interface(wtp);
	capwap_wtp_remove_from_mac_list(wtp);
	if (wtp->ev_base)
		event_base_free(wtp->ev_base);
	capwap_wtp_delete_stations(wtp);
	FREE(wtp->location);
	FREE(wtp->name);
	free(wtp);
}

static void capwap_free_wtp(struct capwap_wtp *wtp)
{
	wtp_list_del(wtp->wtp_client);
	capwap_quit_wtp(wtp);
}

void *capwap_manage_wtp(void *arg)
{
	struct capwap_wtp *wtp = arg;
	int sock;

	CWLog(wtp, "New thread to manage wtp");
	sock = capwap_init_socket(CW_CONTROL_PORT, &wtp->ctrl_addr, wtp->wtp_addr_len);
	if (sock < 0) {
		CWCritLog(wtp, "establish to it error: %s", strerror(-sock));
		free(arg);
		return NULL;
	}

	INIT_LIST_HEAD(&wtp->stations);
	wtp->ctrl_sock = sock;
	wtp->data_sock = -1;
	capwap_set_state(wtp, IDLE);
	wtp->ev_base = event_base_new();
	wtp->ctrl_ev = event_new(wtp->ev_base, sock, EV_READ | EV_PERSIST, capwap_control_port, wtp);
	wtp->ac_ev = event_new(wtp->ev_base, wtp->wtp_client->wtp_pipe, EV_READ,
			       capwap_handle_ac_command, wtp);

	wtp->ctrl_tv.tv_sec = 20;
	wtp->timer_ev = evtimer_new(wtp->ev_base, capwap_timeout_handle, wtp);
	event_add(wtp->timer_ev, &wtp->ctrl_tv);
	event_add(wtp->ctrl_ev, NULL);

	pthread_cleanup_push(capwap_quit_wtp, wtp);
	event_base_dispatch(wtp->ev_base);

	CWCritLog(wtp, "closed");
	capwap_free_wtp(wtp);
	pthread_cleanup_pop(0);

	return NULL;
}

void set_ap_idle_timeout(struct capwap_wtp *wtp, uint8_t timeout)
{
	wtp->ctrl_tv.tv_sec = timeout;
	wtp->ctrl_tv.tv_usec = 0;
}

uint8_t get_echo_interval(struct capwap_wtp *wtp)
{
	if (SUPPORT_HOSTAPD(wtp->version))
		return (uint8_t)(wtp->ctrl_tv.tv_sec - 5);

	return (uint8_t)((wtp->ctrl_tv.tv_sec - 5) / CW_ECHO_MAX_RETRANSMIT_DEFAULT);
}

uint32_t get_idle_timeout(struct capwap_wtp *wtp)
{
	return (uint32_t)wtp->attr->client_idle_time;
}

uint8_t get_seq_num(struct capwap_wtp *wtp)
{
	wtp->seq_num++;
	return wtp->seq_num;
}

struct wifi_info *find_wifi_info(struct capwap_wtp *wtp, uint8_t radio_id, uint8_t wlan_id)
{
	int i;

	if (!wtp)
		return NULL;
	for (i = 0; i < WIFI_NUM; i++) {
		if (wtp->wifi[i].radio_id == radio_id && wtp->wifi[i].wlan_id == wlan_id)
			return &wtp->wifi[i];
	}
	return NULL;
}
