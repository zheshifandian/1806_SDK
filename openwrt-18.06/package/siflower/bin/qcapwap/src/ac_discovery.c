#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#include "tlv.h"
#include "capwap_message.h"
#include "network.h"
#include "CWProtocol.h"
#include "ac_mainloop.h"
#include "ac_interface.h"
#include "CWLog.h"

static int capwap_new_client(struct capwap_ac *ac, struct wtp_client *client)
{
	struct capwap_wtp *arg;
	int err;

	if (!client)
		return -EINVAL;

	client->ac = ac;
	client->ac_event = event_new(ac->base, client->main_pipe, EV_READ | EV_PERSIST, capwap_handle_wtp, client);
	if (!client->ac_event)
		return -ENOMEM;
	event_add(client->ac_event, NULL);
	arg = malloc(sizeof(*arg));
	if (!arg) {
		CWCritLog("No memory for new wtp!!!");
		return -ENOMEM;
	}

	memset(arg, 0, sizeof(*arg));
	memcpy(&arg->ctrl_addr, &client->addr, client->addr_len);
	arg->wtp_addr_len = client->addr_len;
	arg->wtp_client = client;
	client->wtp = arg;
	if (!inet_ntop(client->addr.ss_family, sock_get_addr(&client->addr), arg->ip_addr, sizeof(arg->ip_addr))) {
		CWLog("convert ip string failed with %d", errno);
		err = -errno;
		goto err_str;
	}

	// Start a new thread
	if ((err = pthread_create(&client->thread, NULL, capwap_manage_wtp, arg))) {
		CWWarningLog("create thread fail");
		goto err_str;
	}
	wtp_list_add(ac, client);
	return 0;

err_str:
	free(arg);
	return err;
}

static int capwap_send_discovery_response(int sock, struct cw_ctrlhdr *ctrl_hdr, struct wtp_client *client)
{
	struct cw_ctrlmsg *discovery = cwmsg_ctrlmsg_new(CW_TYPE_DISCOVERY_RESPONSE, ctrl_hdr->seq_num);
	int err;

	CWLog("%s", __func__);
	if (!discovery || cwmsg_assemble_string(discovery, CW_ELEM_AC_NAME_CW_TYPE, AC_NAME, TLV_NOFREE) ||
	    cwmsg_assemble_ipv4_addr(discovery, CW_ELEM_CW_CONTROL_IPV4_ADDRESS_CW_TYPE, NETWORK_IF)) {
		cwmsg_ctrlmsg_destroy(discovery);
		return -ENOMEM;
	}

	err = capwap_send_ctrl_message_unconnected(sock, discovery, &client->addr, client->addr_len);
	cwmsg_ctrlmsg_destroy(discovery);
	return err;
}

int capwap_discovery_state(int sock, struct cw_ctrlhdr *ctrl_hdr, struct wtp_client *client, struct capwap_ac *ac)
{
	struct wtp_client *wtp, *tmp;
	int err;

	// TODO: check discovery request to determine whether to send response or not
	// Check if this wtp already has a thread binded to it by comparing the IP address. If we found one, we
	// have to cancel it first, or the packet will won't be handled correctly.
	// We compare by ip address just because we can't get mac address from the discovery request packet directly.
	wtp_list_lock();
	list_for_each_entry_safe(wtp, tmp, &ac->wtp_list, list) {
		if (!memcmp(&wtp->addr, &client->addr, client->addr_len)) {
			CWWarningLog("wtp already running, cancel it first");
			pthread_cancel(wtp->thread);
			pthread_join(wtp->thread, NULL);
			list_del(&wtp->list);
			wtp_client_free(wtp);
			break;
		}
	}
	wtp_list_unlock();

	// Start new thread to receive data before send response, so that we won't miss join request in new thread.
	if ((err = capwap_new_client(ac, client))) {
		wtp_client_free(client);
		CWLog("start thread for new wtp failed, not send response");
		return err;
	}
	return capwap_send_discovery_response(sock, ctrl_hdr, client);
}