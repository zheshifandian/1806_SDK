#ifndef __ANDLINK__H
#define __ANDLINK__H
#include <libubus.h>
#include <pthread.h>
#include <coap2/coap.h>
#define ANDLINK_CONFIG_PATH "/etc/config"
#define ANDLINK_CONFIG_NAME "andlink"
#define ANDLINK_CONFIG_SECTION "andlink"
#define DEVICE_TYPE		"500301"
#define PRODUCT_TOKEN	"ApxHo30ycbWN4hx3"
#define BUTTON_LISTENER_NAME	"andlink"

struct andlink {
	pthread_mutex_t lock;
	pthread_mutex_t wifi_lock;
	pthread_t listener;
	pthread_t wifi;
	pthread_t coap;
	pthread_cond_t cond;
	pthread_cond_t listener_cond;
	pthread_cond_t wifi_cond;
	struct ubus_object object;
	struct uloop_fd waker_fd;
	int waker_pipe;
	int coap_type;
	int coap_quit;
	void (*wifi_connect_callback)(void *);
	coap_address_t gw_addr;
	bool gw_detected;
	bool coap_request;
	bool listener_thread_inited;
	bool wifi_thread_inited;
	bool exit;
	int wifi_mode;
	bool change_wifi_state;
};
#endif
