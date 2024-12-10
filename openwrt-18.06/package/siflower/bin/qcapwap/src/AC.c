#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <errno.h>
#include <pthread.h>

#include <event2/event.h>

#include "tlv.h"
#include "capwap_message.h"
#include "network.h"
#include "CWProtocol.h"
#include "ac_mainloop.h"
#include "uci_config.h"
#include "ac_interface.h"
#include "CWLog.h"

int gLoggingLevel = DEFAULT_LOGGING_LEVEL;
int gEnabledLog = 1;
uint16_t mobility_domain = 0;

/**
 * This function will receive all the packets send to port CW_CONTROL_PORT except
 * those from binded addresses.
 * If we receive a Discovery Request, start a new thread to manage it;
 * if we receive a CW_TYPE_ECHO_REQUEST which means our manage thread has exited
 * but the WTP is still in RUN state, send a RESET request to restart WTP in this
 * case. We can save times for waiting for the WTP restart by timeout.
 */
static void capwap_main_loop(evutil_socket_t sock, short what, void *arg)
{
	unsigned char buff[1024];
	struct wtp_client *client;
	char addr[INET_ADDRMAXLEN];
	struct cw_protohdr header;
	struct cw_ctrlhdr ctrl_hdr;
	int pipe[2];
	int ret_len;

	if (!(what & EV_READ)) {
		CWLog("%s not read event", __func__);
		return;
	}

	client = wtp_client_new();
	if (!client) {
		CWWarningLog("No memory for new wtp");
		return;
	}

	client->addr_len = sizeof(client->addr);
	while ((ret_len = recvfrom(sock, buff, sizeof(buff), 0, (struct sockaddr *)&client->addr, &client->addr_len)) < 0) {
		if (errno == EINTR)
			continue;
		else
			goto fail;
	}

	cwmsg_protohdr_parse(buff, &header);
	cwmsg_ctrlhdr_parse(buff + CAPWAP_HEADER_LEN(header), &ctrl_hdr);

	if (ctrl_hdr.type == CW_TYPE_DISCOVERY_REQUEST) {
		CWLog("receive a discovery request from %s", sock_ntop_r(&client->addr, addr));
		if (socketpair(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0, pipe) < 0) {
			CWLog("Create socketpair failed %d:%s", errno, strerror(errno));
			free(client);
			return;
		}
		client->main_pipe = pipe[0];
		client->wtp_pipe = pipe[1];
		capwap_discovery_state(sock, &ctrl_hdr, client, arg);
		return;
	} else {
		struct cw_ctrlmsg *msg;
		CWWarningLog("Receive unexpected request %d from %s", ctrl_hdr.type, sock_ntop_r(&client->addr, addr));
		if (ctrl_hdr.type  == CW_TYPE_ECHO_REQUEST) {
			msg = cwmsg_ctrlmsg_new(CW_TYPE_RESET_REQUEST, 0);
			if (msg) {
				CWLog("Send reset request to %s", sock_ntop_r(&client->addr, addr));
				capwap_send_ctrl_message_unconnected(sock, msg, &client->addr, client->addr_len);
				cwmsg_ctrlmsg_free(msg);
			}
		}
		free(client);
	}

	return;

fail:
	CWWarningLog("Error receiving message(%d):%s", errno, strerror(errno));
	free(client);
	return;
}

int main(int argc, char const *argv[])
{
	struct rlimit limit;
	struct capwap_ac ac = {0};
	int sock;
	int err = 0;
	uint8_t mac[6];

	if (getrlimit(RLIMIT_NOFILE, &limit)) {
		CWLog("Can't get max file no");
		return errno;
	}
	limit.rlim_cur = limit.rlim_max;
	if (setrlimit(RLIMIT_NOFILE, &limit)) {
		CWLog("Can't set max file no");
		return errno;
	}
	err = get_mac_addr(NETWORK_IF, mac);
	if (err) {
		CWLog("Can't get mac addr of br-lan");
		return err;
	}
	memcpy(&mobility_domain, &mac[4], sizeof(mobility_domain));
	err = uci_interface_init();
	if (err) {
		CWLog("Uci interface init error: %s", strerror(-err));
		return err;
	}

	ac.base = event_base_new();
	if (!ac.base) {
		CWLog("AC event base error, no memory");
		return -ENOMEM;
	}
	// Init interface to communicate with WUM.
	err = capwap_init_main_interface(&ac);
	if (err) {
		CWLog("Capwap init main interface fail: %s", strerror(-err));
		goto err_interface;
	}

	sock = capwap_init_socket(CW_CONTROL_PORT, NULL, 0);
	if (sock < 0) {
		CWCritLog("Can't create basic sock");
		goto err_socket;
	}

	INIT_LIST_HEAD(&ac.wtp_list);
	ac.ctrl_ev = event_new(ac.base, sock, EV_READ | EV_PERSIST, capwap_main_loop, &ac);

	event_add(ac.ctrl_ev, NULL);
	CWLog("start main loop");
	event_base_dispatch(ac.base);
	CWLog("end main loop");

	event_free(ac.ctrl_ev);
	close(sock);
err_socket:
	capwap_destroy_main_interface(&ac);
err_interface:
	event_base_free(ac.base);
	uci_interface_free();

	return err;
}

