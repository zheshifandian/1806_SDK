#ifndef _AC_INTERFACE_H
#define _AC_INTERFACE_H

#define JSON_BUFF_LEN 1000

#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <event2/event.h>

#include "list.h"

#define JSON_CMD	1
#define LIST_CMD	2
#define UPDATE_CMD	3
#define QUIT_CMD	4
#define MSG_END_CMD	5
#define GROUP_CHANGE	6
#define DISCOVERY_REQ	7
#define PRINT_STATIONS	8
#define AP_ROAM_CMD	9
#define REQUEST_CMD	10
#define GROUP_DELETE	11
#define PING_CMD	12
#define PONG_CMD	13

#define MSG_TYPE_RESULT	1
#define MSG_TYPE_STRING	2
#define MSG_TYPE_EOF	3

struct capwap_wtp;
struct capwap_ac;
extern uint16_t mobility_domain;

enum ac_state {
	RECV_AP_ROAM,
	AC_RUN,
};

struct wtp_client {
	struct sockaddr_storage addr;
	socklen_t addr_len;
	enum ac_state state;
	int main_pipe;
	int wtp_pipe;
	struct list_head list;
	pthread_t thread;
	struct capwap_wtp *wtp;
	struct capwap_ac *ac;
	struct event *ac_event;
};
struct wtp_client *wtp_client_new();
void wtp_client_free(struct wtp_client *client);

void wtp_list_lock();
void wtp_list_unlock();
void wtp_list_add(struct capwap_ac *ac, struct wtp_client *client);
void wtp_list_del(struct wtp_client *client);

struct capwap_interface_message {
	uint32_t cmd;
	uint16_t type;
	// payload length
	uint16_t length;
};

struct interface {
	struct sockaddr_storage client;
	socklen_t client_len;
	int sock;
	struct event *listener;
	void *arg;
	// handler to deal with interface messages.
	int (*handle)(struct interface *interface,
		      struct capwap_interface_message *msg, void *payload);
	uint8_t is_main;
};

#define MSG_GET_DATA(msgp) ((void *)(msgp) + sizeof(struct capwap_interface_message))

#define AC_MAIN_MAC "00_00_00_00_00_00"
#define AC_MAIN_INTERFACE "/tmp/WTP.00_00_00_00_00_00"

int capwap_send_discovery_request(struct wtp_client *client);
void capwap_handle_wtp(evutil_socket_t sock, short what, void *arg);

void capwap_handle_ac_command(evutil_socket_t sock, short what, void *arg);
int capwap_init_main_interface(struct capwap_ac *ac);
void capwap_destroy_main_interface(struct capwap_ac *ac);
int capwap_init_wtp_interface(struct capwap_wtp *wtp);
void capwap_destroy_wtp_interface(struct capwap_wtp *wtp);

/**
 * Send command result to WUM, this will close the connected socket, so we can
 * receive new connect again.
 */
void capwap_send_interface_result(struct capwap_wtp *wtp, int err);
int cpawap_send_interface_data(struct interface *interface, void *data, size_t size);
int capwap_send_interface_msg(int sock, struct capwap_interface_message *msg, void *data);

#define capwap_interface_in_progress(wtp)	((wtp)->interface.client_len)

int capwap_wtp_send_new_ap_event(struct capwap_wtp *wtp, uint8_t *bssid);

#endif // _AC_INTERFACE_H