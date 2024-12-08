/*
 * Copyright (C) 2018-2021 Franklin <franklin.wang@siflower.com.cn>
 * wifi_ap.c
 * this is for the advance feature for wifi, for example:
 * 1, record the wifi device
 * 2, support wifi 2bands use the same ssid and passwd, if one device(stations) support both band, then with this service, they will
 *    connects with our AP smartly(we will provide better connection quanlity to the device)
 *
 */

#include <unistd.h>
#include <signal.h>
#include <libubox/blobmsg_json.h>
#include <json-c/json_object.h>
#include "libubus.h"
#include "list.h"
#include "wifi.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Used for stations that only support 2.4g
 * @WAIT_TIME the time we set stations to initial state.
 * @LB_DENY we will deny 2.4g for LB_DENY times during a period.
 * @LB_PROBE after LB_PROBE times rsp of 2.4g, we set stations to initial state.
 * */
#define WAIT_TIME 10
#define LB_DENY 3
#define LB_PROBE 5
#define DEAUTH_TIME 3

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

//#define MEM_DEBUG

/* the station informations to be recorded;
 * @lb_ony true means it is a 2.4g only device, false means not.
 * */
struct sta_rec_info {
	struct dl_list list;
	char mac_addr[32];

	struct timeval probe_time;
	int lb_deny;
	bool lb_only;
	int deauth_time;
	//TODO:struct timeval enter_time;// when we received associated
	//TODO:struct timeval leave_time; //when this device leave;
};

/// the context for station
struct sta_ctx {
	struct dl_list list;
	int band;
	char mac_addr[32];//mac address is in string
	int sigidx;
	int signal[30];
	int obj_id;
};

/// the context for bss recording
struct bss_ctx {
	struct dl_list list;
	char ssid[32];
	//the mac addr for this bss
	char mac_addr[32];
	//the band type for this bss, 0 means 2.4G while 1 means 5G
	uint32_t band;
	//the working frequency for this bss
	uint32_t freq;
	// the ubus object ID for this bss
	uint32_t obj_id;
	// all the connnected station in this bss
	struct dl_list sta;
	// num of stations
	uint32_t total_sta;
	struct ubus_object *obj;
};

/// the main context for wifi_ap service
struct wifi_ap_ctx {
	struct dl_list bss_list_24G;
	struct dl_list bss_list_5G;
	//struct dl_list forbidden_list_24G;
	struct dl_list sta_list;
	struct dl_list sta_ctx_list;
	uint32_t dual_band_one_setting;
#ifdef RECORD_STA
	struct dl_list sta_rec_list;
#endif
	uint32_t record_sta_enable;
};

enum {
	WIFI_AP_BAND_24G,
	WIFI_AP_BAND_5G
};

/* The result of dual_band_one_setting_handler */
enum {
	GEN_RESP,
	NOT_RESP_EXIST,
	NOT_RESP_NEW,
	ALLOC_ERR
};

enum {
	WIFI_AP_ENABLE_STA_REC_ACTION,
	__WIFI_AP_ENABLE_STA_REC_MAX
};

enum {
	WIFI_AP_DEBUG_STA_REC_ENABLE,
	WIFI_AP_DEBUG_DBOS,
	WIFI_AP_POLICY_MAX
};

enum {
	WIFI_AP_CON_ID,
	WIFI_AP_CON_BAND,
	WIFI_AP_CON_FREQ,
	WIFI_AP_CON_MAC,
	WIFI_AP_CON_SSID,
	__WIFI_AP_CON_MAX
};

enum {
	WIFI_AP_SUBSCRIBER_CBN_MAC,
	WIFI_AP_SUBSCRIBER_CBN_TARGET,
	WIFI_AP_SUBSCRIBER_CBN_SIGNAL,
	WIFI_AP_SUBSCRIBER_CBN_FREQ,
	WIFI_AP_SUBSCRIBER_CBN_OBJID,
	__WIFI_AP_SUBSCRIBER_CBN_MAX
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const struct blobmsg_policy wif_ap_debug_policy[] = {
	[WIFI_AP_DEBUG_STA_REC_ENABLE] = { .name = "sta_rec_enable", .type = BLOBMSG_TYPE_INT32 },
	[WIFI_AP_DEBUG_DBOS] = { .name = "2band1set", .type = BLOBMSG_TYPE_INT32 },

};

static const struct blobmsg_policy wifi_ap_connect_policy[] = {
	[WIFI_AP_CON_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
	[WIFI_AP_CON_BAND] = { .name = "band", .type = BLOBMSG_TYPE_INT32 },
	[WIFI_AP_CON_FREQ] = { .name = "freq", .type = BLOBMSG_TYPE_INT32 },
	[WIFI_AP_CON_MAC] = { .name = "mac", .type = BLOBMSG_TYPE_STRING },
	[WIFI_AP_CON_SSID] = { .name = "ssid", .type = BLOBMSG_TYPE_STRING },
};

static const struct blobmsg_policy wifi_ap_remove_policy[] = {
	[WIFI_AP_CON_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
};

static const struct blobmsg_policy wifi_ap_probe_req_policy[] = {
	[WIFI_AP_SUBSCRIBER_CBN_MAC] = { .name = "address", .type = BLOBMSG_TYPE_STRING },
	[WIFI_AP_SUBSCRIBER_CBN_TARGET] = { .name = "target", .type = BLOBMSG_TYPE_STRING },
	[WIFI_AP_SUBSCRIBER_CBN_SIGNAL] = { .name = "signal", .type = BLOBMSG_TYPE_INT32 },
	[WIFI_AP_SUBSCRIBER_CBN_FREQ] = { .name = "freq", .type = BLOBMSG_TYPE_INT32 },
	[WIFI_AP_SUBSCRIBER_CBN_OBJID] = { .name = "objid", .type = BLOBMSG_TYPE_INT32 },
};

//struct dl_list sta_24g_only;

extern struct ubus_context *ubus_ctx;
static struct blob_buf b;
static struct wifi_ap_ctx *ctx = NULL;
pthread_mutex_t g_sta_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_stacnt_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_bss_mutex = PTHREAD_MUTEX_INITIALIZER;

extern int probe_rssi_24g_r, probe_rssi_5g_l, deauth_rssi_24g_r, deauth_rssi_5g_l;
extern int sta_rec_num, check_deauth_time;

#ifdef MEM_DEBUG
LIST_HEAD(ms_head);
LIST_HEAD(ai_head);
struct mem_statistics {
	struct list_head list;
	size_t module;
	size_t max;
	size_t current;
};

struct alloc_info {
	struct list_head list;
	void *addr;
	size_t size;
	size_t module;
};

int memtotal_cur = 0, memtotal_max = 0;

static void *dbss_alloc(int obj, int obj_size, int module)
{
	struct mem_statistics *p;
	struct alloc_info *q;
	void *data_ptr;
	int found = 0, size = obj * obj_size;

	data_ptr = calloc(obj_size, obj);
	memset(data_ptr, 0, size);
	list_for_each_entry(p, &ms_head, list) {
		if (p->module == module) {
			found = 1;
			p->max += size;
			p->current += size;
			break;
		}
	}
	if (!found) {
		p = (struct mem_statistics *)malloc(sizeof(*p));
		p->module = module;
		p->max = size;
		p->current = size;
		list_add_tail(&p->list, &ms_head);
	}
	q = (struct alloc_info *)malloc(sizeof(*q));
	q->addr = data_ptr;
	q->size = size;
	q->module = module;
	list_add_tail(&q->list, &ai_head);
	/* TODO: smp_lock, overflow */
	memtotal_cur += size;
	if (memtotal_cur > memtotal_max)
		memtotal_max = memtotal_cur;

	return data_ptr;
}

static void dbss_free(void *ptr)
{
	struct mem_statistics *p;
	struct alloc_info *q;
	int size, module, found = 0;

	list_for_each_entry(q, &ai_head, list) {
                if (q->addr == ptr) {
                        found = 1;
                        size = q->size;
                        module = q->module;
                        break;
                }
        }
        if (found) {
                list_del(&q->list);
                free(q); /* q is a MEM_STATISTICS, not in ai_head */
        } else {
                /* if MEM_STATISTICS, not found and return */
		DEBUG(LOG_ERR, "0x%x not found!!!!!\n", ptr);
		free(ptr);
                return;
        }

	found = 0;
	list_for_each_entry(p, &ms_head, list) {
		if (p->module == module) {
			found = 1;
			p->current -= size;
			if (p->current < 0)
				DEBUG(LOG_ERR, "unexpected error!\n");
			break;
		}
	}
	if (!found)
		DEBUG(LOG_ERR, "free an unexpected module, check your malloc!\n");
	else
		memtotal_cur -= size;

	free(ptr);
}
#else
static void *dbss_alloc(int obj, int obj_size, int module)
{
	return calloc(obj_size, obj);
}
static void dbss_free(void *ptr)
{
	free(ptr);
}
#endif /* MEM_DEBUG */

static void sta_deinit(struct sta_ctx *sta)
{
	sta->obj_id = 0;
	return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void wifi_ap_deinit(struct bss_ctx *bss_ctx)
{
	struct sta_ctx *sta, *n;
	//release all the resources that based on this bss
	pthread_mutex_lock(&g_bss_mutex);
	//1, release all the station resources
	dl_list_for_each_safe(sta, n, &bss_ctx->sta, struct sta_ctx, list)
		sta_deinit(sta);
	//2, delete the bss from the whole chain
	dl_list_del(&bss_ctx->list);
	//3, release bss ctx itself
	dbss_free(bss_ctx);
	pthread_mutex_unlock(&g_bss_mutex);
}


static struct bss_ctx *
wifi_ap_find_bss(uint32_t id)
{
	struct bss_ctx *bss;

	dl_list_for_each(bss, &ctx->bss_list_24G, struct bss_ctx, list)
		if (bss->obj_id == id)
			return bss;

	dl_list_for_each(bss, &ctx->bss_list_5G, struct bss_ctx, list)
		if (bss->obj_id == id)
			return bss;

	return  NULL;
}

static int
wifi_ap_remove(struct ubus_context *uctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg)
{
	struct blob_attr *tb[__WIFI_AP_CON_MAX];
	struct bss_ctx *bss_ctx;
	int id;

	DEBUG(LOG_DEBUG, "remove bss_list\n");
	blobmsg_parse(wifi_ap_remove_policy, ARRAY_SIZE(wifi_ap_connect_policy), tb, blob_data(msg), blob_len(msg));
	if (!tb[WIFI_AP_CON_ID])
		return UBUS_STATUS_INVALID_ARGUMENT;

	id =  blobmsg_get_u32(tb[WIFI_AP_CON_ID]);
	bss_ctx = wifi_ap_find_bss(id);
	if (!bss_ctx) {
		DEBUG(LOG_ERR, "can not find the bss with id 0x%08x\n", id);
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
	wifi_ap_deinit(bss_ctx);

	return 0;
}

static int find_sta_ctx_list(char *macaddr)
{
	struct sta_ctx *sta;
	dl_list_for_each(sta, &ctx->sta_ctx_list, struct sta_ctx, list) {
		if (strcmp(sta->mac_addr, macaddr) == 0 )
		      return 0;
	}
	return 1;
}

static int add_and_check_sta(int band, char *macaddr)
{
	struct sta_rec_info *fsta, *fsta1;
	struct dl_list *flist;
	struct timeval now, old;
	int ret;

	flist = &ctx->sta_list;

	dl_list_for_each_safe(fsta, fsta1, flist, struct sta_rec_info, list) {
		/* clear the record of stations that probed 5 mins ago */
		gettimeofday(&now, NULL);
		old.tv_sec = fsta->probe_time.tv_sec;

		if (now.tv_sec - old.tv_sec > 300)
		{
			ret = find_sta_ctx_list(fsta->mac_addr);
			if (ret)
			{
				dl_list_del(&fsta->list);
				dbss_free(fsta);
				continue;
			} else
			      fsta->probe_time.tv_sec = now.tv_sec;
		}

		if (strcmp(fsta->mac_addr, macaddr) == 0) {
			/* update timestamp */
			fsta->probe_time.tv_sec = now.tv_sec;

			if (band == WIFI_AP_BAND_24G) {
				// 2.4g only device will just return GEN_RESP
				if (fsta->lb_only)
					return GEN_RESP;

				if (now.tv_sec - old.tv_sec > WAIT_TIME)
				      fsta->lb_deny = 0;

				if (fsta->lb_deny > LB_DENY) {
					DEBUG(LOG_NOTICE, "now %d - probe %d, mustn't connect\n",
								now.tv_sec, old.tv_sec);

					if (fsta->lb_deny - LB_DENY < LB_PROBE) {
						fsta->lb_deny ++;
						return GEN_RESP;
					} else {
						fsta->lb_deny = 0;
						return GEN_RESP;
					}
				} else {
					fsta->lb_deny ++;
					return -NOT_RESP_EXIST;
				}
			} else {
				fsta->lb_only = false;
				return -NOT_RESP_EXIST;
			}
		}
	}

	DEBUG(LOG_DEBUG, "Sta not found in forbidden list! now add a new one.\n");
	/* check forbidden list */
	fsta = (struct sta_rec_info *)dbss_alloc(1, sizeof(*fsta), 5);
	if (!fsta) {
		DEBUG(LOG_ERR, "Failed to alloc in %s!\n", __func__);
		return -ALLOC_ERR;
	}
	fsta->lb_only = true;
	fsta->deauth_time = 0;

	strcpy(fsta->mac_addr, macaddr);
	gettimeofday(&fsta->probe_time, NULL);
	dl_list_add(flist, &fsta->list);

	return -NOT_RESP_NEW;
}

int sta_cnt[2];

/*
 * Call hostapd ubus get_clients to get the total
 * cnt of stations of hb and lb interface.
 * Check if hb is overload and can redirect to lb.
 * return: 1 for overload, 0 for normal.
 */
static int check_5g_load(void)
{
	int ret = 0;

	pthread_mutex_lock(&g_stacnt_mutex);

	if (sta_cnt[WIFI_AP_BAND_5G] > 20) {
		DEBUG(LOG_DEBUG, "%d 5g stations connected!\n", sta_cnt[WIFI_AP_BAND_5G]);
		if (sta_cnt[WIFI_AP_BAND_5G] - sta_cnt[WIFI_AP_BAND_24G] > 5)
			ret = 1;
		else
			ret = 0;
	} else {
		ret = 0;
	}

	pthread_mutex_unlock(&g_stacnt_mutex);

	return ret;
}

static int dual_band_one_setting_handler(struct bss_ctx *bctx, struct blob_attr **tb)
{
	char *macaddr = blobmsg_get_string(tb[WIFI_AP_SUBSCRIBER_CBN_MAC]);
	int signal = blobmsg_get_u32(tb[WIFI_AP_SUBSCRIBER_CBN_SIGNAL]);
	int band_5g_overload = 0;

	band_5g_overload = check_5g_load();
	DEBUG(LOG_DEBUG, "band is %d, signal is %d, macaddr is %s\n", bctx->band, signal, macaddr);
	/* Signal check */
	if (bctx->band == WIFI_AP_BAND_24G) {
		if (signal > probe_rssi_24g_r) {
			if (!band_5g_overload)
				goto probe_deny; /* rssi ok, not connect, try high band. */
		}
	} else {
		if (signal < probe_rssi_5g_l || band_5g_overload)
			goto probe_deny; /* try low band. */
		else
			add_and_check_sta(bctx->band, macaddr);
	}

	return GEN_RESP;

probe_deny:
	return add_and_check_sta(bctx->band, macaddr);
}

#ifdef RECORD_STA
#ifdef siwifi_dbss_curl
extern int postHttps(char *url,void *data,void *response);
struct HttpResonseData{
	size_t size;
	void *data;
	struct HttpResonseData *next;
};
static int wifiprobe_post(char *hostmac, struct dl_list *l)
{
	int ret = 0;
	struct HttpResonseData response;
	const char *p;
	struct sta_rec_info *sta_rec, *sta_rec1;
	json_object *host_obj, *sta_obj, *stations;

	host_obj = json_object_new_object();
	if (!host_obj)
		return -1;
	json_object_object_add(host_obj, "hostrouter",
			json_object_new_string(hostmac));
	stations = json_object_new_array();
	dl_list_for_each_safe(sta_rec, sta_rec1, l, struct sta_rec_info, list) {
		sta_obj = json_object_new_object();
		json_object_object_add(sta_obj, "mac", json_object_new_string(sta_rec->mac_addr));
		//	json_object_object_add(sta_obj, "connect", json_object_new_int(1));
		//	json_object_object_add(sta_obj, "bssid", json_object_new_string("11:22:33:44:55:66"));
		//	json_object_object_add(sta_obj, "entrytime", json_object_new_string("2018.11.14-14:20:23"));
		//	json_object_object_add(sta_obj, "leavetime", json_object_new_string("2018.11.14-14:20:23"));
		json_object_array_add(stations, sta_obj);
		dl_list_del(&sta_rec->list);
		dbss_free(sta_rec);
	}

	json_object_object_add(host_obj, "stations", stations);
	p = json_object_get_string(host_obj);
	ret = postHttps("https://tommy.siflower.cn/wifiProbe", (char*)p, &response);
	if (response.size > 0)
		dbss_free(response.data);
	DEBUG(LOG_DEBUG, "post station record info ret=%d, str=%s\n", ret, p);
	json_object_put(host_obj);

	return ret;
}
#endif
#endif

static int
wifi_ap_probe_req(struct ubus_context *uctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg)
{
	struct blob_attr *tb[__WIFI_AP_SUBSCRIBER_CBN_MAX];
	char *str;
	struct bss_ctx *bss_ctx;
	uint32_t objid = 0, ret = -1;

	str = blobmsg_format_json(msg, true);
	DEBUG(LOG_DEBUG, "Received notification '%s' : %s\n", method, str);

	blobmsg_parse(wifi_ap_probe_req_policy,
			ARRAY_SIZE(wifi_ap_probe_req_policy), tb, blob_data(msg), blob_len(msg));

	if (tb[WIFI_AP_SUBSCRIBER_CBN_OBJID])
		objid = blobmsg_get_u32(tb[WIFI_AP_SUBSCRIBER_CBN_OBJID]);

	bss_ctx = wifi_ap_find_bss(objid);

	if (!bss_ctx) {
		DEBUG(LOG_ERR, "can not find the bss with id 0x%08x\n", objid);
		ret = UBUS_STATUS_UNKNOWN_ERROR;
		goto out;
	}

	if (!tb[WIFI_AP_SUBSCRIBER_CBN_MAC] || !tb[WIFI_AP_SUBSCRIBER_CBN_FREQ]) {
		DEBUG(LOG_ERR, "invalid tb parameters!\n");
		ret = UBUS_STATUS_INVALID_ARGUMENT;
		goto out;
	}

	if (!strcmp(method, "wifi_ap_probe_req"))
	{
		DEBUG(LOG_DEBUG, "get probe method\n");
		if (!tb[WIFI_AP_SUBSCRIBER_CBN_SIGNAL] || !tb[WIFI_AP_SUBSCRIBER_CBN_TARGET])
		{
			DEBUG(LOG_ERR, "invalid tb parameters!\n");
			ret = UBUS_STATUS_INVALID_ARGUMENT;
			goto out;
		}

#ifdef RECORD_STA
		//record the station's mac address if dump station function is enable
		if (ctx->record_sta_enable) {
			char s[32];
			struct sta_rec_info *sta_info = (struct sta_rec_info *)dbss_alloc(1, sizeof(struct sta_rec_info), 4);
			DEBUG(LOG_DEBUG, "sta record!!!\n");
			if (!sta_info) {
				DEBUG(LOG_ERR, "can not alloc sta_ctx,oom!\n");
				ret = UBUS_STATUS_UNKNOWN_ERROR;
				goto out;
			}
			strcpy(sta_info->mac_addr, blobmsg_get_string(tb[WIFI_AP_SUBSCRIBER_CBN_MAC]));
			gettimeofday(&sta_info->probe_time, NULL);

			dl_list_add(&ctx->sta_rec_list, &sta_info->list);
			bss_ctx->total_sta++;
			//TODO if the whole rec number was big than a threshold, trigger the upload threads to cleanup all dump

			/*****************************************************/
			if (bss_ctx->total_sta >= sta_rec_num) {
				sprintf(s, MACSTR, MAC2STR((unsigned char *)bss_ctx->mac_addr));
#ifdef siwifi_dbss_curl
				wifiprobe_post(s, &ctx->sta_rec_list);
#endif
				bss_ctx->total_sta -= sta_rec_num;
			}
		}
#endif
		//decide if accept this station to probe
		if (ctx->dual_band_one_setting) {
			DEBUG(LOG_DEBUG, "dual band one!!!\n");
			ret = dual_band_one_setting_handler(bss_ctx, tb);
			goto out;
			/*
			//1, check if this probe request is a broadcast frames
			if()
				//2, if the probe is 2.4G or 5G
				2.3.1 check this station was already responsed by the other band, if yes, then we did not send response;
			2.3.1 check if station is 2.4G. if 2.4G, then we just record the station and create a task which will detect if 2.4G's probe request will
				come in again with 10 seconds.
				I
				2.3.2 check if we can connect with 5G. The condition are:
				---if the stations num in 5G has been over than the threshold
				---If the number that 5G'S stations number minus 2.4G's station number has been over than the threshol
				*/


		}

	}

out:
	/* ubus response */
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "ret", ret);
	ubus_send_reply(uctx, req, b.head);
	free(str); /* free NULL is safe */

	return 0;
}

static int
wifi_ap_connect(struct ubus_context *uctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg)
{
	struct blob_attr *tb[__WIFI_AP_CON_MAX];
	struct bss_ctx *bss_ctx;

	DEBUG(LOG_DEBUG, "%s:\n", __func__);

	blobmsg_parse(wifi_ap_connect_policy, ARRAY_SIZE(wifi_ap_connect_policy), tb, blob_data(msg), blob_len(msg));

	if (!tb[WIFI_AP_CON_ID] || !tb[WIFI_AP_CON_BAND] ||
			!tb[WIFI_AP_CON_FREQ] || !tb[WIFI_AP_CON_MAC])
		return UBUS_STATUS_INVALID_ARGUMENT;

	bss_ctx = dbss_alloc(1, sizeof(struct bss_ctx), 3);
	if (!bss_ctx)
		return UBUS_STATUS_UNKNOWN_ERROR;

	bss_ctx->obj_id = blobmsg_get_u32(tb[WIFI_AP_CON_ID]);
	bss_ctx->band = blobmsg_get_u32(tb[WIFI_AP_CON_BAND]);
	bss_ctx->freq = blobmsg_get_u32(tb[WIFI_AP_CON_FREQ]);
	bss_ctx->total_sta = 0;
	strcpy(bss_ctx->mac_addr, blobmsg_get_string(tb[WIFI_AP_CON_MAC]));
	strcpy(bss_ctx->ssid, tb[WIFI_AP_CON_SSID] ? blobmsg_data(tb[WIFI_AP_CON_SSID]) : "unknown");

	dl_list_init(&bss_ctx->sta);

	//add this bss to bss list
	pthread_mutex_lock(&g_bss_mutex);
	if (bss_ctx->band == WIFI_AP_BAND_24G)
		dl_list_add(&ctx->bss_list_24G, &bss_ctx->list);
	else
		dl_list_add(&ctx->bss_list_5G, &bss_ctx->list);
	pthread_mutex_unlock(&g_bss_mutex);

	return 0;
}


static int
wif_ap_debug(struct ubus_context *uctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg)
{
	struct blob_attr *tb[__WIFI_AP_CON_MAX];

	DEBUG(LOG_DEBUG, "%s!!!!!!!!!!!\n", __func__);

	blobmsg_parse(wif_ap_debug_policy, ARRAY_SIZE(wif_ap_debug_policy), tb, blob_data(msg), blob_len(msg));
	if (tb[WIFI_AP_DEBUG_STA_REC_ENABLE])
		ctx->record_sta_enable = blobmsg_get_u32(tb[WIFI_AP_CON_ID]);
	if (tb[WIFI_AP_DEBUG_DBOS])
		ctx->dual_band_one_setting = blobmsg_get_u32(tb[WIFI_AP_DEBUG_DBOS]);

	return 0;
}

static const struct ubus_method main_methods[] = {
	//for example, open the dump feature for all wifi device's mac address
	UBUS_METHOD("wifi_ap_debug", wif_ap_debug, wif_ap_debug_policy),
	UBUS_METHOD("wifi_ap_connect", wifi_ap_connect, wifi_ap_connect_policy),
	UBUS_METHOD("wifi_ap_remove", wifi_ap_remove, wifi_ap_remove_policy),
	UBUS_METHOD("wifi_ap_probe_req", wifi_ap_probe_req, wifi_ap_probe_req_policy),
};

static struct ubus_object_type main_object_type =
			UBUS_OBJECT_TYPE("sf_adv_wl_wifi_ap", main_methods);

static struct ubus_object main_object = {
	.name = "sf_adv_wl_wifi_ap",
	.type = &main_object_type,
	.methods = main_methods,
	.n_methods = ARRAY_SIZE(main_methods),
};

static int get_client_signal(struct blob_attr *cur, int *signal)
{
	struct blob_attr *cur1;
	int rem, status;

	blobmsg_for_each_attr(cur1, cur, rem) {
		if (strcmp(blobmsg_name(cur1), "auth") == 0) {
			status = blobmsg_get_bool(cur1);
			if (!status)
				break;
		}
		if (strcmp(blobmsg_name(cur1), "signal") == 0) {
			*signal = blobmsg_get_u32(cur1);
			DEBUG(LOG_DEBUG, "----------signal is %d\n", *signal);
		}
	}

	return 0;
}

static void band_switch_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
	const struct blobmsg_policy sta_info_policy[] = {
		{ .name = "freq", .type = BLOBMSG_TYPE_INT32 },
		{ .name = "clients", .type = BLOBMSG_TYPE_TABLE },
	};
	struct blob_attr *tb[2], *cur;
	struct sta_ctx *sta, *new_sta;
	int band, sigidx, found, rem;

	if (!msg)
		return;

	blobmsg_parse(sta_info_policy, ARRAY_SIZE(sta_info_policy), tb, blob_data(msg), blob_len(msg));
	if (blobmsg_get_u32(tb[0]) > 5000)
		band = WIFI_AP_BAND_5G;
	else
		band = WIFI_AP_BAND_24G;

	blobmsg_for_each_attr(cur, tb[1], rem) {
		pthread_mutex_lock(&g_stacnt_mutex);
		sta_cnt[band]++;
		pthread_mutex_unlock(&g_stacnt_mutex);
		found = 0;
		DEBUG(LOG_DEBUG, "----------band is %d, mac is %s\n",
				blobmsg_get_u32(tb[0]), blobmsg_name(cur));
		pthread_mutex_lock(&g_sta_mutex);
		dl_list_for_each(sta, &ctx->sta_ctx_list, struct sta_ctx, list) {
			if (strcmp(sta->mac_addr, blobmsg_name(cur)) == 0 &&
								band == sta->band) {
				found = 1;
				sigidx = ++sta->sigidx;
				if (sigidx >= check_deauth_time) {
					DEBUG(LOG_DEBUG, "----------sigidx is %d\n", sigidx);
					sta->sigidx--;
					pthread_mutex_unlock(&g_sta_mutex);
					break;
				}
				if (get_client_signal(cur, &sta->signal[sigidx]))
					sta->sigidx--;
				pthread_mutex_unlock(&g_sta_mutex);
				break;
			}
		}
		if (!found) {
			new_sta = dbss_alloc(1, sizeof(struct sta_ctx), 2);
			new_sta->band = band;
			new_sta->sigidx = 0;
			new_sta->obj_id = *(int *)req->priv;
			DEBUG(LOG_DEBUG, "----------%s objid is 0x%x!\n", __func__, new_sta->obj_id);
			strcpy(new_sta->mac_addr, blobmsg_name(cur));
			if (get_client_signal(cur, &new_sta->signal[new_sta->sigidx])) {
				dbss_free(new_sta);
			} else {
				dl_list_add(&ctx->sta_ctx_list, &new_sta->list);
			}
		}
		pthread_mutex_unlock(&g_sta_mutex);
	}
	DEBUG(LOG_DEBUG, "----------%s done!\n", __func__);

	return;
}

static int sta_deauth(struct sta_ctx *sta)
{
	if (sta->band == WIFI_AP_BAND_24G) {
		if (check_5g_load())
			return 0;
	}

	if (sta->obj_id) {
		DEBUG(LOG_NOTICE, "kick out unexpected stations! mac is %s, band is %d\n",
							sta->mac_addr, sta->band);
		blob_buf_init(&b, 0);
		blobmsg_add_u32(&b, "reason", 3); /* WLAN_REASON_DEAUTH_LEAVING */
		blobmsg_add_u8(&b, "deauth", 1);
		blobmsg_add_u32(&b, "ban_time", 500);
		blobmsg_add_string(&b, "addr", sta->mac_addr);
		ubus_invoke(ubus_ctx, sta->obj_id, "del_client", b.head, NULL, 0, 0);
	}
	dl_list_del(&sta->list);
	dbss_free(sta);

	return 0;
}

/*
 * Returns 0 for accepted signal,
 *         1 means deauth needed.
 */
static int check_signal(struct sta_ctx *sta, int n, char *mac)
{
	struct sta_rec_info *tmpsta;
	int i, max = -200, min = 0, avg = 0;

	dl_list_for_each(tmpsta, &ctx->sta_list, struct sta_rec_info, list) {
		DEBUG(LOG_DEBUG, "check sta %s vs %s\n", tmpsta->mac_addr, mac);
		if (strcmp(tmpsta->mac_addr, mac) != 0)
		      continue;
		if (tmpsta->lb_only || tmpsta->deauth_time > DEAUTH_TIME)
		{
			DEBUG(LOG_INFO, "sta %s is lb only device or deauth %d times.\n", mac, tmpsta->deauth_time);
			return 0;
		}


		for (i = 0; i < n; i++) {
			avg += sta->signal[i];
			max = sta->signal[i] > max ? sta->signal[i] : max;
			min = sta->signal[i] < min ? sta->signal[i] : min;
		}
		avg = (avg - max - min) / (n - 2);

		if (sta->band == WIFI_AP_BAND_24G) {
			/* check 2.4g only */
			if (avg > deauth_rssi_24g_r) {
				DEBUG(LOG_NOTICE, "band: 2.4g, check signal: %d, %d\n", i, avg);
				tmpsta->deauth_time ++;
				return 1;
			}
		} else {
			if (avg < deauth_rssi_5g_l) {
				DEBUG(LOG_NOTICE, "band: 5g, check signal: %d, %d\n", i, avg);
				tmpsta->deauth_time ++;
				return 1;
			}
		}
	}

	return 0;
}

static void band_switch(struct uloop_timeout *timeout);
static void update_sta_signal(struct uloop_timeout *timeout);

#ifdef MEM_DEBUG
static void dump_list_info(struct uloop_timeout *timeout);
static void do_get_clients(struct uloop_timeout *timeout);
#endif

static struct uloop_timeout sta_deauth_timer = {
	.cb = band_switch,
};

static struct uloop_timeout sta_update_timer = {
	.cb = update_sta_signal,
};

#ifdef MEM_DEBUG
static struct uloop_timeout test_timer = {
	.cb = dump_list_info,
};

static struct uloop_timeout test_st_timer = {
	.cb = do_get_clients,
};

static void do_get_clients(struct uloop_timeout *timeout)
{
	struct bss_ctx *bss;

	blob_buf_init(&b, 0);

	dl_list_for_each(bss, &ctx->bss_list_24G, struct bss_ctx, list)
		ubus_invoke(ubus_ctx, bss->obj_id, "get_clients", b.head, NULL, &bss->obj_id, 100);

	dl_list_for_each(bss, &ctx->bss_list_5G, struct bss_ctx, list)
		ubus_invoke(ubus_ctx, bss->obj_id, "get_clients", b.head, NULL, &bss->obj_id, 100);

	uloop_timeout_set(&test_st_timer, 1000);
	return;
}

static void dump_list_info(struct uloop_timeout *timeout)
{
	struct bss_ctx *bss;
	struct sta_rec_info *sta_rec;
	struct sta_ctx *sta_ctx;
	struct mem_statistics *p;
	int cnt;

	cnt = 0;
	dl_list_for_each(bss, &ctx->bss_list_24G, struct bss_ctx, list)
		cnt++;
	DEBUG(LOG_ERR, "bss_list_24G cnt = %d\n", cnt);

	cnt = 0;
	dl_list_for_each(bss, &ctx->bss_list_5G, struct bss_ctx, list)
		cnt++;
	DEBUG(LOG_ERR, "bss_list_5G cnt = %d\n", cnt);

#ifdef RECORD_STA
	cnt = 0;
	dl_list_for_each(sta_rec, &ctx->sta_rec_list, struct sta_rec_info, list)
		cnt++;
	DEBUG(LOG_ERR, "sta_rec_list cnt = %d\n", cnt);
#endif
	cnt = 0
	dl_list_for_each(sta_rec, &ctx->sta_list, struct sta_rec_info, list)
		cnt++;
	DEBUG(LOG_ERR, "sta_lis cnt = %d\n", cnt);

	cnt = 0;
	dl_list_for_each(sta_ctx, &ctx->sta_ctx_list, struct sta_ctx, list)
		cnt++;
	DEBUG(LOG_ERR, "sta_ctx_list cnt = %d\n", cnt);

	DEBUG(LOG_ERR, "------------------------------\n");
	DEBUG(LOG_ERR, "total: max = %d, cur = %d\n", memtotal_max, memtotal_cur);
	DEBUG(LOG_ERR, "------------------------------\n");
	list_for_each_entry(p, &ms_head, list)
		DEBUG(LOG_ERR, "module = %d: total = %d, current = %d\n", p->module, p->max, p->current);
	DEBUG(LOG_ERR, "------------------------------\n");
	DEBUG(LOG_ERR, "%s ends.......\n", __func__);

	uloop_timeout_set(&test_timer, 20000);
	return;
}
#endif /* MEM_DEBUG */

static void update_sta_signal(struct uloop_timeout *timeout)
{
	struct bss_ctx *bss;

	blob_buf_init(&b, 0);

	pthread_mutex_lock(&g_stacnt_mutex);
	sta_cnt[WIFI_AP_BAND_24G] = 0;
	sta_cnt[WIFI_AP_BAND_5G] = 0;
	pthread_mutex_unlock(&g_stacnt_mutex);

	dl_list_for_each(bss, &ctx->bss_list_24G, struct bss_ctx, list)
		ubus_invoke(ubus_ctx, bss->obj_id, "get_clients", b.head, band_switch_cb, &bss->obj_id, 1000);

	dl_list_for_each(bss, &ctx->bss_list_5G, struct bss_ctx, list)
		ubus_invoke(ubus_ctx, bss->obj_id, "get_clients", b.head, band_switch_cb, &bss->obj_id, 1000);

	uloop_timeout_set(&sta_update_timer, 1000);

	return;
}

static void band_switch(struct uloop_timeout *timeout)
{
	struct sta_ctx *sta, *sta1;
	int index;

	pthread_mutex_lock(&g_sta_mutex);
	dl_list_for_each_safe(sta, sta1, &ctx->sta_ctx_list, struct sta_ctx, list) {
		index = sta->sigidx;
		sta->sigidx = -1;
		DEBUG(LOG_DEBUG, "----------sigidx is %d!\n", index);
		if (index < 0)
			sta_deauth(sta);

		if (index < check_deauth_time - 1)
			continue;

		if (check_signal(sta, index + 1, sta->mac_addr))
			sta_deauth(sta);
	}
	pthread_mutex_unlock(&g_sta_mutex);
	uloop_timeout_set(&sta_deauth_timer, check_deauth_time * 1000);

	return;
}


static int band_switch_init(void)
{
	uloop_timeout_set(&sta_deauth_timer, check_deauth_time * 1000);
	uloop_timeout_set(&sta_update_timer, 1000);
#ifdef MEM_DEBUG
	uloop_timeout_set(&test_timer, 20000);
	uloop_timeout_set(&test_st_timer, 1000);
#endif
	return 0;
}

//-------------------------------------------------------------------------------
int wifi_ap_init(struct ubus_context *uctx, uint32_t enable_sta_rec)
{
	int ret;

	DEBUG(LOG_DEBUG, "%s\n", __func__);
	ctx = dbss_alloc(1, sizeof(struct wifi_ap_ctx), 1);
	if (!ctx) {
		DEBUG(LOG_ERR, "can not allocate wifi_ap_ctx, size : %d\n", sizeof(struct wifi_ap_ctx));
		return UBUS_STATUS_UNKNOWN_ERROR;
	}

	dl_list_init(&ctx->bss_list_24G);
	dl_list_init(&ctx->bss_list_5G);
#ifdef RECORD_STA
	dl_list_init(&ctx->sta_rec_list);
#endif
	dl_list_init(&ctx->sta_list);
	dl_list_init(&ctx->sta_ctx_list);

	ret = ubus_add_object(uctx, &main_object);
	if (ret) {
		DEBUG(LOG_ERR, "Failed to add object: %s\n", ubus_strerror(ret));
		dbss_free(ctx);
		return UBUS_STATUS_UNKNOWN_ERROR;
	}

	ctx->record_sta_enable = enable_sta_rec;
	ctx->dual_band_one_setting = 1; //by default, we enable this

	band_switch_init();

	return ret;

}

void wifi_ap_deinit_ctx(struct ubus_context *uctx)
{
	struct bss_ctx *bss, *bss1;
	struct sta_rec_info *sta_rec, *sta_rec1;
	struct sta_ctx *sta_ctx1, *sta_ctx2;

	ubus_remove_object(uctx, &main_object);

	uloop_timeout_cancel(&sta_update_timer);
	uloop_timeout_cancel(&sta_deauth_timer);
#ifdef MEM_DEBUG
	uloop_timeout_cancel(&test_timer);
	uloop_timeout_cancel(&test_st_timer);
#endif

	dl_list_for_each_safe(bss, bss1, &ctx->bss_list_24G, struct bss_ctx, list)
		wifi_ap_deinit(bss);

	dl_list_for_each_safe(bss, bss1, &ctx->bss_list_5G, struct bss_ctx, list)
		wifi_ap_deinit(bss);

#ifdef RECORD_STA
	dl_list_for_each_safe(sta_rec, sta_rec1, &ctx->sta_rec_list, struct sta_rec_info, list)
		dbss_free(sta_rec);
#endif
	dl_list_for_each_safe(sta_rec, sta_rec1, &ctx->sta_list, struct sta_rec_info, list)
		dbss_free(sta_rec);

	dl_list_for_each_safe(sta_ctx1, sta_ctx2, &ctx->sta_ctx_list, struct sta_ctx, list)
		dbss_free(sta_ctx1);

	dbss_free(ctx);
}
