#include <stdio.h>
#include <string.h>
#include <uci.h>
#include <fcntl.h>
#include <json-c/json_tokener.h>
#include <json-c/json_object.h>
#include "utils.h"
#include "log.h"
#include "wifi.h"

static struct ubus_context *ctx = NULL;
static struct andlink *priv = NULL;

/*
* return -1 : some error
* 		  1 : config missing
*		  0 : device has configed
*/
int check_andlink_config()
{
	struct uci_package *p = NULL;
	char *config[] =
	{
		"gwtoken",
		"deviceid",
		"devicetoken",
		"andlinktoken",
		"SSID",
		"password",
		"encrypt",
		"channel"
		"",
	};

	char *str;
	int ret = 0, i = 0;
	struct uci_context *ctx = uci_alloc_context();
	if (!ctx) {
		LOG("%s out of memory\n", __func__);
		return -ENOMEM;
	}
	uci_set_confdir(ctx, ANDLINK_CONFIG_PATH);
	if(uci_load(ctx, ANDLINK_CONFIG_NAME, &p) == UCI_OK) {
		struct uci_section *section = uci_lookup_section(ctx, p, ANDLINK_CONFIG_SECTION);
		if ( section != NULL) {
			while (strcmp(config[i], "")) {
				str = uci_lookup_option_string(ctx, section, config[i]);
				LOG("%s str %s\n", __func__, str);
				if ((str && !strcmp(str, "")) || !str) {
					ret = 1;
					break;
				}
				i++;
			}

		} else {
			LOG("%s cann't get %s from config file\n", __func__, ANDLINK_CONFIG_SECTION);
			ret = -1;
		}
		uci_unload(ctx, p);
	} else {
		ret = -1;
	}
	uci_free_context(ctx);
	return ret;
}


int save_andlink_config(char *option_name, char *value)
{
	struct uci_ptr ptr;
	char *str;
	int ret = 0, i = 0;
	char option[128] = { 0 };
	struct uci_context *ctx = uci_alloc_context();

	memset(&ptr, 0 ,sizeof(ptr));
	if (!ctx) {
		LOG("%s out of memory\n", __func__);
		return -ENOMEM;
	}
	uci_set_confdir(ctx, ANDLINK_CONFIG_PATH);
	sprintf(option, "%s.%s.%s", ANDLINK_CONFIG_NAME, ANDLINK_CONFIG_SECTION, option_name);
	if(UCI_OK != uci_lookup_ptr(ctx, &ptr, option, true)) {
		LOG("%s can not get option %s\n", __func__, option);
		ret = -1;
		goto exit;
	}
	ptr.value = value;
	ret = uci_set(ctx,&ptr);
	if (0 == ret){
		ret = uci_commit(ctx, &ptr.p, false);
	}
	uci_unload(ctx, ptr.p);

exit:
	uci_free_context(ctx);
	return ret;
}


enum {
	LISTENER_NOTIFY,
	__LISTENER_MAX,
};

static const struct blobmsg_policy listener_notify_policy[] = {
	[LISTENER_NOTIFY]     = { .name = "sec", .type = BLOBMSG_TYPE_INT32 },
};

static int listener_notify(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
	//recieved a button evt, we should change to machine to quick connection mode
	struct blob_attr *tb[__LISTENER_MAX];
	blobmsg_parse(listener_notify_policy, ARRAY_SIZE(listener_notify_policy), tb, blob_data(msg), blob_len(msg));
	LOG("%s recieved button notify\n", __func__);
	pthread_mutex_lock(&priv->wifi_lock);
	priv->change_wifi_state = true;
	priv->wifi_mode = QUICK_LINK;
	pthread_cond_signal(&priv->wifi_cond);
	pthread_mutex_unlock(&priv->wifi_lock);
	return 0;
}

static const struct ubus_method listener_methods[] = {
	UBUS_METHOD("button_notify", listener_notify, listener_notify_policy),
};

static void listener_reconnect_timer(struct uloop_timeout *timeout)
{
	int t = 2;
	static struct uloop_timeout retry = {
		.cb = listener_reconnect_timer,
	};

	if (ubus_reconnect(ctx, NULL) != 0) {
		LOG("%s failed to reconnect, trying again in %d seconds\n", __func__, t);
		uloop_timeout_set(&retry, t * 1000);
		return ;
	}

	LOG("%s reconnected to ubus\n", __func__);
	ubus_add_uloop(ctx);
}

static void listener_connection_lost(struct ubus_context *ctx)
{
	listener_reconnect_timer(NULL);
}

static void waker_consume(struct uloop_fd *fd, unsigned int events)
{
	char buf[4];
	while (read(fd->fd, buf, 4) > 0)
		;
}

static void waker_init_fd(int fd)
{
	fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

static void wakeup_fd_destroy(struct andlink *andlink)
{
	if (andlink->waker_pipe >= 0) {
		uloop_fd_delete(&andlink->waker_fd);
		close(andlink->waker_pipe);
		close(andlink->waker_fd.fd);
		andlink->waker_pipe = -1;
	}
}

static int wakeup_fd_init(struct andlink *andlink)
{
	int fds[2];
	if (pipe(fds) < 0)
		return -1;
	waker_init_fd(fds[0]);
	waker_init_fd(fds[1]);
	andlink->waker_pipe = fds[1];

	andlink->waker_fd.fd = fds[0];
	andlink->waker_fd.cb = waker_consume;
	uloop_fd_add(&andlink->waker_fd, ULOOP_READ);
	return 0;
}

void wakeup_exit_uloop(struct andlink *andlilnk)
{
	uloop_end();
	write(priv->waker_pipe, "w", 1);
}

static struct ubus_object_type listener_object_type =
 UBUS_OBJECT_TYPE(BUTTON_LISTENER_NAME, listener_methods);

void listener_thread(void *paras)
{
	struct andlink *andlink = (struct andlink *)paras;
	char *name = NULL;
	int ret;
	if (uloop_init()) {
		LOG("%s uloop init failed\n", __func__);
		goto no_loop;
	}

	if (wakeup_fd_init(andlink)) {
		LOG("%s init wakeup fd failed\n", __func__);
		goto no_wakeup_fd;
	}

	ctx = ubus_connect(NULL);
	if (!ctx) {
		LOG("%s ubus connect failed\n", __func__);
		goto no_ctx;
	}
	ctx->connection_lost = listener_connection_lost;
	ubus_add_uloop(ctx);
	if (asprintf(&name, BUTTON_LISTENER_NAME) < 0) {
		LOG("%s oom \n", __func__);
		goto exit;
	}
	andlink->object.name = name;
	andlink->object.type = &listener_object_type;
	andlink->object.methods = listener_methods;
	andlink->object.n_methods = ARRAY_SIZE(listener_methods);
	ret = ubus_add_object(ctx, &andlink->object);
	if (ret) {
		LOG("%s add object failed\n", __func__);
		goto exit;
	}
	pthread_mutex_lock(&andlink->lock);
	andlink->listener_thread_inited = true;
	pthread_cond_signal(&andlink->listener_cond);
	pthread_mutex_unlock(&andlink->lock);
	priv = andlink;
	LOG("%s button listener started\n", __func__);
	uloop_run();
exit:
	LOG("%s exit\n", BUTTON_LISTENER_NAME);
	if (name)
		free(name);
	ubus_free(ctx);
no_ctx:
	pthread_mutex_lock(&andlink->lock);
	wakeup_fd_destroy(andlink);
	pthread_mutex_unlock(&andlink->lock);
no_wakeup_fd:
	uloop_done();
no_loop:
	pthread_mutex_lock(&andlink->lock);
	pthread_cond_signal(&andlink->listener_cond);
	pthread_mutex_unlock(&andlink->lock);

	return;
}

int remove_ssid()
{

}

int parse_net_data(char *buf, uint32_t size)
{
	json_tokener* tok;
	json_object* main_json;
	json_object* ssid;
	json_object* password;
	json_object* encrypt;
	json_object* channel;
	int ret = 0;
	tok = json_tokener_new();
	if (!tok) {
		LOG("%s new json tokener failed\n ", __func__);
		return -1;
	}
	main_json = json_tokener_parse_ex(tok, buf, size);
	if (tok->err != json_tokener_success) {
		LOG("%s invalid json data (%s)\n", __func__, buf);
		return -1;
	}
	json_tokener_free(tok);
	if (!json_object_object_get_ex(main_json, "SSID", &ssid)) {
		LOG("%s can not get SSID from net data\n", __func__);
		ret = -1;
		goto exit;
	}
	LOG("%s get SSID %s from net data\n", __func__, json_object_get_string(ssid));
	if (!json_object_object_get_ex(main_json, "password", &password)) {
		LOG("%s can not get password from net data\n", __func__);
		ret = -1;
		goto exit;
	}
	LOG("%s get password %s from net data\n", __func__, json_object_get_string(password));

	if (!json_object_object_get_ex(main_json, "encrypt", &encrypt)) {
		LOG("%s can not get encrypt from net data\n", __func__);
		ret = -1;
		goto exit;
	}
	LOG("%s get encrypt %s from net data\n", __func__, json_object_get_string(encrypt));
	if (!json_object_object_get_ex(main_json, "channel", &channel)) {
		LOG("%s can not get encrypt from net data\n", __func__);
		ret = -1;
		goto exit;
	}
	LOG("%s get channel %s from net data\n", __func__, json_object_get_string(channel));
	save_andlink_config("SSID", json_object_get_string(ssid));
	save_andlink_config("password", json_object_get_string(password));
	save_andlink_config("encrypt", json_object_get_string(encrypt));
	save_andlink_config("channel", json_object_get_string(channel));
exit:
	json_object_put(main_json);
	return ret;
}
