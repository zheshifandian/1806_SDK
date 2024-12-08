#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <json-c/json.h>

#include "ac_mainloop.h"
#include "network.h"
#include "capwap_message.h"
#include "ac_manager.h"
#include "ac_interface.h"
#include "ac_ieee80211.h"
#include "common.h"
#include "ac_log.h"

struct json_operation {
    char *name;
    int (*act)(json_object *, struct interface *);
};

#define OPERATION(func)                     \
	{                                   \
		.name = #func, .act = func, \
	}

static void _capwap_send_interface_result(struct interface *interface, int err);

struct json_object *json_object_object_get_old(struct json_object *obj, const char *name)
{
	struct json_object *sub;
	return json_object_object_get_ex(obj, name, &sub) ? sub : NULL;
}

static const char *get_string_of_json_key(json_object *json, const char *key)
{
	struct json_object *key_obj = json_object_object_get_old(json, key);
	if (!key_obj)
		return NULL;

	return json_object_get_string(key_obj);
}

static struct device_attr *find_device_attr_by_json(json_object *json)
{
	const char *dev_mac = get_string_of_json_key(json, "device");

	return find_dev_attr_by_mac(dev_mac);
}

static struct json_operation *find_operation(struct json_operation *op, const char *name)
{
	int i = 0;

	if (!op || !name)
		return NULL;
	for (i = 0; op[i].name; i++) {
		if (!strcmp(op[i].name, name))
			return &op[i];
	}
	return NULL;
}

static int json_handle(const char *msg, struct json_operation *op, struct interface *interface)
{
	struct capwap_wtp *wtp = interface->is_main ? NONE : interface->arg;
	json_object *msg_obj, *cmd_obj;
	struct json_operation *dev_op;
	const char *command;
	int err;

	CWLog(wtp, "%s: %s\n", __func__, msg);

	msg_obj = json_tokener_parse(msg);
	if (is_error(msg_obj))
		return -EINVAL;

	cmd_obj = json_object_object_get_old(msg_obj, "command");
	if (is_error(cmd_obj))
		return -EINVAL;

	command = json_object_get_string(cmd_obj);
	dev_op = find_operation(op, command);
	if (!dev_op)
		return -EINVAL;

	err = dev_op->act(msg_obj, interface);
	json_object_put(msg_obj);
	return err;
}

int capwap_send_interface_msg(int sock, struct capwap_interface_message *msg, void *data)
{
	struct iovec io[2];
	int count = 1;
	int err;

	if (!msg)
		return 0;
	io[0].iov_base = msg;
	io[0].iov_len = sizeof(*msg);
	if (data && msg->length) {
		io[1].iov_base = data;
		io[1].iov_len = msg->length;
		count++;
	}
	while ((err = writev(sock, io, count)) == -1 && errno == EINTR)
		;
	return err < 0 ? -errno : 0;
}

static void _capwap_send_interface_result(struct interface *interface, int err)
{
	struct capwap_interface_message result;
	struct sockaddr_un unconnect = {0};

	CWLog(NONE, "send interface result: %d", err);
	result.cmd = MSG_END_CMD;
	result.type = MSG_TYPE_RESULT;
	result.length = sizeof(err);

	capwap_send_interface_msg(interface->sock, &result, &err);

	unconnect.sun_family = AF_UNSPEC;
	connect(interface->sock, (void *)&unconnect, sizeof(unconnect));
	interface->client_len = 0;
}

static void capwap_recv_interface(evutil_socket_t sock, short what, void *arg)
{
	struct capwap_interface_message *msg;
	struct interface *interface = arg;
	void *buff;
	int buff_len = 1024, recv_len;
	int err;

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	msg = buff = malloc(buff_len);
	if (!buff) {
		CWLog(NONE, "No memory for interface message");
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		return;
	}
	interface->client_len = sizeof(interface->client);
	recv_len = capwap_recv_message(sock, buff, buff_len, (void *)&interface->client, &interface->client_len);
	if ((recv_len < 0) || (recv_len != msg->length + sizeof(*msg)) || (recv_len >= buff_len) ||
	    connect(sock, (void *)&interface->client, interface->client_len) < 0) {
		CWLog(NONE, "Interface receive message fail");
		free(buff);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		return;
	}

	((char *)buff)[recv_len] = '\0';
	buff += sizeof(*msg);
	err = interface->handle(interface, msg, buff);
	free(msg); // Always need free.
	if (err) {
		_capwap_send_interface_result(interface, err);
		CWWarningLog(NONE, "json_handle() returned with %d", err);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		return;
	}
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
}

int cpawap_send_interface_data(struct interface *interface, void *data, size_t size)
{
	struct capwap_interface_message msg;

	msg.cmd = 0;
	msg.type = MSG_TYPE_STRING;
	msg.length = size;

	return capwap_send_interface_msg(interface->sock, &msg, data);
}

/************************************* WTP interface *************************************/
static int set_device_to_group(json_object *msg, struct interface *interface)
{
	struct capwap_wtp *wtp = interface->arg;
	struct capwap_ac *ac = interface->arg;
	struct device_attr *dev_attr = find_device_attr_by_json(msg);
	struct group_attr *group_attr;
	int i, err = 0;
	int status;

	group_attr = find_group_attr_by_name(
		get_string_of_json_key(msg, "name_of_group"));
	if (!group_attr)
		return -EINVAL;

	dev_attr_mutex_lock(dev_attr);
	status = dev_attr->status;
	if (status == DEV_SETTING) {
		dev_attr_mutex_unlock(dev_attr);
		return -EBUSY;
	}
	set_group_attr_to_device(dev_attr, group_attr);
	group_attr_free(group_attr);

	// If we come from the main thread, just save it.
	// Only do settings in ap control thread.
	if (interface->is_main) {
		save_device_config(dev_attr, 1);
		dev_attr_mutex_unlock(dev_attr);
		_capwap_send_interface_result(&ac->interface, 0);
		return 0;
	}

	for (i = 0; i < WIFI_NUM; i++) {
		if (ac_update_wifi(wtp, i))
			err = -EFAULT;
	}
	if (err) {
		// Reload origin config.
		reload_dev_attr(dev_attr);
		dev_attr_mutex_unlock(dev_attr);
		return err;
	}

	if (!SUPPORT_HOSTAPD(wtp->version)) {
		dev_attr->status = DEV_SETTING;
		err = ac_reset_device(wtp);
	} else {
		err = ac_add_wlan(wtp);
		save_device_config(wtp->attr, 1);
	}
	dev_attr_mutex_unlock(dev_attr);
	if (!err && capwap_interface_in_progress(wtp))
		capwap_send_interface_result(wtp, 0);

	return err;
}

static bool valid_2G_channel(int channel)
{
	return channel >= 1 && channel <= 13;
}

static bool valid_5G_channel(int channel)
{
	int channels[] = {36,40,44,48,149,153,157,161,165};
	int i;

	for (i = 0; i < sizeof(channels) / sizeof(int); i++) {
		if (channel == channels[i])
			return true;
	}
	return false;
}

static bool parse_int_attr(int *attr, int value)
{
	if (*attr == value)
		return false;

	*attr = value;
	return true;
}

#define STRING_EMPTY(s) (!(s) || *(s) == '\0')
static bool parse_string_attr(char **attr, const char *value)
{
	if (STRING_EMPTY(*attr) && STRING_EMPTY(value))
		return false;

	if (STRING_EMPTY(value)) {
		// *attr is not empty here
		FREE_STRING(*attr);
		return true;
	}

	// We know value is not empty now
	// This is the only case we will return with false
	if (*attr && strcmp(*attr, value) == 0)
		return false;

	FREE_STRING(*attr);
	*attr = strdup(value);
	return true;
}

static bool parse_json_dev_config(json_object *config_obj, struct device_attr *attr)
{
	bool changed = false;

	if (!attr || !config_obj)
		return false;

	json_object_object_foreach(config_obj, key, json_obj) {
		if (!strncmp(key, "name", 4)) {
			changed |= parse_string_attr(&attr->name, json_object_get_string(json_obj));
		} else if (!strncmp(key, "ap_alive_time", 13)) {
			changed |= parse_int_attr(&attr->ap_alive_time, json_object_get_int(json_obj));
		} else if (!strncmp(key, "client_alive_time", 17)) {
			changed |= parse_int_attr(&attr->client_alive_time, json_object_get_int(json_obj));
		} else if (!strncmp(key, "client_idle_time", 16)) {
			changed |= parse_int_attr(&attr->client_idle_time, json_object_get_int(json_obj));
		} else if (!strncmp(key, "led", 3)) {
			changed |= parse_int_attr(&attr->led, json_object_get_int(json_obj));
		} else if (!strncmp(key, "custom_wifi", 11)) {
			if (!attr->custom_wifi && strcmp(json_object_get_string(json_obj), "group")) {
				changed = true;
				attr->custom_wifi = "group";
			}
		}
	}
	return changed;
}

static bool parse_json_wifi_config(json_object *wifi_obj, struct wifi_attr *attr, int band)
{
	bool changed = false, enable_changed = false, valid;
	bool weak_signal_changed = false, weak_sta_changed = false;
	int old_encryption, channel;

	if (!attr || !json_object_is_type(wifi_obj, json_type_object))
		return false;

	old_encryption = attr->encryption;
	json_object_object_foreach(wifi_obj, key, json_obj) {
		if (!strncmp(key, "enabled", 7)) {
			changed |= parse_int_attr(&attr->enabled, json_object_get_int(json_obj));
		} else if (!strncmp(key, "ssid", 4)) {
			if (strlen(json_object_get_string(json_obj)) <= 32)
				changed |= parse_string_attr(&attr->ssid, json_object_get_string(json_obj));
		} else if (!strncmp(key, "hide", 4)) {
			changed |= parse_int_attr(&attr->hide, json_object_get_int(json_obj));
		} else if (!strncmp(key, "bandwidth_limit", 15)) {
			changed |= parse_int_attr(&attr->bandwidth_limit, json_object_get_int(json_obj));
		} else if (!strncmp(key, "bandwidth_upload", 16)) {
			changed |= parse_int_attr(&attr->bandwidth_upload, json_object_get_int(json_obj));
		} else if (!strncmp(key, "bandwidth_download", 18)) {
			changed |= parse_int_attr(&attr->bandwidth_download, json_object_get_int(json_obj));
		} else if (!strncmp(key, "encryption", 10)) {
			const char *encry = json_object_get_string(json_obj);
			if (!strncmp(encry, "open", 4))
				attr->encryption = ENCRYPTION_OPEN;
			else
				attr->encryption = ENCRYPTION_WPA2_PSK;
		} else if (!strncmp(key, "password", 8)) {
			changed |= parse_string_attr(&attr->password, json_object_get_string(json_obj));
		} else if (!strncmp(key, "channel", 7)) {
			channel = json_object_get_int(json_obj);
			if (band == WIFI_2G)
				valid = valid_2G_channel(channel);
			else
				valid = valid_5G_channel(channel);
			if ((channel == 0) || valid)
				changed |= parse_int_attr(&attr->channel, channel);
		} else if (!strncmp(key, "country_code", 12)) {
			changed |= parse_string_attr(&attr->country_code, json_object_get_string(json_obj));
		} else if (!strncmp(key, "weak_sta_signal_enable", 22)) {
			enable_changed |= parse_int_attr(&attr->weak_sta_signal_enable, json_object_get_int(json_obj));
		} else if (!strncmp(key, "prohibit_sta_signal_enable", 26)) {
			enable_changed |= parse_int_attr(&attr->prohibit_sta_signal_enable, json_object_get_int(json_obj));
		} else if (!strncmp(key, "weak_sta_signal", 15)) {
			weak_signal_changed |= parse_int_attr(&attr->weak_sta_signal, json_object_get_int(json_obj));
		} else if (!strncmp(key, "prohibit_sta_signal", 19)) {
			weak_sta_changed |= parse_int_attr(&attr->prohibit_sta_signal, json_object_get_int(json_obj));
		} else if (!strncmp(key, "bandwidth", 10)) {
			changed |= parse_string_attr(&attr->bandwidth, json_object_get_string(json_obj));
		}
	}
	// If no password is specified, we set encryption to "open".
	if (!attr->password)
		attr->encryption = ENCRYPTION_OPEN;
	// Only judge signal related changes when they are enabled.
	if (enable_changed)
		changed = true;
	if (attr->weak_sta_signal_enable && weak_signal_changed)
		changed = true;
	if (attr->prohibit_sta_signal_enable && weak_sta_changed)
		changed = true;
	if (attr->encryption != old_encryption)
		changed = true;
	return changed;
}

static int set_device_config(json_object *msg, struct interface *interface)
{
	struct capwap_wtp *wtp = interface->arg;
	struct device_attr *dev_attr;
	struct json_object *dev_config, *wifi_config;
	char *wifi_config_name[WIFI_NUM];
	bool changed;
	int i, err = 0;

	dev_attr = find_device_attr_by_json(msg);
	if (!dev_attr)
		return -EINVAL;
	dev_config = json_object_object_get_old(msg, "dev_config");
	if (dev_config) {
		dev_attr_mutex_lock(dev_attr);
		changed = parse_json_dev_config(dev_config, dev_attr);
		dev_attr_mutex_unlock(dev_attr);
		err = ac_set_dev_attr(wtp, changed);
		if (err)
			goto err_out;
	}

	wifi_config_name[WIFI_2G] = "wifi_2g_config";
	wifi_config_name[WIFI_5G] = "wifi_5g_config";
	for (i = 0; i < WIFI_NUM; i++) {
		wifi_config = json_object_object_get_old(msg, wifi_config_name[i]);
		if (wifi_config) {
			dev_attr_mutex_lock(dev_attr);
			changed = parse_json_wifi_config(wifi_config, &dev_attr->wifi.band[i], i);
			dev_attr_mutex_unlock(dev_attr);
			if (!SUPPORT_WEAK_SIGNAL(wtp->version)) {
				if (dev_attr->wifi.band[i].weak_sta_signal_enable ||
				    dev_attr->wifi.band[i].prohibit_sta_signal_enable) {
					err = -35;
					goto err_out;
				}
			}
			if (changed) {
				dev_attr->custom_wifi = NULL;
				ac_update_wifi(wtp, i);
				save_device_config(dev_attr, 0);
			}
		}
	}

	if (SUPPORT_HOSTAPD(wtp->version)) {
		err = ac_add_wlan(wtp);
	} else {
		// status is DEV_RUNNING means we didn't change device_attr, so we should reset
		// device to apply wifi changes.
		if (dev_attr->status == DEV_RUNNING) {
			dev_attr->status = DEV_SETTING;
			err = ac_reset_device(wtp);
		} else {
			err = save_device_config(dev_attr, 0);
		}
	}
	// It may takes a long time to get result from wtp, send interface result here to avoid
	// long waiting
	if (capwap_interface_in_progress(wtp))
		capwap_send_interface_result(wtp, err);

	return err;

err_out:
	dev_attr_mutex_lock(dev_attr);
	reload_dev_attr(dev_attr);
	dev_attr_mutex_unlock(dev_attr);
	return err;
}

static int do_wtp_command(json_object *msg, struct interface *interface)
{
	struct capwap_wtp *wtp = interface->arg;
	struct device_attr *attr = find_device_attr_by_json(msg);
	const char *command;

	if (!attr)
		return -EINVAL;

	command = get_string_of_json_key(msg, "wtp_command");
	if (!command)
		return -EINVAL;

	return ac_do_wtp_command(wtp, command);
}

// send update request
static int ap_update(json_object *msg, struct interface *interface)
{
	struct capwap_wtp *wtp = interface->arg;
	struct device_attr *attr = find_device_attr_by_json(msg);
	struct ap_update update;
	struct cw_ctrlmsg *ctrlmsg;
	int err;

	if (!attr)
		return -EINVAL;

	ctrlmsg = cwmsg_ctrlmsg_new(CW_TYPE_CONFIGURE_UPDATE_REQUEST, get_seq_num(wtp));
	update.path = (char *)get_string_of_json_key(msg, "path");
	update.md5 = (char *)get_string_of_json_key(msg, "md5");
	update.version = (char *)get_string_of_json_key(msg, "version");
	if (!ctrlmsg || !update.path || !update.md5 || !update.version)
		return -EINVAL;

	err = cwmsg_assemble_ap_update(ctrlmsg, &update);
	if (err)
		return err;
	err = capwap_send_request(wtp, ctrlmsg);
	if (err)
		return err;
	capwap_set_state(wtp, UPDATE);
	wtp->attr->updating = WTP_UPDATE_INPROCESS;
	save_device_config(wtp->attr, 0);
	return 0;
}

static struct json_operation device_operations[] = {
	OPERATION(set_device_to_group),
	OPERATION(set_device_config),
	OPERATION(do_wtp_command),
	OPERATION(ap_update),
	{NULL, NULL},
};

void capwap_send_interface_result(struct capwap_wtp *wtp, int err)
{
	_capwap_send_interface_result(&wtp->interface, err);
}

static void pong(struct capwap_wtp *wtp)
{
	struct capwap_interface_message result;

	CWLog(wtp, "send interface pong");
	result.cmd = PONG_CMD;
	result.type = wtp->state == RUN;
	result.length = 0;

	capwap_send_interface_msg(wtp->interface.sock, &result, NULL);
}

static int capwap_wtp_handle_interface(struct interface *interface, struct capwap_interface_message *msg,
					void *payload)
{
	struct capwap_wtp *wtp = interface->arg;

	if ((wtp->state != RUN) && (msg->cmd != PING_CMD))
		return -EBUSY;
	CWLog(wtp, "wtp handle interface cmd: %d", msg->cmd);
	switch (msg->cmd) {
	case JSON_CMD:
		return json_handle(payload, device_operations, interface);
	case PING_CMD:
		pong(wtp);
		break;
	case QUIT_CMD:
		ac_reset_device(wtp);
		_capwap_send_interface_result(interface, 0);
		break;
	}

	return 0;
}

int capwap_init_wtp_interface(struct capwap_wtp *wtp)
{
	uint8_t *mac;
	int sock;

	if (!wtp)
		return -EINVAL;

	mac = wtp->board_data.mac;
	bzero(wtp->if_path, sizeof(wtp->if_path));
	snprintf(wtp->if_path, sizeof(wtp->if_path), "/tmp/WTP.%02hhx_%02hhx_%02hhx_%02hhx_%02hhx_%02hhx",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	if ((sock = capwap_init_interface_sock(wtp->if_path)) < 0) {
		CWLog(wtp, "create interface socket fail");
		return sock;
	}

	wtp->interface.is_main = 0;
	wtp->interface.sock = sock;
	wtp->interface.handle = capwap_wtp_handle_interface;
	wtp->interface.arg = wtp;
	wtp->interface.listener = event_new(wtp->ev_base, sock, EV_READ | EV_PERSIST, capwap_recv_interface, &wtp->interface);
	if (!wtp->interface.listener) {
		perror("Couldn't create listener");
		return -errno;
	};
        event_add(wtp->interface.listener, NULL);
	CWLog(wtp, "Create %s interface", wtp->if_path);

	return 0;
}

void capwap_destroy_wtp_interface(struct capwap_wtp *wtp)
{
	if (wtp->interface.listener) {
		event_free(wtp->interface.listener);
		close(wtp->interface.sock);
		wtp->interface.listener = NULL;
	}
	// unlink(wtp->if_path);
}

int capwap_wtp_send_ap_roam_event(struct capwap_wtp *wtp, struct mac_event *event, int num)
{
	struct capwap_interface_message msg = {0};
	int err = 0;
	int i, send = 0;

	if (!SUPPORT_AP_ROAM(wtp->version))
		return 0;

	msg.cmd = AP_ROAM_CMD;
	msg.length = sizeof(msg);

	for (i = 0; i < num; i++) {
		if (event[i].event == ADD)
			send = 1;
	}
	if (send)
		err = capwap_send_interface_msg(wtp->wtp_client->wtp_pipe, &msg, NULL);
	return err;
}

static void capwap_group_change(struct capwap_wtp *wtp, char *group_name)
{
	struct json_object *json = json_object_new_object();

	if (!json)
		return;
	if (use_group_wifi_attr(wtp->attr) && wtp->attr->group &&
	    !strcmp(wtp->attr->group, group_name)) {
		json_object_object_add(json, "name_of_group", json_object_new_string(group_name));
		json_object_object_add(json, "device", json_object_new_string(wtp->attr->mac));
		set_device_to_group(json, &wtp->interface);
	}
	json_object_put(json);
	return;
}

static void capwap_group_delete(struct capwap_wtp *wtp)
{
	struct json_object *json = json_object_new_object();

	if (!json)
		return;
	json_object_object_add(json, "name_of_group", json_object_new_string("default"));
	json_object_object_add(json, "device", json_object_new_string(wtp->attr->mac));
	set_device_to_group(json, &wtp->interface);
	json_object_put(json);
	return;
}

/**
 * This function is used to handle message comes from the main thread
 */
void capwap_handle_ac_command(evutil_socket_t sock, short what, void *arg)
{
	struct capwap_wtp *wtp = arg;
	struct capwap_interface_message *ac_msg;
	struct cw_ctrlmsg *msg;
	void *buff = NULL, *payload;
	int buf_len = 128;

	if (!(what & EV_READ))
		return;

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	buff = malloc(buf_len);
	if (!buff) {
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		return;
	}
	if (read(sock, buff, buf_len) < 0) {
		free(buff);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		return;
	}
	ac_msg = buff;
	payload = MSG_GET_DATA(ac_msg);
	CWLog(wtp, "handle ac command:%d", ac_msg->cmd);
	switch (ac_msg->cmd) {
	case REQUEST_CMD:
		memcpy(&msg, payload, sizeof(msg));
		capwap_do_send_request(wtp, msg);
		break;
	case AP_ROAM_CMD:
		if (!SUPPORT_AP_ROAM(wtp->version))
			break;
		if (capwap_wtp_send_mac_list(wtp)) {
			CWLog(wtp, "send roam mac list fail");
		}
		event_add(wtp->ac_ev, NULL);
		break;
	case GROUP_CHANGE:
		capwap_group_change(wtp, payload);
		event_add(wtp->ac_ev, NULL);
		break;
	case GROUP_DELETE:
		capwap_group_delete(wtp);
		event_add(wtp->ac_ev, NULL);
		break;
	case QUIT_CMD:
		capwap_set_state(wtp, QUIT);
		event_active(wtp->ctrl_ev, EV_WRITE, 0);
		break;
	default:
		break;
	}
	free(buff);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
}

/************************************* AC main interface *************************************/

extern pthread_mutex_t wtp_mutex;
static int delete_device(json_object *msg, struct interface *interface)
{
	struct capwap_ac *ac = interface->arg;
	struct device_attr *attr = find_device_attr_by_json(msg);
	int err = 0;

	if (!attr) {
		_capwap_send_interface_result(&ac->interface, 0);
		return 0;
	}

	dev_attr_mutex_lock(attr);
	if ((attr->status == DEV_RUNNING) || (attr->status == DEV_SETTING)) {
		dev_attr_mutex_unlock(attr);
		return -EBUSY;
	}

	err = delete_device_config(attr);
	dev_attr_mutex_unlock(attr);

	if (err)
		return err;

	dev_attr_free(attr);
	free(attr);

	_capwap_send_interface_result(&ac->interface, err);

	return err;
}

static int set_group_name(json_object *msg, struct interface *interface)
{
	struct capwap_ac *ac = interface->arg;
	struct device_attr *dev_attr;
	const char *name;
	int err = 0;

	dev_attr = find_device_attr_by_json(msg);
	if (!dev_attr)
		return -EINVAL;

	name = get_string_of_json_key(msg, "name_of_group");
	if (!name)
		return -EINVAL;

	dev_attr_mutex_lock(dev_attr);
	FREE_STRING(dev_attr->group);
	dev_attr->group = strdup(name);
	err = save_device_config(dev_attr, 1);
	dev_attr_mutex_unlock(dev_attr);
	_capwap_send_interface_result(&ac->interface, err);

	return err;
}

static int ap_group_event(json_object *json_msg, struct interface *interface, uint32_t event)
{
	struct capwap_ac *ac = interface->arg;
	struct wtp_client *client;
	struct device_attr *dev_attr;
	struct capwap_interface_message msg = {0};
	const char *group;

	group = get_string_of_json_key(json_msg, "name_of_group");
	if (!group)
		return -EINVAL;
	msg.cmd = event;
	msg.length = strlen(group) + 1;
	pthread_mutex_lock(&wtp_mutex);
	list_for_each_entry(client, &ac->wtp_list, list) {
		dev_attr = client->wtp->attr;
		if (!dev_attr)
			continue;
		dev_attr_mutex_lock(dev_attr);
		if (strcmp(group, dev_attr->group) == 0)
			capwap_send_interface_msg(client->main_pipe, &msg, (void *)group);
		dev_attr_mutex_unlock(dev_attr);
	}
	pthread_mutex_unlock(&wtp_mutex);
	_capwap_send_interface_result(&ac->interface, 0);
	return 0;
}

static int ap_group_change(json_object *json_msg, struct interface *interface) {
	return ap_group_event(json_msg, interface, GROUP_CHANGE);
}

static int ap_group_delete(json_object *json_msg, struct interface *interface) {
	return ap_group_event(json_msg, interface, GROUP_DELETE);
}

static struct json_operation ac_operations[] = {
	OPERATION(set_device_to_group),
	OPERATION(set_group_name),
	OPERATION(ap_group_change),
	OPERATION(ap_group_delete),
	OPERATION(delete_device),
	{NULL, NULL},
};

static void print_station(struct station *sta, void *arg)
{
	struct interface *interface = arg;
	char *band;
	char *msg;
	int msg_len = 64;

	msg = MALLOC(msg_len);
	if (!msg)
		return;
	if (sta->radio_id == WIFI_5G)
		band = "5G";
	else if (sta->radio_id == WIFI_2G)
		band = "2.4G";
	else
		band = "unknown band";
	snprintf(msg, msg_len, "sta:" MACSTR ", ap:%s@%s", MAC2STR(sta->mac),
		 sta->wtp->ip_addr, band);
	cpawap_send_interface_data(interface, msg, msg_len);
	free(msg);
}

static int capwap_main_handle_interface(struct interface *interface, struct capwap_interface_message *msg,
					void *payload)
{
	struct capwap_ac *ac = interface->arg;

	CWLog(NONE, "ac handle interface cmd: %d", msg->cmd);
	switch (msg->cmd) {
	case JSON_CMD:
		return json_handle(payload, ac_operations, interface);
	case QUIT_CMD:
		event_base_loopbreak(ac->base);
		_capwap_send_interface_result(interface, 0);
		break;
	case PRINT_STATIONS:
		station_for_each(print_station, interface, 0);
		_capwap_send_interface_result(interface, 0);
		break;
	}

	return 0;
}

int capwap_init_main_interface(struct capwap_ac *ac)
{
	int sock;

	snprintf(ac->if_path, sizeof(ac->if_path), AC_MAIN_INTERFACE);
	if ((sock = capwap_init_interface_sock(ac->if_path)) < 0) {
		CWCritLog(NONE, "Create interface socket fail");
		return sock;
	}

	ac->interface.is_main = 1;
	ac->interface.sock = sock;
	ac->interface.arg = ac;
	ac->interface.handle = capwap_main_handle_interface;
	ac->interface.listener = event_new(ac->base, sock, EV_READ | EV_PERSIST, capwap_recv_interface, &ac->interface);
	if (!ac->interface.listener) {
		perror("Couldn't create listener");
		return errno;
	};
        return event_add(ac->interface.listener, NULL);
}

void capwap_destroy_main_interface(struct capwap_ac *ac)
{
	event_free(ac->interface.listener);
	close(ac->interface.sock);
	unlink(ac->if_path);
}

pthread_mutex_t wtp_mutex = PTHREAD_MUTEX_INITIALIZER;

struct wtp_client *wtp_client_new()
{
	struct wtp_client *client;

	client = MALLOC(sizeof(*client));
	return client;
}

void wtp_client_free(struct wtp_client *client)
{
	if (!client)
		return;

	close(client->wtp_pipe);
	close(client->main_pipe);
	if (client->ac_event) {
		event_free(client->ac_event);
	}
	free(client);
}

void wtp_list_lock()
{
	pthread_mutex_lock(&wtp_mutex);
}

void wtp_list_unlock()
{
	pthread_mutex_unlock(&wtp_mutex);
}

void wtp_list_del(struct wtp_client *client)
{
	pthread_mutex_lock(&wtp_mutex);
	pthread_detach(client->thread);
	list_del(&client->list);
	wtp_client_free(client);
	pthread_mutex_unlock(&wtp_mutex);
}

void wtp_list_add(struct capwap_ac *ac, struct wtp_client *client)
{
	pthread_mutex_lock(&wtp_mutex);
	list_add_tail(&client->list, &ac->wtp_list);
	pthread_mutex_unlock(&wtp_mutex);
}

int capwap_send_discovery_request(struct wtp_client *client)
{
	struct capwap_interface_message msg = {0};

	msg.cmd = DISCOVERY_REQ;
	msg.length = 0;
	return capwap_send_interface_msg(client->main_pipe, &msg, NULL);
}

void capwap_handle_wtp(evutil_socket_t sock, short what, void *arg)
{
	struct wtp_client *client = arg;
	struct capwap_ac *ac = client->ac;
	struct capwap_interface_message *msg;
	struct timeval t = {10, 0};
	void *buff;
	int buff_len = 256;
	int msg_len;

	if (what & EV_READ) {
		buff = malloc(buff_len);
		if (!buff)
			return;
		if ((msg_len = read(sock, buff, buff_len)) < 0) {
			CWLog(NONE, "read socketpair error");
			free(buff);
			return;
		}
		msg = buff;
		switch (msg->cmd) {
		case AP_ROAM_CMD:
			wtp_list_lock();
			list_for_each_entry (client, &ac->wtp_list, list) {
				if (SUPPORT_AP_ROAM(client->wtp->version)) {
					client->state = RECV_AP_ROAM;
					event_add(client->ac_event, &t);
				}
			}
			wtp_list_unlock();
			break;

		default:
			break;
		}
		free(buff);
	}
	if (what & EV_TIMEOUT) {
		if (client->state == RECV_AP_ROAM) {
			struct capwap_interface_message roam_msg = {0};
			roam_msg.cmd = AP_ROAM_CMD;
			roam_msg.length = sizeof(roam_msg);
			capwap_send_interface_msg(client->main_pipe, &roam_msg, NULL);
			client->state = AC_RUN;
		}
	}
	return;
}
