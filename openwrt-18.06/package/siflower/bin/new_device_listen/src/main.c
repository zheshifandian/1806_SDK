#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>
#include <getopt.h>
#include <curl/curl.h>
#include <uci.h>
#include <regex.h>
#include <signal.h>
#include <linux/rtnetlink.h>

#include "utils.h"
#include "changes.h"
//#include "http.h"
#include "iwlib.h"
#include "iwevent.h"
#include "local_storage.h"
#include "thread_ssl.h"
#include "ssst_request.h"
//#include "mtd.h"
#include "token.h"
#include "cJSON.h"
#include "sf_factory_read.h"

#define WDS_PORT    1111

#ifdef URLLIST
#define URLLIST_STRING    "URLL"
#endif
#ifdef P2P
/*define create p2p cmd*/
#define CP2P_CMD	9
/*define destroy p2p cmd*/
#define DP2P_CMD	10
#endif
#ifdef URLLIST
#define URL_CMD	12
#endif

#ifdef USE_CUSTOM_SOCKET
#define WIFI_CUSTOM_LISTEN_PORT 7892
#endif

#define FETCH_MESSAGE_WAIT_TIMEOUT_SECONDS 5

char XCLOUD_REMOTE_ADDR[64] = {0};
char XCLOUD_REMOTE_FUNCTION_ADDR[64] = {0};
char XCLOUD_REMOTE_DATA_ADDR[64] = {0};

volatile int g_force_exit_signal = 0;
volatile bool g_mesh_slave = false;
mesh_msg_q *g_mesh_msg_queue = NULL;

struct uloop_fd wds_fd;
struct uloop_fd mesh_fd;
struct uloop_fd netlink_fd;
struct uloop_fd arp_event_fd;
struct uloop_fd dps_fd;
struct uloop_fd usb_fd;
struct uloop_fd ts_fd;
#ifdef USE_CUSTOM_SOCKET
struct uloop_fd wifi_fd;
#endif

extern void handle_netlink_events(struct uloop_fd *rth, unsigned int uevents);
extern void handle_dps_netlink_event(struct uloop_fd *rth, unsigned int uevents);
extern void handle_usb_netlink_uevent(struct uloop_fd *rth, unsigned int uevents);
extern void handle_arp_netlink_event(struct uloop_fd *rth, unsigned int uevents);
extern void handle_ts_netlink_event(struct uloop_fd *rth, unsigned int uevents);
#ifdef USE_CUSTOM_SOCKET
extern void handle_custom_wifi_events(struct uloop_fd *rth, unsigned int uevents);
#endif

typedef struct wds_msg{
	int fd;
	char buf[BUFFER_SIZE];
	struct sockaddr_in sock_addr;
	socklen_t sock_len;
}wds_msg;

void sighandler(int sig)
{
	printf("handle signal!\n");
	g_force_exit_signal = 1;
}

extern u_int16_t wan_ifi;
static int iwevent_process(void* args)
{
#ifdef USE_CUSTOM_SOCKET
	struct rtnl_handle    rth[6];
#else
	struct rtnl_handle    rth[5];
#endif
	int group, ret;
	int dpsgroup, tsgroup = 1;
	char wan_if[32];

	ret = getUciConfig("network", "wan", "ifname", wan_if);
	if(ret < 0 ){
		LOG("Get wan ifname fail\n");
		return ret;
	}

	wan_ifi = name2index(wan_if);
	DB_LOG("wan interface index is %d\n", wan_ifi);
	if(wan_ifi < 0){
		LOG("Get wan ifname index fail\n");
		return -1;
	}

	if(rtnl_open(&rth[0], 0, NETLINK_GENERIC) < 0)
	{
		perror("Can't initialize rtnetlink socket");
		return -1;
	}
	group = get_genl_group(&rth[0], "nl80211", "mlme");
	dpsgroup = get_genl_group(&rth[0], "DPS_NL", "updown");
	//tsgroup = get_genl_group(&rth[0], "SF_TS_NL", "sf-ts");
	if ( group < 0 || dpsgroup < 0 || tsgroup < 0){
		LOG("group number is %d, dpsgroup number is %d, tsgroup number is %d\n", group, dpsgroup, tsgroup);
		goto err1;
	}

	ret = setsockopt(rth[0].fd, SOL_NETLINK, NETLINK_ADD_MEMBERSHIP, &group , sizeof(int));
	if(ret){
		LOG("setsockopt error\n");
		goto err1;
	}else{
		netlink_fd.cb = handle_netlink_events;
		netlink_fd.fd = rth[0].fd;
		netlink_fd.registered =  false;
		netlink_fd.flags = ULOOP_READ;
	}

	fprintf(stderr, "Waiting for Wireless Events from interfaces...\n");

	if (rtnl_open(&rth[1], 1 << (dpsgroup - 1), NETLINK_GENERIC) < 0){
		perror("Can't initialize generic netlink\n");
		goto err1;
	}else{
		dps_fd.cb = handle_dps_netlink_event;
		dps_fd.fd = rth[1].fd;
		dps_fd.registered =  false;
		dps_fd.flags = ULOOP_READ;
	}


	if (rtnl_open(&rth[2], 1,NETLINK_KOBJECT_UEVENT) < 0){
		perror("Can't initialize uevent netlink\n");
		goto err2;
	}else{
		usb_fd.cb = handle_usb_netlink_uevent;
		usb_fd.fd = rth[2].fd;
		usb_fd.registered =  false;
		usb_fd.flags = ULOOP_READ;
	}


	if (rtnl_open(&rth[3], 1 << (RTNLGRP_NEIGH - 1), NETLINK_ROUTE) < 0){
		perror("Can't initialize uevent netlink\n");
		goto err3;
	}else{
		arp_event_fd.cb = handle_arp_netlink_event;
		arp_event_fd.fd = rth[3].fd;
		arp_event_fd.registered =  false;
		arp_event_fd.flags = ULOOP_READ;
	}


#if 0
	if (rtnl_open(&rth[4], 1 << (tsgroup - 1), NETLINK_GENERIC) < 0){
		perror("Can't initialize generic netlink\n");
		goto err4;
	}else{
		ts_fd.cb = handle_ts_netlink_event;
		ts_fd.fd = rth[4].fd;
		ts_fd.registered =  false;
		ts_fd.flags = ULOOP_READ;
	}
#endif


	//if define USE_CUSTOM_SOCKET we use custom udp socket,otherwise use netlink socket
#ifdef USE_CUSTOM_SOCKET
	if(init_wifi_custom_socket(&rth[5],WIFI_CUSTOM_LISTEN_PORT) < 0)
	{
		fprintf(stderr,"Can't initialize custom socket");
		goto err5;
	}
#endif
	//wait_for_event(rth);

	return 0;
err5:
	rtnl_close(&rth[4]);
//err4:
//	rtnl_close(&rth[3]);
err3:
	rtnl_close(&rth[2]);
err2:
	rtnl_close(&rth[1]);
err1:
	rtnl_close(&rth[0]);

	return -1;
}

static void *wds_msg_handler(void *param)
{
	FILE *fstream = NULL;
	char *mac, *tag, *ssid;
	char cmd[256] = {0};
	char tmp[128] = {0};
	struct wds_msg *msg = param;

	cJSON *data = cJSON_Parse(msg->buf);
	if (data == NULL){
		LOG("[server] wds json pack into data error\n");
		free(msg);
		return NULL;
	}

	mac = cJSON_GetObjectItem(data, "mac")->valuestring;
	tag = cJSON_GetObjectItem(data, "tag")->valuestring;
	ssid = cJSON_GetObjectItem(data, "ssid")->valuestring;

	if (mac && tag){
		sprintf(cmd, "ifconfig |grep %s || cat /etc/config/ap_groups |grep %s || cat /etc/config/capwap_devices |grep %s", mac, ssid, ssid);
		if((fstream = popen(cmd, "r")) != NULL){
			while(fgets(tmp, sizeof(tmp), fstream) != NULL){
				LOG("[server] wds popen fsgets %s\n", tmp);
			}
		}
		pclose(fstream);

		if (tmp[0] != 0){
			do {
				memset(tmp, 0, sizeof(tmp));
				// return wds ip if wds enabled and connected, wds_sta_status 0 means connected
				if ((fstream = fopen("/tmp/wds_sta_status","r")) != NULL){
					fgets(tmp, sizeof(tmp), fstream);
					fclose(fstream);
					LOG("[server] wds popen fgets wds status %s\n", tmp);
					if (tmp[0] == '0'){
						if ((fstream = popen("uci get network.stabridge.ipaddr |tr -d '\n'","r")) != NULL){
							while(fgets(tmp, sizeof(tmp), fstream) != NULL){
								LOG("[server] wds popen fgets stabridge ipaddr %s\n", tmp);
							}
							pclose(fstream);
							break;
						}
					}
				}

				// return lan ip in normal mode
				if ((fstream = popen("uci get network.lan.ipaddr |tr -d '\n'","r")) != NULL){
					while(fgets(tmp, sizeof(tmp), fstream) != NULL){
						LOG("[server] wds popen fgets lan ipaddr %s\n", tmp);
					}
					pclose(fstream);
				}
			}while(0);
		}else
		  goto ERROR;

		cJSON *sendbuf = cJSON_CreateObject();
		cJSON_AddStringToObject(sendbuf, "ip", tmp);
		cJSON_AddStringToObject(sendbuf, "tag", tag);
		char *s = cJSON_PrintUnformatted(sendbuf);
		if(s) {
			LOG("[server] wds sendbuf:%s\n", s);
		}
		if (sendto(msg->fd, s, strlen(s), 0, (struct sockaddr*) &msg->sock_addr, msg->sock_len) < 0){
			LOG( "[server] wds send message fail, errno=%s\n", strerror(errno));
		}

		free(s);
		cJSON_Delete(sendbuf);
	}

ERROR:
	cJSON_Delete(data);
	free(msg);
	return NULL;
}

static void wds_cb(struct uloop_fd *u,unsigned int events){
	pthread_t wdsThread;
	pthread_attr_t att;
	struct sockaddr_in clt_addr;
	socklen_t sock_len;
	char buf[BUFFER_SIZE] = {0};
	int32_t len = 0, ret = -1;
	int32_t fd = u->fd;

	pthread_attr_init(&att);
	pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED);

	sock_len = sizeof(clt_addr);
	if ((len = recvfrom(fd, buf, BUFFER_SIZE, 0, (struct sockaddr*)&clt_addr, &sock_len)) == -1){
		LOG( "[server] wds receive message fail from:%s, break\n", inet_ntoa(clt_addr.sin_addr));
		return;
	}
	LOG( "[server] wds receive message from:ip %s, content:%s\n",inet_ntoa(clt_addr.sin_addr), buf);

	struct wds_msg *msg = malloc(sizeof(struct wds_msg));
	msg->fd = fd;
	msg->sock_len = sock_len;
	memcpy(msg->buf, buf, BUFFER_SIZE);
	memcpy(&msg->sock_addr, &clt_addr, sock_len);

	ret = pthread_create(&wdsThread, &att, wds_msg_handler, (void *)msg);
	if (ret != 0){
		LOG("[server] wds pthread create error:%s!\n", strerror(ret));
		free(msg);
	}
	return;
}

static int wds_socket_server(void *args)
{
	int32_t fd;
	struct sockaddr_in srv_addr;
	int32_t val = 1;

	//create socket to bind local IP and PORT
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(fd < 0)
	{
		LOG("[server] can't create wds socket!\n");
		return -1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val))) {
		LOG("[server] wds setsockopt error!\n");
		return -1;
	}

	bzero(&srv_addr, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	srv_addr.sin_port = htons(WDS_PORT);

	//bind socket
	if (bind(fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0)
	{
		LOG("[server] wds bind error!\n");
		return -1;
	}

	wds_fd.fd = fd;
	wds_fd.cb = wds_cb;
	wds_fd.registered = false;
	wds_fd.flags = ULOOP_READ;

	return 0;
}

/**
 * Create the mesh msg queue
 */
mesh_msg_q *create_mesh_msg_queue(void)
{
	mesh_msg_q *newQueue = (mesh_msg_q *)malloc(sizeof(mesh_msg_q));
	newQueue->count = 0;
	newQueue->header = NULL;
	newQueue->tail = NULL;
	pthread_mutex_init(&newQueue->lock,NULL);
	sem_init (&newQueue->sem, 0, 0);
	return newQueue;
}

/**
 * Free the mesh msg queue
 */
int destory_mesh_msg_queue(mesh_msg_q *msgQueue)
{
	int ret = -1;
	if(msgQueue)
	{
		pthread_mutex_lock(&msgQueue->lock);
		mesh_msg *msg = msgQueue->header;
		while(msg){
			mesh_msg *next = msg->next;
			free(msg);
			msg = next;
		}
		msgQueue->header = NULL;
		msgQueue->tail = NULL;
		ret = sem_destroy(&msgQueue->sem);
		pthread_mutex_unlock(&msgQueue->lock);
		ret |= pthread_mutex_destroy(&msgQueue->lock);
		free(msgQueue);
	}
	return ret;
}

/*
 * Get the msg from the queue
 */
mesh_msg *fetch_mesh_msg(mesh_msg_q *msg_queue, int blocked, int timeouts)
{
	mesh_msg *ret = NULL;
	if(blocked)
	{
		if(timeouts == -1){
			sem_wait(&msg_queue->sem);
		}else{
			struct timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += timeouts;
			sem_timedwait(&msg_queue->sem, &ts);
		}
	}
	else{
			sem_trywait(&msg_queue->sem);
		}

	pthread_mutex_lock(&msg_queue->lock);
	if(msg_queue->header == NULL){
			ret = NULL;
	}else{
		ret = msg_queue->header;
		msg_queue->header = ret->next;
		if(ret == msg_queue->tail) msg_queue->tail = NULL;
		msg_queue->count--;
	}
	pthread_mutex_unlock (&msg_queue->lock);
	return ret;
}

/*
 * Send the message into the queue
 */
int send_mesh_message(mesh_msg_q *msg_queue, void *msg)
{
	int ret = -1;
	pthread_mutex_lock(&msg_queue->lock);
	mesh_msg *Msg = (mesh_msg *)msg;
	Msg->next = NULL;

	if(!msg_queue->tail){
		Msg->next = NULL;
		msg_queue->tail = Msg;
		msg_queue->header = Msg;
	}else{
		mesh_msg *tail = msg_queue->tail;
		tail->next = Msg;
		msg_queue->tail = Msg;
	}
	msg_queue->count++;
	ret = sem_post(&msg_queue->sem);
	pthread_mutex_unlock(&msg_queue->lock);
	return ret;
}

/**
 * Mesh slave device updown msg handler
 */
static void *mesh_msg_handler(void *param)
{
	char cmd[256] = {0}, *rtMac = NULL, *devMac = NULL, *dev_name = NULL;
	int devPort, updown, is_wireless;

	mesh_msg_q *mesh_msg_queue = (mesh_msg_q *)param;
	while(1) {
		mesh_msg *msg = fetch_mesh_msg(mesh_msg_queue, 0, FETCH_MESSAGE_WAIT_TIMEOUT_SECONDS);
		if(msg != NULL){

			cJSON *data = cJSON_Parse(msg->buf);
			if (data == NULL){
				LOG("[server] mesh json msg format error\n");
				free(msg);
				continue;
			}

			is_wireless = cJSON_GetObjectItem(data, "is_wireless")->valueint;
			if (is_wireless) {
				devMac = cJSON_GetObjectItem(data, "devMac")->valuestring;
				dev_name = cJSON_GetObjectItem(data, "dev")->valuestring;
			}else {
				devPort = cJSON_GetObjectItem(data, "devPort")->valueint;
			}

			updown  = cJSON_GetObjectItem(data, "updown")->valueint;
			cJSON *tmp = cJSON_GetObjectItem(data, "rtMac");
			if (tmp)
				rtMac = tmp->valuestring;

			if (!rtMac){
				LOG("[server] mesh updown msg error!!! can not get rtMac\n");
				cJSON_Delete(data);
				free(msg);
				return NULL;
			}

			if (is_wireless) {
				sprintf(cmd, "/sbin/dev.sh %d %s %s %d %s",
						is_wireless, devMac, dev_name, updown, rtMac);
			}else {
				sprintf(cmd, "/sbin/dev.sh %d %d %d %s",
						is_wireless, devPort, updown, rtMac);
			}

			LOG("[server] mesh exec cmd:%s\n", cmd);
			system(cmd);
			cJSON_Delete(data);
			memset(cmd, 0, sizeof(cmd));
			free(msg);
		}
	}
}

/**
 * Mesh master handle slave device updown event
 */
static void mesh_cb(struct uloop_fd *u, unsigned int events){
	socklen_t sock_len;
	struct sockaddr_in clt_addr;
	char buf[BUFFER_SIZE] = {0};
	int32_t len = 0, ret = -1;
	int32_t fd = u->fd;

	sock_len = sizeof(clt_addr);
	if ((len = recvfrom(fd, buf, BUFFER_SIZE, 0,
					(struct sockaddr*)&clt_addr, &sock_len)) < 0){
		LOG( "[server] mesh receive message fail from:%s, break\n",
				inet_ntoa(clt_addr.sin_addr));
		return;
	}

	LOG( "[server] mesh receive message from:ip %s, len:%d content:%s\n",
			inet_ntoa(clt_addr.sin_addr), len, buf);

	struct mesh_msg *msg = malloc(sizeof(struct mesh_msg));
	msg->fd = fd;
	msg->sock_len = sock_len;
	memcpy(msg->buf, buf, len);
	memcpy(&msg->sock_addr, &clt_addr, sock_len);

	ret = send_mesh_message(g_mesh_msg_queue, (void *)msg);
}

/**
 * Mesh master manage slave device updown event socket server
 */
static int mesh_socket_server(void *args)
{
	int32_t fd;
	struct sockaddr_in srv_addr;
	int32_t val = 1;

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(fd < 0)
	{
		LOG("[server] can't create mesh master socket!\n");
		return -1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val))) {
		LOG("[server] mesh master setsockopt error!\n");
		return -1;
	}

	bzero(&srv_addr, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	srv_addr.sin_port = htons(MESH_PORT);

	if (bind(fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0)
	{
		LOG("[server] mesh master bind error!\n");
		return -1;
	}

	mesh_fd.fd = fd;
	mesh_fd.cb = mesh_cb;
	mesh_fd.registered = false;
	mesh_fd.flags = ULOOP_READ;
	LOG("[server] mesh master socket setup!\n");

	return 0;
}

void init_devlist(){
	system("/sbin/init_devlist");
}

void init_meshlist(){
	system("/sbin/dev.sh 2");
}

bool is_mesh_master(void)
{
	struct uci_package *pkg;
	struct uci_context *ctx = uci_alloc_context();;
	struct uci_section *s;
	struct uci_element *e;
	const char *mode_name, *enable;
	bool ret = false;

	uci_set_confdir(ctx, "/etc/config");
	if (uci_load(ctx, "prplmesh", &pkg) == 0){

		uci_foreach_element(&pkg->sections, e)
		{
			s = uci_to_section(e);
			if (strcmp(s->type, "prplmesh"))
				continue;

			mode_name = uci_lookup_option_string(ctx, s, "management_mode");
			if (!mode_name)
				continue;

			enable = uci_lookup_option_string(ctx, s, "enable");
			if (!enable)
				continue;

			if (strstr(mode_name, "Controller")) {
				if (atoi(enable) == 1)
					ret = true;
				break;
			} else {
				g_mesh_slave = true;
			}
		}
		uci_unload(ctx, pkg);
	}

	uci_free_context(ctx);
	return ret;
}

int32_t main(int32_t argc, char *argv[])
{

	int wds_ret = -1, mesh_ret = -1, iwevent_ret = -1;

	//signal(SIGINT, sighandler);

	init_devlist();

	uloop_init();

	wds_ret = wds_socket_server(NULL);
	if(wds_ret == 0){
		LOG("start listen wds event\n");
		uloop_fd_add(&wds_fd,ULOOP_READ);
	}

	if (is_mesh_master()) {
		init_meshlist();
		mesh_ret = mesh_socket_server(NULL);
		if(mesh_ret == 0){
			LOG("start listen mesh device updown event\n");
			uloop_fd_add(&mesh_fd,ULOOP_READ);
			pthread_t meshThread;
			pthread_attr_t att;
			pthread_attr_init(&att);
			pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED);
			g_mesh_msg_queue = create_mesh_msg_queue();
			pthread_create(&meshThread, &att, mesh_msg_handler, g_mesh_msg_queue);
		}
	}

	iwevent_ret = iwevent_process(NULL);
	if(iwevent_ret == 0){
		LOG("start listen event about ts nl80211 arp dps usb wifi\n");
		uloop_fd_add(&ts_fd,ULOOP_READ);
		uloop_fd_add(&netlink_fd,ULOOP_READ);
		uloop_fd_add(&arp_event_fd,ULOOP_READ);
		uloop_fd_add(&dps_fd,ULOOP_READ);
		uloop_fd_add(&usb_fd,ULOOP_READ);
#ifdef USE_CUSTOM_SOCKET
		uloop_fd_add(&wifi_fd,ULOOP_READ);
#endif
	}

	system("/sbin/dev.sh");
	uloop_run();

	if(wds_ret == 0){
		uloop_fd_delete(&wds_fd);
	}
	if(mesh_ret == 0){
		uloop_fd_delete(&mesh_fd);
		destory_mesh_msg_queue(g_mesh_msg_queue);
	}
	if(iwevent_ret == 0){
		uloop_fd_delete(&ts_fd);
		uloop_fd_delete(&netlink_fd);
		uloop_fd_delete(&arp_event_fd);
		uloop_fd_delete(&dps_fd);
		uloop_fd_delete(&usb_fd);
#ifdef USE_CUSTOM_SOCKET
		uloop_fd_delete(&wifi_fd);
#endif
	}
	uloop_done();

	return 0;
}
