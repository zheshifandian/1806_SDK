#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <uci.h>
#include "uci_config.h"
#include "CWLog.h"

static struct uci_context *ctx = NULL;
static struct uci_package *device_pkg = NULL;
static LIST_HEAD(device_list);
static LIST_HEAD(save_list);
static pthread_spinlock_t dev_list_mutex;
static pthread_mutex_t dev_uci_mutex;
static pthread_mutex_t save_mutex;
static pthread_cond_t save_cond = PTHREAD_COND_INITIALIZER;

static inline void dev_list_add_tail(struct list_head *new, struct list_head *head)
{
	pthread_spin_lock(&dev_list_mutex);
	list_add_tail(new, head);
	pthread_spin_unlock(&dev_list_mutex);
}

static inline void dev_list_del(struct list_head *entry)
{
	pthread_spin_lock(&dev_list_mutex);
	list_del(entry);
	pthread_spin_unlock(&dev_list_mutex);
}

// section will be changed after uci_commit, so we have to update them.
static inline int device_uci_commit()
{
	int err;

	if ((err = uci_save(ctx, device_pkg))) {
		CWCritLog("uci_save failed with %d", err);
		return err;
	}
	if ((err = uci_commit(ctx, &device_pkg, true))) {
		CWCritLog("uci_commit failed with %d", err);
		return err;
	}
	return 0;
}

static char *attr_itoa(void *value)
{
	char *buf = malloc(16);
	if (!buf)
		return NULL;
	memset(buf, 0, 16);
	snprintf(buf, 16, "%d", *(int *)value);

	return buf;
}

static int attr_atoi(const char *value)
{
	if (!value)
		return 0;
	return atoi(value);
}

static char *attr_strdup(const char *value)
{
	if (!value)
		return NULL;
	return strdup(value);
}

#define BUILD_INT_CONVERT(dev, type)                       \
	static char *type##_itoa(void *data)                   \
	{                                                      \
		struct dev##_attr *attr = data;                    \
		return attr_itoa(&attr->type);                     \
	}                                                      \
	static void type##_atoi(void *data, const char *value) \
	{                                                      \
		struct dev##_attr *attr = data;                    \
		attr->type = attr_atoi(value);                     \
	}

#define BUILD_DEV_INT_CONVERT(type) BUILD_INT_CONVERT(device, type)
#define BUILD_WLAN_INT_CONVERT(type) BUILD_INT_CONVERT(wifi, type)
BUILD_DEV_INT_CONVERT(status)
BUILD_DEV_INT_CONVERT(ap_alive_time)
BUILD_DEV_INT_CONVERT(client_alive_time)
BUILD_DEV_INT_CONVERT(client_idle_time)
BUILD_DEV_INT_CONVERT(led)
BUILD_DEV_INT_CONVERT(updating)
BUILD_WLAN_INT_CONVERT(enabled)
BUILD_WLAN_INT_CONVERT(hide)
BUILD_WLAN_INT_CONVERT(isolation)
BUILD_WLAN_INT_CONVERT(bandwidth_limit)
BUILD_WLAN_INT_CONVERT(bandwidth_upload)
BUILD_WLAN_INT_CONVERT(bandwidth_download)
BUILD_WLAN_INT_CONVERT(channel)
BUILD_WLAN_INT_CONVERT(weak_sta_signal_enable)
BUILD_WLAN_INT_CONVERT(prohibit_sta_signal_enable)
BUILD_WLAN_INT_CONVERT(weak_sta_signal)
BUILD_WLAN_INT_CONVERT(prohibit_sta_signal)

static char *encryption_to_uci(void *data)
{
	struct wifi_attr *attr = data;
	switch (attr->encryption) {
	case ENCRYPTION_OPEN:
		return strdup("open");
	case ENCRYPTION_WPA2_PSK:
		return strdup("wpa2-psk");
	default:
		return NULL;
	}
	return NULL;
}

static void encryption_to_attr(void *data, const char *value)
{
	struct wifi_attr *attr = data;
	if (!strcmp(value, "open"))
		attr->encryption = ENCRYPTION_OPEN;
	else
		attr->encryption = ENCRYPTION_WPA2_PSK;
}

#define BUILD_STRING_CONVERT(dev, type)                           \
	static char *type##_to_uci(void *data)                    \
	{                                                         \
		struct dev##_attr *attr = data;                   \
		return attr_strdup(attr->type);                   \
	}                                                         \
	static void type##_to_attr(void *data, const char *value) \
	{                                                         \
		struct dev##_attr *attr = data;                   \
		attr->type = attr_strdup(value);                  \
	}
#define BUILD_DEV_STRING_CONVERT(type) BUILD_STRING_CONVERT(device, type)
#define BUILD_WLAN_STRING_CONVERT(type) BUILD_STRING_CONVERT(wifi, type)
BUILD_DEV_STRING_CONVERT(name)
BUILD_DEV_STRING_CONVERT(model)
BUILD_DEV_STRING_CONVERT(hardware_version)
BUILD_DEV_STRING_CONVERT(firmware_version)
BUILD_DEV_STRING_CONVERT(mac)
BUILD_DEV_STRING_CONVERT(group)
BUILD_WLAN_STRING_CONVERT(ssid)
BUILD_WLAN_STRING_CONVERT(password)
BUILD_WLAN_STRING_CONVERT(country_code)
BUILD_WLAN_STRING_CONVERT(bandwidth)

#define INT_ATTR_CONVERT(type)                                                  \
	{                                                                       \
		.option = #type, .to_uci = type##_itoa, .to_attr = type##_atoi, \
	}
#define STRING_ATTR_CONVERT(type)                                                    \
	{                                                                            \
		.option = #type, .to_uci = type##_to_uci, .to_attr = type##_to_attr, \
	}

static const struct attr_convert device_convert[] = {
	STRING_ATTR_CONVERT(name),
	STRING_ATTR_CONVERT(model),
	STRING_ATTR_CONVERT(hardware_version),
	STRING_ATTR_CONVERT(firmware_version),
	STRING_ATTR_CONVERT(mac),
	INT_ATTR_CONVERT(status),
	STRING_ATTR_CONVERT(group),
	INT_ATTR_CONVERT(ap_alive_time),
	INT_ATTR_CONVERT(client_alive_time),
	INT_ATTR_CONVERT(client_idle_time),
	INT_ATTR_CONVERT(led),
	INT_ATTR_CONVERT(updating),
};

static const struct attr_convert wifi_convert[] = {
	INT_ATTR_CONVERT(enabled),
	STRING_ATTR_CONVERT(ssid),
	INT_ATTR_CONVERT(hide),
	STRING_ATTR_CONVERT(encryption),
	STRING_ATTR_CONVERT(password),
	INT_ATTR_CONVERT(isolation),
	INT_ATTR_CONVERT(bandwidth_limit),
	INT_ATTR_CONVERT(bandwidth_upload),
	INT_ATTR_CONVERT(bandwidth_download),
	INT_ATTR_CONVERT(channel),
	STRING_ATTR_CONVERT(country_code),
	STRING_ATTR_CONVERT(bandwidth),
	INT_ATTR_CONVERT(weak_sta_signal_enable),
	INT_ATTR_CONVERT(prohibit_sta_signal_enable),
	INT_ATTR_CONVERT(weak_sta_signal),
	INT_ATTR_CONVERT(prohibit_sta_signal),
};

static const struct attr_convert *find_convert_by_name(const char *name, const struct attr_convert *convert, int num)
{
	int i = 0;
	for (i = 0; i < num; i++) {
		if (!strcmp(convert[i].option, name))
			return &convert[i];
	}

	return NULL;
}

static const struct attr_convert *find_dev_convert_by_name(const char *name)
{
	return find_convert_by_name(name, device_convert, ARRAY_SIZE(device_convert));
}

static const struct attr_convert *find_wifi_convert_by_name(const char *name)
{
	return find_convert_by_name(name, wifi_convert, ARRAY_SIZE(wifi_convert));
}

/*
 * If custom_wifi == "group", we use group configures in ap_config file
 * as this device's wifi configure.
 */
bool use_group_wifi_attr(struct device_attr *attr)
{
	return attr->custom_wifi && (!strcmp(attr->custom_wifi, "group"));
}

static int set_common_uci_attr(const char *section, const char *option, const char *value, int band)
{
	struct uci_ptr attr_option = {0};
	char buffer[32] = {0};
	int err = 0;

	if (band == WIFI_2G) {
		snprintf(buffer, sizeof(buffer), "2G_%s", option);
		attr_option.option = buffer;
	} else if (band == WIFI_5G) {
		snprintf(buffer, sizeof(buffer), "5G_%s", option);
		attr_option.option = buffer;
	} else {
		attr_option.option = option;
	}
	attr_option.package = DEV_CONFIG;
	attr_option.section = section;
	attr_option.value = value;

	if (value) {
		err = uci_set(ctx, &attr_option);
		// CWCritLog("uci set %s.%s=%s", section, option, value);
		if (err)
			CWCritLog("uci set %s.%s=%s failed with %d", section, option, value, err);
		return err;
	} else {
		err = uci_delete(ctx, &attr_option);
		return err == UCI_ERR_NOTFOUND ? 0 : err;
	}
}

static int set_device_uci_attr(const char *section, const char *option, const char *value)
{
	return set_common_uci_attr(section, option, value, -1);
}

static int set_wifi_uci_attr(const char *section, const char *option, const char *value, int band)
{
	return set_common_uci_attr(section, option, value, band);
}

static int dev_attr_mutex_init(struct device_attr *attr)
{
	return pthread_mutex_init(&attr->mutex, NULL);
}

int dev_attr_mutex_lock(struct device_attr *attr)
{
	return pthread_mutex_lock(&attr->mutex);
}

int dev_attr_mutex_unlock(struct device_attr *attr)
{
	return pthread_mutex_unlock(&attr->mutex);
}

/*
 * Change struct device_attr to a group of struct uci_ptrs.
 * Malloc new space of struct uci_ptr and return number of it.
 * MUST BE used with dev_uci_mutex hold.
 */
static int dev_attr_to_uci(struct device_attr *attr)
{
	struct uci_ptr ptr = {0};
	int i = 0, band = 0;
	char *value = NULL;
	int err = 0;

	for (i = 0; i < ARRAY_SIZE(device_convert); i++) {
		value = device_convert[i].to_uci(attr);
		if ((err = set_device_uci_attr(attr->mac, device_convert[i].option, value)))
			goto error;
		FREE_STRING(value);
	}

	if (use_group_wifi_attr(attr)) {
		if ((err = set_device_uci_attr(attr->mac, "custom_wifi", "group")))
			goto error;
		// Delete wifi attributes
		for (band = 0; band < WIFI_NUM; band++) {
			for (i = 0; i < ARRAY_SIZE(wifi_convert);i++) {
				if ((err = set_wifi_uci_attr(attr->mac, wifi_convert[i].option, NULL, band)))
					goto error;
			}
		}
	} else {
		if ((err = set_device_uci_attr(attr->mac, "custom_wifi", NULL)))
			goto error;
		for (band = 0; band < WIFI_NUM; band++) {
			for (i = 0; i < ARRAY_SIZE(wifi_convert);i++) {
				value = wifi_convert[i].to_uci(&attr->wifi.band[band]);
				if ((err = set_wifi_uci_attr(attr->mac, wifi_convert[i].option, value, band)))
					goto error;
				FREE_STRING(value);
			}
		}
	}
	return 0;

error:
	ptr.package = DEV_CONFIG;
	ptr.section = attr->mac;
	uci_revert(ctx, &ptr);
	FREE_STRING(value);
	CWCritLog("uci set fail with %d!!!!!!!!!", err);
	return err;
}

void do_save_device_config(struct device_attr *attr)
{
	int err = 0;

	if (!attr)
		return;

	dev_attr_mutex_lock(attr);
	pthread_mutex_lock(&dev_uci_mutex);
	if (!attr->section) {
		/* Add new section */
		struct uci_ptr section_ptr = {0};
		section_ptr.package = DEV_CONFIG;
		section_ptr.section = attr->mac;
		section_ptr.value = "device";
		section_ptr.target = UCI_TYPE_SECTION;
		if (uci_set(ctx, &section_ptr) != UCI_OK) {
			err = -EFAULT;
			goto out;
		}
		attr->section = uci_lookup_section(ctx, device_pkg, attr->mac);
		dev_list_add_tail(&attr->list, &device_list);
	}

	if ((err = dev_attr_to_uci(attr)))
		goto out;

	if ((err = uci_save(ctx, device_pkg)))
		CWCritLog("uci_save failed with %d", err);

out:
	pthread_mutex_unlock(&dev_uci_mutex);
	dev_attr_mutex_unlock(attr);

	// status = get_dev_uci_attr(attr, "status");
	// if (atoi(status) != attr->status) {
	// 	CWLog("WTP %d status == 0 after commit, killall AC!!!!!!!!!!!!!", attr->index);
	// 	FREE_STRING(status);
	// 	system("killall AC");
	// }
	// FREE_STRING(status);

	return;
}

void *save_thread(void *arg)
{
	struct device_attr *attr;
	struct timespec t;
	int err = 0;

	pthread_detach(pthread_self());

	for (;;) {
		err = 0;
		pthread_mutex_lock(&save_mutex);
		while (list_empty(&save_list)) {
			if (err == ETIMEDOUT)
				break;
			clock_gettime(CLOCK_REALTIME, &t);
			t.tv_sec += 1;
			err = pthread_cond_timedwait(&save_cond, &save_mutex, &t);
		}
		if (!list_empty(&save_list)) {
			attr = list_first_entry(&save_list, struct device_attr, save_list);
			list_del(&attr->save_list);
			pthread_mutex_unlock(&save_mutex);
			do_save_device_config(attr);
		} else {
			pthread_mutex_unlock(&save_mutex);
		}
		if (err == ETIMEDOUT) {
			if ((err = uci_commit(ctx, &device_pkg, true)))
				CWCritLog("uci_commit failed with %d", err);
		}
	}
}

int save_device_config(struct device_attr *attr, int locked)
{
	struct device_attr *dev;

	pthread_mutex_lock(&save_mutex);
	// check if attr is already in the list, because the same list_node can't
	// be added twice.
	list_for_each_entry(dev, &save_list, save_list) {
		if (dev == attr) {
			pthread_mutex_unlock(&save_mutex);
			return 0;
		}
	}
	list_add_tail(&attr->save_list, &save_list);
	pthread_cond_signal(&save_cond);
	pthread_mutex_unlock(&save_mutex);

	return 0;
}

int delete_device_config(struct device_attr *attr)
{
	struct uci_ptr ptr = {0};
	int err = 0;

	if (!attr)
		return 0;

	pthread_mutex_lock(&dev_uci_mutex);
	if (attr->section) {
		ptr.package = DEV_CONFIG;
		ptr.section = attr->mac;
		uci_delete(ctx, &ptr);
	}
	err = device_uci_commit();
	pthread_mutex_unlock(&dev_uci_mutex);

	return err;
}

void uci_option_to_wifi_attr(struct uci_option *o, struct wifi *wifi)
{
	const struct attr_convert *convert = NULL;
	struct wifi_attr *band;
	const char *name = o->e.name;

	if (!strncmp(name, "2G_", 3))
		band = &wifi->band[WIFI_2G];
	else if (!strncmp(name, "5G_", 3))
		band = &wifi->band[WIFI_5G];
	else
		return;

	convert = find_wifi_convert_by_name(&name[3]);
	if (convert && convert->to_attr)
		convert->to_attr(band, o->v.string);
}

struct group_attr *find_group_attr_by_name(const char *name)
{
	struct uci_context *group_ctx;
	struct uci_package *group_pkg = NULL;
	struct uci_section *section;
	struct group_attr *attr = NULL;
	if (!name)
		return NULL;

	group_ctx = uci_alloc_context();
	if (!group_ctx)
		return NULL;

	if (uci_load(group_ctx, GROUP_CONFIG, &group_pkg) != UCI_OK) {
		uci_free_context(group_ctx);
		return NULL;
	}

	section = uci_lookup_section(group_ctx, group_pkg, name);
	if (section) {
		attr = calloc(1, sizeof(*attr));
		if (attr)
			uci_to_group_attr(section, attr);
	}

	uci_unload(group_ctx, group_pkg);
	uci_free_context(group_ctx);

	return attr;
}

void uci_to_group_attr(struct uci_section *section, struct group_attr *attr)
{
	struct uci_element *e;

	uci_foreach_element (&section->options, e) {
		struct uci_option *o = uci_to_option(e);
		uci_option_to_wifi_attr(o, &attr->wifi);
	}
	attr->name = strdup(section->e.name);
	//dev_list_add_tail(&attr->list, &group_list);
}

void set_group_attr_to_device(struct device_attr *device, struct group_attr *group)
{
	int i;

	if (!device || !group)
		return;

	for (i = 0; i < WIFI_NUM; i++) {
		FREE_STRING(device->wifi.band[i].ssid);
		FREE_STRING(device->wifi.band[i].password);
		FREE_STRING(device->wifi.band[i].country_code);
		FREE_STRING(device->wifi.band[i].bandwidth);
		memcpy(&device->wifi.band[i], &group->wifi.band[i], WIFI_ATTR_SIZE);
	}
	device->custom_wifi = "group";
	FREE_STRING(device->group);
	device->group = group->name;
}

/**
 * Must be used with dev_uci_mutex hold.
 */
static int uci_to_dev_attr(struct uci_section *section, struct device_attr *attr)
{
	struct uci_element *e;
	struct group_attr *group;

	if (!section || !attr)
		return -EINVAL;

	uci_foreach_element (&section->options, e) {
		struct uci_option *o = uci_to_option(e);
		const struct attr_convert *convert = find_dev_convert_by_name(o->e.name);
		if (convert) {
			convert->to_attr(attr, o->v.string);
		} else {
			if (!strcmp(o->e.name, "custom_wifi"))
				attr->custom_wifi = "group";
			else
				uci_option_to_wifi_attr(o, &attr->wifi);
		}
	}

	/*
	 * Only after all the options have been checked
	 * we can consider "group" attribute exists.
	 */
	if (attr->custom_wifi) {
		group = find_group_attr_by_name(attr->group);
		if (!group)
			group = find_group_attr_by_name("default");
		if (!group)
			return -EINVAL;

		set_group_attr_to_device(attr, group);
		group_attr_free(group);
	}
	attr->section = section;
	dev_list_add_tail(&attr->list, &device_list);

	return 0;
}

void reload_dev_attr(struct device_attr *dev)
{
	struct uci_section *section;

	if (!dev)
		return;

	// DO NOT use dev->section here, it may changed after uci_commit
	// and we don't have a lock to keep synchronize with it.
	pthread_mutex_lock(&dev_uci_mutex);
	section = uci_lookup_section(ctx, device_pkg, dev->mac);;
	dev_attr_free(dev);
	uci_to_dev_attr(section, dev);
	pthread_mutex_unlock(&dev_uci_mutex);
}

struct device_attr *find_dev_attr_by_mac(const char *mac)
{
	struct device_attr *attr;

	if ((!mac) || list_empty(&device_list))
		return NULL;

	pthread_spin_lock(&dev_list_mutex);
	list_for_each_entry(attr, &device_list, list) {
		if (attr->mac && !strcmp(attr->mac, mac)) {
			pthread_spin_unlock(&dev_list_mutex);
			return attr;
		}
	}
	pthread_spin_unlock(&dev_list_mutex);
	return NULL;
}

struct device_attr *find_dev_attr_by_raw_mac(uint8_t mac[6])
{
	char str[18] = {0};

	snprintf(str, sizeof(str), "%02hhx_%02hhx_%02hhx_%02hhx_%02hhx_%02hhx",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return find_dev_attr_by_mac(str);
}

int load_device_configs()
{
	struct device_attr *attr = NULL;
	struct uci_element *e;
	int need_commit = 0;
	int err = 0;

	pthread_mutex_lock(&dev_uci_mutex);
	uci_foreach_element (&device_pkg->sections, e) {
		struct uci_section *s = uci_to_section(e);
		attr = malloc(sizeof(*attr));
		if (!attr)
			return -ENOMEM;
		memset(attr, 0, sizeof(*attr));
		dev_attr_mutex_init(attr);

		uci_to_dev_attr(s, attr);
		if (attr->status) {
			// AC just started, every device should be off-line now.
			attr->status = 0;
			set_device_uci_attr(attr->mac, "status", "0");
			need_commit = 1;
		}
	}

	err = need_commit ? device_uci_commit() : 0;
	pthread_mutex_unlock(&dev_uci_mutex);

	return err;
}

void wifi_attr_free(struct wifi *attr)
{
	int i;
	for (i = 0; i < WIFI_NUM; i++) {
		FREE_STRING(attr->band[i].ssid);
		FREE_STRING(attr->band[i].password);
		FREE_STRING(attr->band[i].country_code);
		FREE_STRING(attr->band[i].bandwidth);
	}
}

/*
 * This function is used when there is a new WTP join in,
 * and we need alloc new attrbute to it.
 */
struct device_attr *dev_attr_alloc()
{
	struct device_attr *attr = malloc(sizeof(*attr));
	struct group_attr *group;
	if (!attr)
		return NULL;

	memset(attr, 0, sizeof(*attr));
	pthread_mutex_init(&attr->mutex, NULL);

	attr->name = strdup(DEFAULT_WTP_NAME);

	group = find_group_attr_by_name("default");
	if (!group)
		goto err;
	set_group_attr_to_device(attr, group);
	group_attr_free(group);

	return attr;

err:
	free(attr);
	return NULL;
}

void dev_attr_free(struct device_attr *attr)
{
	if (!attr)
		return;

	if (attr->list.prev && attr->list.next)
		dev_list_del(&attr->list);

	FREE_STRING(attr->name);
	FREE_STRING(attr->model);
	FREE_STRING(attr->hardware_version);
	FREE_STRING(attr->firmware_version);
	FREE_STRING(attr->mac);
	FREE_STRING(attr->group);
	wifi_attr_free(&attr->wifi);
	pthread_mutex_destroy(&attr->mutex);
}

void group_attr_free(struct group_attr *attr)
{
	// Struct wifi is copyed to device_attr, so don't
	// free wifi_attr here.
	// wifi_attr_free(&attr->wifi);
	// free(attr->name);
	free(attr);
}

int uci_interface_init()
{
	pthread_t t;

	ctx = uci_alloc_context();
	if (!ctx)
		return -ENOMEM;

	if (uci_load(ctx, DEV_CONFIG, &device_pkg) != UCI_OK)
		goto clean_ctx;

	if (!device_pkg)
		goto clean_ctx;

	pthread_spin_init(&dev_list_mutex, PTHREAD_PROCESS_PRIVATE);
	pthread_mutex_init(&dev_uci_mutex, NULL);
	pthread_mutex_init(&save_mutex, NULL);

	if (load_device_configs())
		goto clean_dev_pkg;

	if (pthread_create(&t, NULL, save_thread, NULL)) {
		CWLog("create save thread failed");
		return -errno;
	}
	return 0;

clean_dev_pkg:
	uci_unload(ctx, device_pkg);
clean_ctx:
	uci_free_context(ctx);
	return -EIO;
}

void uci_interface_free()
{
	struct device_attr *device, *tmp;

	list_for_each_entry_safe(device, tmp, &device_list, list) {
		dev_attr_free(device);
		free(device);
	}

	uci_unload(ctx, device_pkg);
	uci_free_context(ctx);

	pthread_mutex_destroy(&save_mutex);
	pthread_mutex_destroy(&dev_uci_mutex);
	pthread_spin_destroy(&dev_list_mutex);
}
