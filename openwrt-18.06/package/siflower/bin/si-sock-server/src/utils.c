/*
 * =====================================================================================
 *
 *       Filename:  utils.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  07/28/2015 10:03:37 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  franklin , franklin.wang@siflower.com.cn
 *        Company:  Siflower Communication Tenology Co.,Ltd
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uci.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <linux/module.h>

#include "utils.h"
#include "cJSON.h"
#include "local_http.h"
#include "object_save_file.h"
#include "device_info.h"
#include "local_storage.h"
#include "mtd.h"
#include "sf_factory_read.h"

struct MesageQueue *g_periodEventQueue = NULL;

int loglv = 0;

void init_log_level(void){
	char loglv_buf[2];
	getUciConfig("sidefault","ssstlog","level",loglv_buf);
	if(strncmp(loglv_buf, "0", 1) == 0){
		loglv = 0;
	}else if(strncmp(loglv_buf, "1", 1) == 0){
		loglv = 1;
	}else{
		loglv = 2;
	}
}

int32_t get_log_level(void){
	return loglv;
}

int32_t get_json_value_string(cJSON *item,char *key,char *buffer)
{
    cJSON *value = cJSON_GetObjectItem(item,key);
    if(!value){
        LOG( "[server] %s item' is not included in json\n",key);
        return -1;
    }
    if(cJSON_GetValueString(value, buffer) < 0){
        LOG( "[server]%s can not get string value from item\n",key);
        return -1;
    }
    return 0;
}

int32_t get_json_value_int(cJSON *item,char *key,int32_t *buffer)
{
    cJSON *value = cJSON_GetObjectItem(item,key);
    if(!value){
        LOG( "[server] %s item' is not included in json\n",key);
        return -1;
    }
    if(cJSON_GetValueInt(value, buffer) < 0){
        LOG( "[server]%s can not get int value from item\n",key);
        return -1;
    }
    return 0;
}

int32_t get_json_pointer_attr_string(cJSON *root,char *name,char *key,char *buffer)
{
    cJSON *item = cJSON_GetObjectItem(root,name);
    if(!item) return -1;
    return get_json_value_string(item,key,buffer);
}

int32_t get_json_pointer_attr_int(cJSON *root,char *name,char *key,int32_t *buffer)
{
    cJSON *item = cJSON_GetObjectItem(root,name);
    if(!item) return -1;
    return get_json_value_int(item,key,buffer);
}

int32_t parseStringFromJson(const char *key,void *data,char *value)
{
    int32_t res = -1;
    cJSON* json = cJSON_Parse(data);
    if(!json) goto clean;
    cJSON *object = cJSON_GetObjectItem(json,key);
    if(!object){
        LOG("objectid not exist\n");
        goto clean;
    }
    char *json_tmp = cJSON_Print(object);
    sprintf(value,"%s", json_tmp);
    res = 0;
    free(json_tmp);
clean:
   if(json) cJSON_Delete(json);
    return res;
}

int32_t parseIntFromJson(const char *key,void *data,int *value)
{
    int32_t res = -1;
    cJSON* json = cJSON_Parse(data);
    if(!json) goto clean;
    cJSON *object = cJSON_GetObjectItem(json,key);
    if(!object){
        LOG("objectid not exist\n");
        goto clean;
    }
    cJSON_GetValueInt(object, value);
    res = 0;
clean:
    if(json) cJSON_Delete(json);
        return res;
}

int32_t getRouterListValueFromUci(const char *key,char *retBuffer,int32_t order)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, "siserver", &p) == UCI_OK)
    {
        struct uci_section *router = uci_lookup_section(ctx, p, "cloudrouter");
        //lookup values
        if(router != NULL){
            struct uci_option *option = uci_lookup_option(ctx,router,key);
            if(option != NULL && option->type == UCI_TYPE_LIST){
                int counter = 0;
                struct uci_element *e;
                uci_foreach_element(&option->v.list, e) {
                    if(counter == order){
                        strcpy(retBuffer, e->name);
                        ret = 0;
                        break;
                    }
                    counter++;
                }
            }
        }
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

int32_t getSfHardwareConfig(const char *key,char *retBuffer)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, "siwifi", &p) == UCI_OK)
    {
        struct uci_section *router = uci_lookup_section(ctx, p, "hardware");
        //lookup values
        if(router != NULL){
            const char *value = uci_lookup_option_string(ctx, router, key);
            if(value != NULL){
                ret = 0;
                sprintf(retBuffer,"%s",value);
            }
        }
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

int32_t getUciConfig(const char *file, const char *node, const char *key,char *retBuffer)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, file, &p) == UCI_OK)
    {
        struct uci_section *router = uci_lookup_section(ctx, p, node);
        //lookup values
        if(router != NULL){
            const char *value = uci_lookup_option_string(ctx, router, key);
            if(value != NULL){
                ret = 0;
                sprintf(retBuffer,"%s",value);
            }
        }
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

int32_t setUciConfig(const char *file, const char *node, const char *key,char *Buffer)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, file, &p) == UCI_OK)
    {
        struct uci_section *router = uci_lookup_section(ctx, p, node);
        //lookup values
        if(router != NULL){
			struct uci_ptr ptr = { .p = p, .s = router};
			uciSetValue(ctx,&ptr,key,Buffer);
			uci_save(ctx,p);
			uci_commit(ctx,&p,false);
			ret = 0;
        }
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

int32_t getServerAddress(char *retBuffer)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = 0;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, "sicloud", &p) == UCI_OK)
    {
        struct uci_section *addr= uci_lookup_section(ctx, p, "addr");
        //lookup values
        if(addr!= NULL){
            const char *value = uci_lookup_option_string(ctx, addr, "ip");
			int len = strlen(value);
            if(value != NULL){
                sprintf(retBuffer,"%s",value);
            }else {
				ret = -1;
			}

			value = uci_lookup_option_string(ctx, addr, "port");
            if(value != NULL){
                sprintf(retBuffer+len,":%s",value);
            }else{
				ret = -1;
			}


        }else{
			ret = -1;
		}

        uci_unload(ctx,p);
    }else{
		ret = -1;
	}
    uci_free_context(ctx);
    return ret;
}

int32_t setSfHardwareConfig(char *key,char *inBuffer)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, "siwifi", &p) == UCI_OK)
    {
        struct uci_section *router = uci_lookup_section(ctx, p, "hardware");
        //lookup values
        if(router != NULL){
			struct uci_ptr ptr = { .p = p, .s = router};
			uciSetValue(ctx,&ptr,key,inBuffer);
			uci_save(ctx,p);
			uci_commit(ctx,&p,false);
			ret = 0;
        }
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}
int32_t getRouterValueFromSection(struct uci_context *ctx,struct uci_section *router,const char *key,char *retBuffer)
{
    int ret = -1;
    //lookup values
    if(router != NULL){
        const char *value = uci_lookup_option_string(ctx, router, key);
        if(value != NULL){
            ret = 0;
            sprintf(retBuffer,"%s",value);
        }
    }
    return ret;
}


int32_t deleteRouterOptionFromUci(const char *key)
{
    if(!key) return -1;
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, "siserver", &p) == UCI_OK)
    {
        struct uci_section *router = uci_lookup_section(ctx, p, "cloudrouter");
        //lookup values
        if(router != NULL){
            struct uci_option *option = uci_lookup_option(ctx,router,key);
            if(option != NULL){
                uciDeleteOption(ctx,p,router,key);
                uci_save(ctx,p);
                uci_commit(ctx,&p,false);
            }
        }
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

int32_t has_binded()
{
    // LOG( "[server]%s\n",__func__);
#ifdef USE_FILE_SAVE_OBJECTID
    return has_binded_file();
#else
    char bind[20] = "";
    int32_t getBind = getRouterValueUci("bind",bind);
    return (getBind == 0 && !strcmp(bind,"1")) ? 0 : -1;
#endif
}

int32_t delete_bind(int32_t all)
{
    LOG( "[server]%s, %d\n",__func__, all);
#ifdef USE_FILE_SAVE_OBJECTID
    return delete_bind_file(all);
#else
    int ret = setRouterValueToUci("cloudrouter","bind","0");
    //clear binder
    setRouterValueToUci("cloudrouter","binder","");
    if(all == 1){
        //delete all remote info in local
        setRouterValueToUci("cloudrouter","routerid","");
        setRouterValueToUci("cloudrouter","subid","");
        setRouterValueToUci("cloudrouter","stateid","");
        setRouterValueToUci("cloudrouter","wifinum","0");
        setRouterValueToUci("cloudrouter","devnum","0");
        //delete wifi objects
        deleteRouterOptionFromUci("wifi");
        //delete device objects
        deleteRouterOptionFromUci("device");
        deleteRouterOptionFromUci("managernum");
        deleteRouterOptionFromUci("manager");
    }
    return (ret == 0) ? 0 : -1;
#endif

}

//int32_t save_bind(router_table *rt, router_sub_table *subt, router_state_table *rst,
//        wireless_table *wt, int32_t wnum, device_table *dst, int32_t dnum,char *binder_id, int32_t bind)
int32_t save_bind(router_table *rt,char *binder_id, int32_t bind)
{
	LOG( "[server] save_bind--binder_id %s \n",binder_id);
#ifdef USE_FILE_SAVE_OBJECTID
    return save_bind_file(rt,subt,rst,wt,wnum,dst,dnum,binder_id);
#else
    int32_t ret = -1;
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    uci_set_confdir(ctx, "/etc/config");
    uci_load(ctx, "siserver", &p);

    struct uci_section *router = uci_lookup_section(ctx, p, "cloudrouter");
    //lookup values
    if(router != NULL){
        struct uci_ptr ptr = { .p = p, .s = router};
        char bind_str[4] = "";
        sprintf(bind_str, "%d", bind);
        uciSetValue(ctx,&ptr,"bind",bind_str);
        uciSetValue(ctx,&ptr,"binder",binder_id);
        uciSetValue(ctx,&ptr,"routerid",rt->objectid);
		/*
        if(strlen(rt->name) != 0) uciSetValue(ctx,&ptr,"routername",rt->name);
        if(strlen(rt->mac) != 0) uciSetValue(ctx,&ptr,"mac",rt->mac);
        uciSetValue(ctx,&ptr,"subid",subt->objectid);
        uciSetValue(ctx,&ptr,"stateid",rst->objectid);
        //add wifi devices
        struct uci_option *option = uci_lookup_option(ctx,router,"wifi");
        if(option != NULL) uciDeleteOption(ctx,p,router,"wifi");
        char num[4] = "";
        sprintf(num,"%d",wnum);
        uciSetValue(ctx,&ptr,"wifinum",num);
        int i = 0;
        for(i = 0; i < wnum; i++){
            //cJSON_AddStringToObject(wireless, "__object", wt[i].objectid);
            struct uci_ptr ptr = { .p = p, .s = router};
            ptr.o      = NULL;
            ptr.option = "wifi";
            ptr.value  = wt[i].objectid;
            uci_add_list(ctx,&ptr);
        }

        option = uci_lookup_option(ctx,router,"device");
        if(option != NULL) uciDeleteOption(ctx,p,router,"device");
        char devNum[4] = "";
        sprintf(devNum,"%d",dnum);
        uciSetValue(ctx,&ptr,"devnum",devNum);
        for(i = 0; i < dnum; i++){
            //cJSON_AddStringToObject(wireless, "__object", wt[i].objectid);
            struct uci_ptr ptr = { .p = p, .s = router};
            ptr.o      = NULL;
            ptr.option = "device";
            ptr.value  = dst[i].objectid;
            uci_add_list(ctx,&ptr);
        }

		*/
        LOG( "[server] save bind--2\n");
        uci_save(ctx,p);
        uci_commit(ctx,&p,false);
        ret = 0;
    }

    uci_unload(ctx,p);
    uci_free_context(ctx);
    LOG( "[server] save_bind--2\n");
    return ret;
#endif
}

const char *getTableString(int32_t table)
{
    const char *tmp = NULL;
    switch(table){
        case TABLE_ROUTER:
            tmp = "routerid";
            break;
        case TABLE_ROUTER_SUB:
            tmp = "subid";
            break;
        case TABLE_ROUTER_STATE:
            tmp = "stateid";
            break;
        case TABLE_ROUTER_WIFI:
            tmp = "wifi";
            break;
        case TABLE_ROUTER_DEVICE:
            tmp = "device";
            break;
        default:
            tmp = "unknown";
            break;
    }
    return tmp;
}

int32_t get_table(char *object, int32_t table, int32_t order)
{
    LOG("[server]%s, %d, %d\n", __func__, table, order);
#ifdef USE_FILE_SAVE_OBJECTID
    return get_table_file(object,table,order);
#else
    int32_t ret = -1;
    const char *table_name = getTableString(table);
    if(table == TABLE_ROUTER || table == TABLE_ROUTER_SUB || table == TABLE_ROUTER_STATE){
        ret = getRouterValueUci(table_name,object);
    }else if(order >= 0){
        ret = getRouterListValueFromUci(table_name,object,order);
    }else{
        LOG("[server] unknow type or Illegal order \n");
    }

    return ret;
#endif
}

int32_t get_router_table_value(router_table *rt)
{
    LOG("[server] %s \n", __func__);
    //fetch from local uhttpd-luci siwifi api
    int32_t res = 0;
    char postData[36] = "{\"version\":\"V10\"}";
    struct HttpResonseData response;

    /****************get init info********************/
    response.size = 0;
    int ret = postDataToHttpdCommon(LOCAL_COMMAND_INIT_INFO,postData,&response);                //local getInitInfo interface
    res = ret ? -1:res;
    LOG("get response----ret=%d---data: %s\n",ret,(response.size > 0) ? (char*)(response.data) : "NULL");
    if(ret == CURLE_OK && response.size > 0){
        //parse http response data
        cJSON *root = NULL;
        root = cJSON_Parse((char*)response.data);
        if(root){
            int32_t code = -1;
            int32_t luci_init_ret_code = -1;
            luci_init_ret_code = get_json_value_int(root,"code",&code);
            if(luci_init_ret_code == 0 && code == 0){
                //todo: check with rom version
                getSfHardwareConfig("romtime",rt->romtime);
                get_json_value_string(root,"mac",rt->mac);
                get_json_value_string(root,"name",rt->name);
                get_json_value_string(root,"romversion",rt->romversion);
                get_json_value_string(root,"sn",rt->sn);
                get_json_value_string(root,"hardware",rt->hardware);
                get_json_value_string(root,"account",rt->account);
                get_json_value_int(root,"romversionnumber",&rt->romversionnumber);
                get_json_value_int(root,"romtype",&rt->romtype);
                get_json_value_int(root,"disk",&rt->disk);
				get_json_value_int(root, "zigbee", &rt->zigbee);
				get_json_value_int(root, "storage", &rt->storage);
            }else{
                LOG( "[ssst]===function:%s=== maybe luci init_info result error!!!==luci_init_ret_code=%d, code=%d===", __func__, luci_init_ret_code, code);
                res = -1;
            }
            cJSON_Delete(root);
        }else{
            LOG( "[ssst]===function:%s===parse response.data failed!!!====", __func__);
            res = -1;
        }
    }
    if(response.size > 0) free(response.data);
    LOG( "[ssst]===function:%s===after init_info res=%d===", __func__, res);

    /****************get router feature info*****************/
    response.size = 0;
    ret = postDataToHttpdCommon(LOCAL_COMMAND_GET_ROUTER_FEATURE,postData,&response);
    res = ret ? -1:res;
    LOG("get response----ret=%d---data: %s\n",ret,(response.size > 0) ? (char*)(response.data) : "NULL");
    if(ret == CURLE_OK && response.size > 0){
        //parse http response data
        cJSON *root = NULL;
        root = cJSON_Parse((char*)response.data);
        if(root){
            int32_t code = -1;
            int32_t luci_getrouterfeature_ret_code = -1;
            luci_getrouterfeature_ret_code = get_json_value_int(root,"code",&code);
            if(luci_getrouterfeature_ret_code == 0 && code == 0){
                get_json_value_int(root,"onlineWarn",&rt->feat.onlineWarn);
                get_json_value_int(root,"usb",&rt->feat.usb);
                get_json_value_int(root,"led",&rt->feat.led);
                get_json_value_int(root,"ac",&rt->feat.ac);
                get_json_value_int(root,"wifi",&rt->feat.wifi);
                get_json_value_int(root,"guestWifi",&rt->feat.guestWifi);
                get_json_value_int(root,"externalPA",&rt->feat.externalPA);
                get_json_value_int(root,"dhcp",&rt->feat.dhcp);
                get_json_value_int(root,"wds",&rt->feat.wds);
                get_json_value_int(root,"virtualServer",&rt->feat.virtualServer);
                get_json_value_int(root,"dmz",&rt->feat.dmz);
                get_json_value_int(root,"upnp",&rt->feat.upnp);
                get_json_value_int(root,"ddns",&rt->feat.ddns);
                get_json_value_int(root,"staticRouter",&rt->feat.staticRouter);
                get_json_value_int(root,"homeControl",&rt->feat.homeControl);
                get_json_value_int(root,"leaseWiFi",&rt->feat.leaseWiFi);
                get_json_value_int(root,"leaseWeb",&rt->feat.leaseWeb);
                get_json_value_int(root,"freqInter",&rt->feat.freqInter);
            }else{
                LOG( "[ssst]===function:%s=== maybe luci getrouterfeature result error!!!==getrouterfeature=%d, code=%d===", __func__, luci_getrouterfeature_ret_code, code);
                res = -1;
            }
            if(root) cJSON_Delete(root);
        }else{
            LOG( "[ssst]===function:%s===parse response.data failed!!!====", __func__);
            res = -1;
        }
    }
    if(response.size > 0) free(response.data);
    LOG( "[ssst]===function:%s===after getrouterfeature res=%d===", __func__, res);

    /****************get qos info*****************/
/*
    response.size = 0;
    ret = postDataToHttpdCommon(LOCAL_COMMAND_QOS_INFO,postData,&response);                 //local qos_info interface
    res = ret ? -1:res;
    LOG("get response----ret=%d---data: %s\n",ret,(response.size > 0) ? (char*)(response.data) : "NULL");
    if(ret == CURLE_OK && response.size > 0){
        //parse http response data
        cJSON *root = NULL;
        root = cJSON_Parse((char*)response.data);
        if(root){
            int32_t code = -1;
            int32_t luci_qos_ret_code = -1;
            luci_qos_ret_code = get_json_value_int(root,"code",&code);
            if(luci_qos_ret_code == 0 && code == 0){
                get_json_value_int(root,"enable",&rt->qos.enable);
                get_json_value_int(root,"mode",&rt->qos.mode);
            }else{
                LOG( "[ssst]===function:%s=== maybe luci qos_info result error!!!==luci_qos_ret_code=%d, code=%d===", __func__, luci_qos_ret_code, code);
                res = -1;
            }
            if(root) cJSON_Delete(root);
        }else{
            LOG( "[ssst]===function:%s===parse response.data failed!!!====", __func__);
            res = -1;
        }
    }
    if(response.size > 0) free(response.data);
    LOG( "[ssst]===function:%s===after qos_info res=%d===", __func__, res);
*/

    /****************get wifi_filter info*****************/
/*
    response.size = 0;
    ret = postDataToHttpdCommon(LOCAL_COMMAND_GET_WIFI_FILTER,postData,&response);                 //local get_wifi_filter interface
    res = ret ? -1:res;
    LOG("get response----ret=%d---data: %s\n",ret,(response.size > 0) ? (char*)(response.data) : "NULL");
    if(ret == CURLE_OK && response.size > 0){
        //parse http response data
        cJSON *root = NULL;
        root = cJSON_Parse((char*)response.data);
        if(root){
            int32_t code = -1;
            int32_t luci_wififilter_ret_code = -1;
            luci_wififilter_ret_code = get_json_value_int(root,"code",&code);
            if(luci_wififilter_ret_code == 0 && code == 0){
                get_json_value_int(root,"enable",&rt->wifi_filter.enable);
                get_json_value_int(root,"mode",&rt->wifi_filter.mode);
            }else{
                LOG( "[ssst]===function:%s=== maybe luci get_wifi_filter result error!!!==luci_wififilter_ret_code=%d, code=%d===", __func__, luci_wififilter_ret_code, code);
                res = -1;
            }
            if(root) cJSON_Delete(root);
        }else{
            LOG( "[ssst]===function:%s===parse response.data failed!!!====", __func__);
            res = -1;
        }
    }
    if(response.size > 0) free(response.data);
    LOG( "[ssst]===function:%s===after get_wifi_filter res=%d===", __func__, res);
*/
    /****************get wan_type info*****************/
	/*
    response.size = 0;
    ret = postDataToHttpdCommon(LOCAL_COMMAND_GET_WAN_TYPE,postData,&response);                 //local get_wan_type interface
    res = ret ? -1:res;
    LOG("get response----ret=%d---data: %s\n",ret,(response.size > 0) ? (char*)(response.data) : "NULL");
    if(ret == CURLE_OK && response.size > 0){
        //parse http response data
        cJSON *root = NULL;
        root = cJSON_Parse((char*)response.data);
        if(root){
            int32_t code = -1;
            int32_t luci_getwantype_ret_code = -1;
            luci_getwantype_ret_code = get_json_value_int(root,"code",&code);
            if(luci_getwantype_ret_code == 0 && code == 0){
                get_json_value_int(root,"type",&rt->wan_type.type);
                get_json_value_int(root,"autodns",&rt->wan_type.autodns);
                get_json_value_string(root,"pppname",rt->wan_type.pppname);
                get_json_value_string(root,"ppppwd",rt->wan_type.ppppwd);
                get_json_value_string(root,"ip",rt->wan_type.ip);
                get_json_value_string(root,"mask",rt->wan_type.mask);
                get_json_value_string(root,"dns1",rt->wan_type.dns1);
                get_json_value_string(root,"dns2",rt->wan_type.dns2);
                get_json_value_string(root,"gateway",rt->wan_type.gateway);
            }else{
                LOG( "[ssst]===function:%s=== maybe luci get_wan_type result error!!!==luci_getwantype_ret_code=%d, code=%d===", __func__, luci_getwantype_ret_code, code);
                res = -1;
            }
            if(root) cJSON_Delete(root);
        }else{
            LOG( "[ssst]===function:%s===parse response.data failed!!!====", __func__);
            res = -1;
        }
    }
    if(response.size > 0) free(response.data);
    LOG( "[ssst]===function:%s===after get_wan_type res=%d===", __func__, res);

    ret = get_wireless_table_info(&rt->wireless, &rt->wnum);
    res = ret ? -1:res;
    LOG( "[ssst]===function:%s===after get_wireless_table_info res=%d===", __func__, res);
*/
    return res;
}

int32_t get_router_state_table_value(router_state_table *rst)
{
    LOG("[server] %s \n", __func__);
    //fetch from local uhttpd-luci siwifi api
    int32_t res = 0;
    char postData[36] = "{\"version\":\"V10\"}";
    struct HttpResonseData response;
    response.size = 0;
    int ret = postDataToHttpdCommon(LOCAL_COMMAND_MAIN_STATUS,postData,&response);
    LOG("get response----ret=%d---data: %s\n",ret,(response.size > 0) ? (char*)(response.data) : "NULL");
    if(ret == CURLE_OK && response.size > 0){
        //parse http response data
        cJSON *root = NULL;
        root = cJSON_Parse((char*)response.data);
        if(root){
            int32_t ret_getsn = getSfHardwareConfig("sn",rst->sn);
            int32_t code = -1;
            int32_t luci_mainstatus_ret_code = get_json_value_int(root,"code",&code);
            if(ret_getsn == 0 && luci_mainstatus_ret_code == 0 && code == 0){
                get_json_value_int(root,"status",&rst->status);
                get_json_value_int(root,"cpuload",&rst->cpuload);
                get_json_value_int(root,"devicecount",&rst->devicecount);
                get_json_value_int(root,"downloadingcount",&rst->downloadingcount);
                get_json_value_int(root,"downspeed",&rst->downspeed);
                get_json_value_int(root,"memoryload",&rst->memoryload);
                get_json_value_int(root,"upspeed",&rst->upspeed);
                get_json_value_int(root,"useablespace",&rst->useablespace);
            }else{
                LOG( "[ssst]===function:%s failed!!!==ret_getsn=%d, luci_mainstatus_ret_code=%d, code=%d=", __func__, ret_getsn, luci_mainstatus_ret_code, code);
                res = -1;
            }
            if(root) cJSON_Delete(root);
        }else{
            LOG( "[ssst]===function:%s===parse response.data failed!!!===", __func__);
            res = -1;
        }
    }else{
        LOG( "[ssst]===function:%s failed!!!", __func__);
        res = -1;
    }
    if(response.size > 0) free(response.data);
    LOG( "[ssst]===function:%s res = %d", __func__, res);
    return res;
}

int32_t get_router_sub_table_value(router_sub_table *subt)
{
    LOG("[server] %s \n", __func__);
    int32_t ret = 0;
    ret = getSfHardwareConfig("sn",subt->sn);
    return ret;
}

int32_t get_wireless_table_info(wireless_table **wt, int32_t *wnum)
{
    LOG("[server] %s \n", __func__);
    //fetch from local uhttpd-luci siwifi api
    int32_t res = 0;
    char postData[36] = "{\"version\":\"V10\"}";
    struct HttpResonseData response;
    response.size = 0;
    int ret = postDataToHttpdCommon(LOCAL_COMMAND_WIFI_DETAIL,postData,&response);
    LOG("get response----ret=%d---data: %s\n",ret,(response.size > 0) ? (char*)(response.data) : "NULL");
    if(ret == CURLE_OK && response.size > 0){
        //parse http response data
        cJSON *root = NULL;
        root = cJSON_Parse((char*)response.data);
        if(root){
            //getSfHardwareConfig("sn",rst->sn);
            int32_t code = -1;
            int32_t luci_wifidetail_ret_code = get_json_value_int(root,"code",&code);
            if(luci_wifidetail_ret_code == 0 && code == 0){
                cJSON *item = cJSON_GetObjectItem(root,"info");
                int32_t count = item ? cJSON_GetArraySize(item) : 0;
                if(item && count > 0){
                    int32_t i = 0;
                    wireless_table* wt_array = (wireless_table *)malloc(count * sizeof(wireless_table));
                    memset((void *)wt_array, 0, count * sizeof(wireless_table));
                    if(wt_array){
						for(i = 0; i < count; i++){
							cJSON *subitem = cJSON_GetArrayItem(item, i);
							if(subitem){
								getSfHardwareConfig("sn",(wt_array + i)->sn);
								get_json_value_int(subitem,"signal",&((wt_array + i)->signal));
								get_json_value_int(subitem,"enable",&((wt_array + i)->enable));
								get_json_value_int(subitem,"channel",&((wt_array + i)->channel));
								get_json_value_string(subitem,"band",(wt_array + i)->band);
								get_json_value_string(subitem,"ifname",(wt_array + i)->ifname);
								get_json_value_string(subitem,"mac",(wt_array + i)->mac);
								get_json_value_string(subitem,"encryption",(wt_array + i)->encryption);
								get_json_value_string(subitem,"password",(wt_array + i)->password);
								get_json_value_string(subitem,"ssid",(wt_array + i)->ssid);
								get_json_value_string(subitem,"net_type",(wt_array + i)->network);
								(wt_array + i)->reserverd = 0;
							}
						}
                        *wt = wt_array;
                        *wnum = count;
                    }else{
                        res = -1;
                        LOG("[server] not enough memory \n");
                    }
                }else{
                    LOG( "[ssst]===function:%s===get wifi info failed ===item=%p===count==%d===", __func__, item, count);
                    res = -1;
                }
            }else{
                LOG( "[ssst]===function:%s===get wifi_detail return code failed!!!==luci_wifidetail_ret_code=%d==code==%d=", __func__, luci_wifidetail_ret_code, code);
                res = -1;
            }
            if(root) cJSON_Delete(root);
        }else{
            res = -1;
            LOG( "[ssst]===function:%s===parse response.data failed!!!===", __func__);
        }
    }else{
        res = -1;
    }
    if(response.size > 0) free(response.data);
    return res;
}

int32_t get_5G_ssid(char *buf)
{
    int32_t wnum = 0, i = 0, ret = -1;
    wireless_table *wt = NULL;

	ret = get_wireless_table_info(&wt, &wnum);
	if(ret != 0){
		LOG("[ssst]===function:%s===call get_wireless_table_info failed!!!===ret=%d===", __func__, ret);
		return ret;
	}

	for(i = 0; i < wnum; i++){
		if(!strncmp(wt[i].band, "5G", 2) && !strncmp(wt[i].network, "lan", 3) && wt[i].ssid){
			memcpy(buf, wt[i].ssid, 32);
			return 0;
		}
	}
	return ret;
}

int32_t get_devices_info(device_table **dst, int32_t *dnum)
{
    LOG("[server] %s \n", __func__);
    //fetch from local uhttpd-luci siwifi api
    int32_t res = 0;
    char postData[36] = "{\"version\":\"V10\",\"type\":1}";
    struct HttpResonseData response;
    response.size = 0;
    int ret = postDataToHttpdCommon(LOCAL_COMMAND_DEVICE_LIST,postData,&response);
    LOG("get response----ret=%d---data: %s\n",ret,(response.size > 0) ? (char*)(response.data) : "NULL");

#if KEVIN_TEST
    int32_t tmp_count = 0;
    int32_t tmp_num = strlen((char *)response.data)/50+1;
    LOG( "\n======device_tmp_num=====%d===", tmp_num);
    char tmp_buf[51]="";
    for(tmp_count=0; tmp_count<tmp_num; tmp_count++){
        memcpy(tmp_buf, (char *)response.data+tmp_count*50, 50);
        tmp_buf[50] = '\0';
        usleep(100);
        LOG( "device_tmp_buf[%d]=========%s=======", tmp_count, tmp_buf);
    }

#endif
    if(ret == CURLE_OK && response.size > 0){
        //parse http response data
        cJSON *root = NULL;
        root = cJSON_Parse((char*)response.data);
        //getSfHardwareConfig("sn",rst->sn);
        if(root){
            int32_t code = -1;
            int32_t luci_devicelist_ret_code = get_json_value_int(root,"code",&code);
            if(luci_devicelist_ret_code == 0 && code == 0){
                cJSON *item = cJSON_GetObjectItem(root,"list");
                int32_t count = item ? cJSON_GetArraySize(item) : 0;
                if(item && count > 0){
                    int32_t i = 0;
                    device_table* dt_array = (device_table *)malloc(count * sizeof(device_table));
                    memset((void *)dt_array, 0, count * sizeof(device_table));
                    if(dt_array){
                        for(i = 0; i < count; i++){
                            cJSON *subitem = cJSON_GetArrayItem(item, i);
                            getSfHardwareConfig("sn",(dt_array + i)->sn);
                            get_json_value_string(subitem,"hostname",(dt_array + i)->originname);
                            get_json_value_string(subitem,"nickname",(dt_array + i)->hostname);
                            get_json_value_string(subitem,"mac",(dt_array + i)->mac);
                            get_json_value_int(subitem,"online",&((dt_array + i)->online));
                            get_json_value_string(subitem,"ip",(dt_array + i)->ip);
                            get_json_value_int(subitem,"port",&((dt_array + i)->port));
                            get_json_value_string(subitem,"dev",(dt_array + i)->dev);
                            get_json_value_string(subitem,"icon",(dt_array + i)->icon);
                            //get authority
                            get_json_pointer_attr_int(subitem,"authority","internet",&((dt_array + i)->authority.internet));
                            get_json_pointer_attr_int(subitem,"authority","wan",&((dt_array + i)->authority.wan));
                            get_json_pointer_attr_int(subitem,"authority","lan",&((dt_array + i)->authority.lan));
                            get_json_pointer_attr_int(subitem,"authority","notify",&((dt_array + i)->authority.notify));
                            get_json_pointer_attr_int(subitem,"authority","speedlvl",&((dt_array + i)->authority.speedlvl));
                            get_json_pointer_attr_int(subitem,"authority","disk",&((dt_array + i)->authority.disk));
                            get_json_pointer_attr_int(subitem,"authority","limitup",&((dt_array + i)->authority.limitup));
                            get_json_pointer_attr_int(subitem,"authority","limitdown",&((dt_array + i)->authority.limitdown));
                            //get speed
                            get_json_pointer_attr_int(subitem,"speed","maxdownloadspeed",&((dt_array + i)->speed.maxdownloadspeed));
                            get_json_pointer_attr_int(subitem,"speed","uploadtotal",&((dt_array + i)->speed.uploadtotal));
                            get_json_pointer_attr_int(subitem,"speed","upspeed",&((dt_array + i)->speed.upspeed));
                            get_json_pointer_attr_int(subitem,"speed","downspeed",&((dt_array + i)->speed.downspeed));
                            get_json_pointer_attr_int(subitem,"speed","online",&((dt_array + i)->speed.online));
                            get_json_pointer_attr_int(subitem,"speed","maxuploadspeed",&((dt_array + i)->speed.maxuploadspeed));
                            get_json_pointer_attr_int(subitem,"speed","downloadtotal",&((dt_array + i)->speed.downloadtotal));
                            get_json_pointer_attr_int(subitem,"speed","online",&((dt_array + i)->speed.online));
                        }
                        *dst = dt_array;
                        *dnum = count;
                    }else{
                        LOG("[server] not enough memory \n");
                        res = -1;
                    }
                }else{
                    LOG( "[ssst]===function:%s===get devicelist failed!!!====item=%p, count=%d===", __func__, item, count);
                    res = -1;
                }
            }else{
                LOG( "[ssst]===function:%s===get luci devicelist return code failed!!!====luci_deviceinfo_ret_code=%d, code=%d===", __func__, luci_devicelist_ret_code, code);
                res = -1;
            }
            cJSON_Delete(root);
//            res = 0;
        }else{
            LOG( "[ssst]===function:%s=====parse response.data failed!!!====", __func__);
            res = -1;
        }
    }else{
        res = -1;
    }
    if(response.size > 0) free(response.data);
    return res;
}

int32_t add_router_to_json(cJSON *root ,int32_t isknown, router_table router)
{
    LOG("[server] %s=====isknown=%d=== \n", __func__, isknown);
    int32_t res = 0;
    int32_t ret = 0;
    router_table rt;
    if(isknown){
        memcpy(&rt, &router, sizeof(router));
    }else{
        memset(&rt, 0, sizeof(router_table));
        ret = get_router_table_value(&rt);
        res = ret ? -1:res;
        if(res != 0){
            LOG( "[ssst]===function:%s===call get_router_table_value failed!!!===res=%d===", __func__, res);
            return res;
        }
    }
    cJSON *root_rt = cJSON_CreateObject();
    if(root_rt){
        cJSON_AddStringToObject(root_rt, "sn", rt.sn);
        cJSON_AddStringToObject(root_rt, "mac", rt.mac);
        cJSON_AddStringToObject(root_rt, "name", rt.name);
        cJSON_AddStringToObject(root_rt, "romversion", rt.romversion);
        cJSON_AddStringToObject(root_rt, "romtime", rt.romtime);
        cJSON_AddStringToObject(root_rt, "hardware", rt.hardware);
        cJSON_AddStringToObject(root_rt, "account", rt.account);
        cJSON_AddNumberToObject(root_rt, "romversionnumber", rt.romversionnumber);
        cJSON_AddNumberToObject(root_rt, "romtype", rt.romtype);
        cJSON_AddNumberToObject(root_rt, "disk", rt.disk);
		cJSON_AddNumberToObject(root_rt, "zigbee", rt.zigbee);
		cJSON_AddNumberToObject(root_rt, "storage", rt.storage);

        cJSON *root_rt_ft = cJSON_CreateObject();
		if(root_rt_ft){
			cJSON_AddNumberToObject(root_rt_ft, "onlineWarn", rt.feat.onlineWarn);
			cJSON_AddNumberToObject(root_rt_ft, "usb", rt.feat.usb);
			cJSON_AddNumberToObject(root_rt_ft, "led", rt.feat.led);
			cJSON_AddNumberToObject(root_rt_ft, "ac", rt.feat.ac);
			cJSON_AddNumberToObject(root_rt_ft, "wifi", rt.feat.wifi);
			cJSON_AddNumberToObject(root_rt_ft, "guestWifi", rt.feat.guestWifi);
			cJSON_AddNumberToObject(root_rt_ft, "externalPA", rt.feat.externalPA);
			cJSON_AddNumberToObject(root_rt_ft, "dhcp", rt.feat.dhcp);
			cJSON_AddNumberToObject(root_rt_ft, "wds", rt.feat.wds);
			cJSON_AddNumberToObject(root_rt_ft, "virtualServer", rt.feat.virtualServer);
			cJSON_AddNumberToObject(root_rt_ft, "dmz", rt.feat.dmz);
			cJSON_AddNumberToObject(root_rt_ft, "upnp", rt.feat.upnp);
			cJSON_AddNumberToObject(root_rt_ft, "ddns", rt.feat.ddns);
			cJSON_AddNumberToObject(root_rt_ft, "staticRouter", rt.feat.staticRouter);
			cJSON_AddNumberToObject(root_rt_ft, "homeControl", rt.feat.homeControl);
			cJSON_AddNumberToObject(root_rt_ft, "leaseWiFi", rt.feat.leaseWiFi);
			cJSON_AddNumberToObject(root_rt_ft, "leaseWeb", rt.feat.leaseWeb);
			cJSON_AddNumberToObject(root_rt_ft, "freqInter", rt.feat.freqInter);
			cJSON_AddItemToObject(root_rt, "feature", root_rt_ft);
        }else{
            LOG( "function:%s create root_rt_ft object failed!!!", __func__);
            res = -1;
        }
/*
        cJSON *root_rt_qos = cJSON_CreateObject();
        if(root_rt_qos){
            cJSON_AddNumberToObject(root_rt_qos, "enable", rt.qos.enable);
            cJSON_AddNumberToObject(root_rt_qos, "mode", rt.qos.mode);
            cJSON_AddItemToObject(root_rt, "qos", root_rt_qos);
        }else{
            LOG( "function:%s create root_rt_qos object failed!!!", __func__);
            res = -1;
        }

        cJSON *root_rt_wifi_filter = cJSON_CreateObject();
        if(root_rt_wifi_filter){
            cJSON_AddNumberToObject(root_rt_wifi_filter, "enable", rt.wifi_filter.enable);
            cJSON_AddNumberToObject(root_rt_wifi_filter, "mode", rt.wifi_filter.mode);
            cJSON_AddItemToObject(root_rt, "wifi_filter", root_rt_wifi_filter);
        }else{
            LOG( "function:%s create root_rt_wifi_filter object failed!!!", __func__);
            res = -1;
        }

        cJSON *root_rt_wan_type = cJSON_CreateObject();
        if(root_rt_wan_type){
            cJSON_AddNumberToObject(root_rt_wan_type, "type", rt.wan_type.type);
            cJSON_AddNumberToObject(root_rt_wan_type, "autodns", rt.wan_type.autodns);
            cJSON_AddStringToObject(root_rt_wan_type, "pppname", rt.wan_type.pppname);
            cJSON_AddStringToObject(root_rt_wan_type, "ppppwd", rt.wan_type.ppppwd);
            cJSON_AddStringToObject(root_rt_wan_type, "ip", rt.wan_type.ip);
            cJSON_AddStringToObject(root_rt_wan_type, "mask", rt.wan_type.mask);
            cJSON_AddStringToObject(root_rt_wan_type, "dns1", rt.wan_type.dns1);
            cJSON_AddStringToObject(root_rt_wan_type, "dns2", rt.wan_type.dns2);
            cJSON_AddStringToObject(root_rt_wan_type, "gateway", rt.wan_type.gateway);
            cJSON_AddItemToObject(root_rt, "wan_type", root_rt_wan_type);
        }else{
            LOG( "function:%s create root_rt_wan_type object failed!!!", __func__);
            res = -1;
        }

        cJSON *root_rt_wireless = cJSON_CreateObject();
        if(root_rt_wireless){
            int32_t i = 0;
            char wifi_type[32] = "";
            char justtmp[32] = "";
            for(i=0; i<rt.wnum; i++){
				if(rt.wireless[i].enable == 1){
					cJSON *root_wt_each = cJSON_CreateObject();
					cJSON_AddStringToObject(root_wt_each, "sn", rt.wireless[i].sn);
					cJSON_AddNumberToObject(root_wt_each, "signal", rt.wireless[i].signal);
					cJSON_AddNumberToObject(root_wt_each, "enable", rt.wireless[i].enable);
					cJSON_AddNumberToObject(root_wt_each, "channel", rt.wireless[i].channel);
					cJSON_AddStringToObject(root_wt_each, "band", rt.wireless[i].band);
					cJSON_AddStringToObject(root_wt_each, "ifname", rt.wireless[i].ifname);
					cJSON_AddStringToObject(root_wt_each, "mac", rt.wireless[i].mac);
					cJSON_AddStringToObject(root_wt_each, "encryption", rt.wireless[i].encryption);
					cJSON_AddStringToObject(root_wt_each, "password", rt.wireless[i].password);
					cJSON_AddStringToObject(root_wt_each, "ssid", rt.wireless[i].ssid);
					strcpy(justtmp, rt.wireless[i].band);
					strrpl(justtmp, '.', '_');
					sprintf(wifi_type, "wifi%s", justtmp);
					cJSON_AddItemToObject(root_rt_wireless, wifi_type, root_wt_each);
				}
            }
            cJSON_AddItemToObject(root_rt, "routerwifi", root_rt_wireless);
            free(rt.wireless);
        }else{
            LOG( "function:%s create root_rt_wireless object failed!!!", __func__);
            res = -1;
        }
        */
        cJSON_AddItemToObject(root, "router", root_rt);
    }else{
        LOG( "function:%s create root_rt object failed!!!", __func__);
        res = -1;
    }
    return res;
}

int32_t add_router_sub_to_json(cJSON *root, int32_t isknown, router_sub_table routersub)
{
    LOG("[server] %s=====isknown=%d=== \n", __func__, isknown);
    int32_t res = 0;
    int32_t ret = 0;
    router_sub_table subt;
    if(isknown){
        memcpy(&subt, &routersub, sizeof(routersub));
    }else{
        memset(&subt, 0, sizeof(router_sub_table));
        ret = get_router_sub_table_value(&subt);
        res = ret ? -1:res;
        if(res != 0){
            LOG( "[ssst]===function:%s===call get_router_sub_table_value failed!!!===res=%d===", __func__, res);
            return res;
        }
    }
    cJSON *root_subt = cJSON_CreateObject();
    if(root_subt){
        cJSON_AddStringToObject(root_subt, "sn", subt.sn);
        cJSON_AddItemToObject(root, "routersub", root_subt);
    }else{
        LOG( "function:%s create root_rt object failed!!!", __func__);
        res = -1;
    }
    return res;
}

int32_t add_router_state_to_json(cJSON *root, int32_t isknown, router_state_table routerstate)
{
    LOG("[server] %s=====isknown=%d=== \n", __func__, isknown);
    int32_t res = 0;
    int32_t ret = 0;
    cJSON *router_state_root = NULL;
    router_state_table rst;
    if(isknown){
        memcpy(&rst, &routerstate, sizeof(routerstate));
    }else{
        memset(&rst, 0, sizeof(router_state_table));
        ret = get_router_state_table_value(&rst);
        res = ret ? -1:res;
        if(res != 0){
            LOG( "[ssst]===function:%s===call get_router_state_table_value failed!!!===res=%d===", __func__, res);
            return res;
        }
    }
    router_state_root = cJSON_CreateObject();
    if(router_state_root){
        cJSON_AddNumberToObject(router_state_root, "cpuload", rst.cpuload);
        cJSON_AddNumberToObject(router_state_root, "devicecount", rst.devicecount);
        cJSON_AddNumberToObject(router_state_root, "downloadingcount", rst.downloadingcount);
        cJSON_AddNumberToObject(router_state_root, "downspeed", rst.downspeed);
        cJSON_AddNumberToObject(router_state_root, "memoryload", rst.memoryload);
        cJSON_AddNumberToObject(router_state_root, "upspeed", rst.upspeed);
        cJSON_AddNumberToObject(router_state_root, "useablespace", rst.useablespace);
        cJSON_AddStringToObject(router_state_root, "sn", rst.sn);
        cJSON_AddItemToObject(root, "routerstate", router_state_root);
    }else{
        res = -1;
        LOG("function: %s  [server] cJSON_CreateObject failed!!!\n", __func__);
    }
    return res;
}

int32_t add_wifi_info_to_json(cJSON *root, int32_t isknown, wireless_table *routerwifi, int32_t wifinum)
{
    LOG("[server] %s=====isknown=%d=== \n", __func__, isknown);
    int32_t res = 0;
    int32_t ret = 0;
    wireless_table *wt = NULL;
    int32_t wnum = 0;
    if(isknown){
        wnum = wifinum;
        wt = routerwifi;
    }else{
        ret = get_wireless_table_info(&wt, &wnum);
        res = ret ? -1:res;
        if(res != 0){
            LOG( "[ssst]===function:%s===call get_wireless_table_value failed!!!===res=%d===", __func__, res);
            return res;
        }
    }
    cJSON *root_wt = cJSON_CreateArray();
    int32_t i = 0;
    if(root_wt){
        for(i=0; i<wnum; i++){
			if(wt[i].enable == 1){
				cJSON *root_wt_each = cJSON_CreateObject();
				if(root_wt_each){
					cJSON_AddStringToObject(root_wt_each, "sn", wt[i].sn);
					cJSON_AddNumberToObject(root_wt_each, "signal", wt[i].signal);
					cJSON_AddNumberToObject(root_wt_each, "enable", wt[i].enable);
					cJSON_AddNumberToObject(root_wt_each, "channel", wt[i].channel);
					cJSON_AddStringToObject(root_wt_each, "band", wt[i].band);
					cJSON_AddStringToObject(root_wt_each, "ifname", wt[i].ifname);
					cJSON_AddStringToObject(root_wt_each, "mac", wt[i].mac);
					cJSON_AddStringToObject(root_wt_each, "encryption", wt[i].encryption);
					cJSON_AddStringToObject(root_wt_each, "password", wt[i].password);
					cJSON_AddStringToObject(root_wt_each, "ssid", wt[i].ssid);
					cJSON_AddItemToArray(root_wt, root_wt_each);
				}else{
					LOG( "function:%s create root_wt_each object failed!!!", __func__);
					res = -1;
					break;
				}
			}
        }
        cJSON_AddItemToObject(root, "routerwifi", root_wt);
    }else{
        LOG( "function:%s create root_wt object failed!!!", __func__);
        res = -1;
    }
    if(wt){ free(wt);}
    return res;
}

int32_t add_devices_info_to_json(cJSON *root, int32_t isknown, device_table *routerdevice, int32_t devicenum)
{
    LOG("[server] %s=====isknown=%d=== \n", __func__, isknown);
    int32_t res = 0;
    int32_t ret = 0;
    cJSON *all_device_root = NULL;
    cJSON *device_root = NULL;
    cJSON *device_speed_root = NULL;
    cJSON *device_authority_root = NULL;
    device_table *dst = NULL;
//    char device_root_name[32];                                                    //-----
    int32_t dnum = 0;
    int32_t i = 0;
    if(isknown){
        dst = routerdevice;
        dnum = devicenum;
    }else{
        ret = get_devices_info(&dst, &dnum);
        res = ret ? -1:res;
        if(res != 0){
            LOG( "[ssst]===function:%s===call get_device_info failed!!!===res=%d===", __func__, res);
            return res;
        }
    }
    LOG( "\n\n\n\n\n\n\n=-=-=-=-dnum == %d=-=-=-=-=-=-=-\n\n\n\n\n\n\n", dnum);
    all_device_root = cJSON_CreateArray();                                          //++++++
    if(all_device_root && dst){
        for(i=0; i<dnum; i++)
        {
            device_speed_root = cJSON_CreateObject();
            if(device_speed_root){
                cJSON_AddNumberToObject(device_speed_root, "maxdownloadspeed", dst[i].speed.maxdownloadspeed);
                cJSON_AddNumberToObject(device_speed_root, "uploadtotal", dst[i].speed.uploadtotal);
                cJSON_AddNumberToObject(device_speed_root, "upspeed", dst[i].speed.upspeed);
                cJSON_AddNumberToObject(device_speed_root, "downspeed", dst[i].speed.downspeed);
                cJSON_AddNumberToObject(device_speed_root, "maxuploadspeed", dst[i].speed.maxuploadspeed);
                cJSON_AddNumberToObject(device_speed_root, "downloadtotal", dst[i].speed.downloadtotal);
                cJSON_AddNumberToObject(device_speed_root, "online", dst[i].speed.online);
            }else{
                LOG( "[ssst]===function:%s===device_speed_root cJSON create failed!!!===", __func__);
                res = -1;
                break;
            }

            device_authority_root = cJSON_CreateObject();
            if(device_authority_root){
                cJSON_AddNumberToObject(device_authority_root, "internet", dst[i].authority.internet);
                cJSON_AddNumberToObject(device_authority_root, "lan", dst[i].authority.lan);
                cJSON_AddNumberToObject(device_authority_root, "notify", dst[i].authority.notify);
                cJSON_AddNumberToObject(device_authority_root, "speedlvl", dst[i].authority.speedlvl);
                cJSON_AddNumberToObject(device_authority_root, "disk", dst[i].authority.disk);
                cJSON_AddNumberToObject(device_authority_root, "limitup", dst[i].authority.limitup);
                cJSON_AddNumberToObject(device_authority_root, "limitdown", dst[i].authority.limitdown);
            }else{
                LOG( "[ssst]===function:%s===device_authority_root cJSON create failed!!!===", __func__);
                res = -1;
                break;
            }

            device_root = cJSON_CreateObject();
            if(device_root){
                cJSON_AddItemToObject(device_root, "speed", device_speed_root);
                cJSON_AddItemToObject(device_root, "authority", device_authority_root);
                cJSON_AddStringToObject(device_root, "sn", dst[i].sn);
                cJSON_AddStringToObject(device_root, "hostname", dst[i].originname);
                cJSON_AddStringToObject(device_root, "nickname", dst[i].hostname);
                cJSON_AddStringToObject(device_root, "ip", dst[i].ip);
                cJSON_AddStringToObject(device_root, "dev", dst[i].dev);
                cJSON_AddStringToObject(device_root, "icon", dst[i].icon);
                cJSON_AddStringToObject(device_root, "mac", dst[i].mac);
                cJSON_AddNumberToObject(device_root, "port", dst[i].port);
                cJSON_AddNumberToObject(device_root, "online", dst[i].online);
            }else{
                LOG( "[ssst]===function:%s===device_root cJSON create failed!!!===", __func__);
                res = -1;
                break;
            }

            cJSON_AddItemToArray(all_device_root, device_root);                      //++++++
        }
        free(dst);
    }else{
        res = -1;
        LOG( "[ssst]===function:%s===cJSON_CreateObject failed!!!===all_device_root=%p,dst=%p", __func__, all_device_root, dst);
    }
    if(all_device_root){
        cJSON_AddItemToObject(root, "deviceinfo", all_device_root);
    }else{
        res = -1;
        LOG("function: %s  [server] cJSON_CreateObject failed!!!\n", __func__);
    }
    return res;
}

/******************************************************
 *function: getOnlineDeviceNumberFromUci
 *brief: get online device number from uci
 *param: none
 *return:online device number
 ******************************************************/
int32_t getOnlineDeviceNumberFromUci()
{
	struct uci_context *ctx = uci_alloc_context();
	struct uci_package *pkg = NULL;
	struct uci_element *e;
	int32_t count = 0;

	uci_set_confdir(ctx, "/etc/config");
	if (UCI_OK == uci_load(ctx, "devlist", &pkg)){
		uci_foreach_element(&pkg->sections, e){
			struct uci_section *s = uci_to_section(e);
			if (atoi(uci_lookup_option_string(ctx, s, "online")) == 1)
			  count++;
		}
		LOG( "%s, online device number is %d",__func__, count);
		uci_unload(ctx,pkg);
	}

	uci_free_context(ctx);
	return count;
}

/******************************************************
 *function: getOnlineDeviceIpFromUci
 *brief: get online device ip list from uci
 *param: ip_list --- a point to an point array
 *return:none
 ******************************************************/
void getOnlineDeviceIpFromUci(char *ip_list[])
{
	struct uci_context *ctx = uci_alloc_context();
	struct uci_package *pkg = NULL;
	struct uci_element *e;
	int32_t count = 0;
	const char *ipstr = NULL;

	uci_set_confdir(ctx, "/etc/config");
	if (UCI_OK == uci_load(ctx, "devlist", &pkg)){
		uci_foreach_element(&pkg->sections, e){
			struct uci_section *s = uci_to_section(e);
			if (NULL != (ipstr = uci_lookup_option_string(ctx, s, "ip"))){
				/*for someone set an example in devlist:offline device with a fake ip*/
				if (atoi(uci_lookup_option_string(ctx, s, "online")) == 1){
				//if(strcmp(ipstr, "0.0.0.0")){
					strncpy(ip_list[count], ipstr, 16);
					LOG( "%s, add devlist ip to ip_list %s",__func__, ip_list[count]);
					count++;
				}
			}
		}
		uci_unload(ctx,pkg);
	}

	uci_free_context(ctx);
}

int32_t updateDevlistUci(devinfo *device, int newdev)
{

//    GetDevLowInfo(device);

    char *device_macaddr = NULL;
    char *device_ip = NULL;
    char *device_online = NULL;
    char *is_wireless = NULL;
    char *device_interface = NULL;
    char *device_hostname = NULL;

    char device_associate_time[32] = "";
    sprintf(device_associate_time, "%ld", device->associate_time);


    DB_LOG( "========device_associate_time===%s===", device_associate_time);

    device_online = device->online;
	is_wireless   = device->is_wireless;
	if(strcmp(device_online, "1") == 0){
		strlower(device->macaddr); // GetDevName use mac format like aa:bb:cc:dd:ee:ff
		GetDevName(device);
		strrpl(device->macaddr, ':', '_');//below function use mac format like AA_BB_CC_DD_EE_FF
		strupper(device->macaddr);
		GetDevNotify(device);
	}else{
		strrpl(device->macaddr, ':', '_');
	}
    device_macaddr = device->macaddr;

	device_ip = "0.0.0.0";
    device_hostname = device->hostname;
    device_interface = device->interface;

    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *pkg = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, WLDEVLIST, &pkg) == UCI_OK)
    {
        struct uci_section *section;
        struct uci_ptr ptr;
        memset(&ptr, 0, sizeof(struct uci_ptr));
        char *sectionType = "device";
        char *configName = WLDEVLIST;
        char *sectionName = device_macaddr;
        ptr.value = sectionType;
        ptr.package = configName;
        ptr.section = sectionName;
        ptr.option = NULL;
        uci_set(ctx, &ptr);

        section = uci_lookup_section(ctx, pkg, sectionName);

        memset(&ptr, 0, sizeof(struct uci_ptr));

        ptr.p = pkg;
        ptr.s = section;


		if(strcmp(device_online, "1") == 0){
			if(newdev){
				uciSetValue(ctx, &ptr, "mac", device_macaddr);
				uciSetValue(ctx, &ptr, "internet", device->internet);
				uciSetValue(ctx, &ptr, "lan", device->lan);
				uciSetValue(ctx, &ptr, "port", "1");
				uciSetValue(ctx, &ptr, "ip", device_ip);
				uciSetValue(ctx, &ptr, "warn", "0");
			}
			uciSetValue(ctx, &ptr, "online", device_online);
			uciSetValue(ctx, &ptr, "is_wireless", is_wireless);
			uciSetValue(ctx, &ptr, "dev", device_interface);
			if(device_hostname[0]){
				uciSetValue(ctx, &ptr, "hostname", device_hostname);
				uciSetValue(ctx, &ptr, "push_flag", "1"); // not need arp push
			}else{
				if(newdev){
					uciSetValue(ctx, &ptr, "push_flag", "2"); // new device notify push
				}else{
					uciSetValue(ctx, &ptr, "push_flag", "0"); // need arp push
				}
			}
			uciSetValue(ctx, &ptr, "associate_time", device_associate_time);
		}else{
			uciSetValue(ctx, &ptr, "online", device_online);
		}
        uci_save(ctx, pkg);
        uci_commit(ctx, &pkg, 0);

        uci_unload(ctx,pkg);
        ret = 0;
    }
    uci_free_context(ctx);
    return ret;
}

int32_t getPushSetting(Pushinfo *push_setting)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, "notify", &p) == UCI_OK)
    {
        struct uci_section *push_section = uci_lookup_section(ctx, p, "setting");
        //lookup values
        if(push_section != NULL){
            push_setting->enable = atoi(uci_lookup_option_string(ctx, push_section, "enable"));
            push_setting->mode = atoi(uci_lookup_option_string(ctx, push_section, "mode"));
            ret = 0;
        }
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

int32_t getKerHealthMode(void)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, "basic_setting", &p) == UCI_OK)
    {
        struct uci_section *kerHealth = uci_lookup_section(ctx, p, "kerHealth");
        //lookup values
        if(kerHealth != NULL){
            ret = atoi(uci_lookup_option_string(ctx, kerHealth, "enable"));
        }
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

int32_t getCurTime(struct tm *t)
{
    time_t timep;
    struct tm *local_t = NULL;
    time(&timep);
    local_t = localtime(&timep);
    memcpy(t, local_t, sizeof(struct tm));

    return 0;
}

int32_t push(my_msg_list *plist, my_message *msg)
{
    LOG("[server]push\n");
    if(!plist || !msg){
        LOG( "[server]null params passed in\n");
        return -1;
    }
    if(plist->head == NULL){
        LOG( "[server]empty message list\n");
        plist->tail = msg;
        plist->head = plist->tail;
        plist->size = 1;
        return 1;
    }
    plist->tail->next = msg;
    msg->prev = plist->tail;
    plist->tail = msg;
    plist->size += 1;
    return plist->size;
}

my_message *pop(my_msg_list *plist)
{
    LOG("[server]pop\n");
    if(!plist || plist->size == 0){
        LOG("[server]invalid plist or empty list\n");
        return NULL;
    }
    my_message *tmp;
    if(plist->size == 1){
        tmp = plist->head;
        plist->head = NULL;
        plist->tail = NULL;
        plist->size = 0;
        return tmp;
    }
    tmp = plist->head;
    plist->head = tmp->next;
    plist->head->prev = NULL;
    plist->size -= 1;
    return tmp;
}

/****************************************/
//function: strrpl
//brief: use dpl to replace spl in str
//param: str-----the source string
//       spl-----the char to be replaced
//       dpl-----the char used to replace spl
//return: void
/****************************************/
void strrpl(char *str, char spl, char dpl)
{
    int i = 0;
    for(i = 0; i<strlen(str); i++)
    {
        if(str[i] == spl)
        {
            str[i] = dpl;
        }
    }
}

void strupper(char *str)
{
    int i = 0;
    do
    {
        if(str[i]>='a' && str[i]<='z')
        {
            str[i] = str[i] - 0x20;
        }
    }while(str[i++] != '\0');
}

void strlower(char *str)
{
    int i = 0;
    do
    {
        if(str[i]>='A' && str[i]<='Z')
        {
            str[i] = str[i] + 0x20;
        }
    }while(str[i++] != '\0');
}

int detect_process(char * process_name)
{
    FILE *ptr = NULL;
    char buff[512];
    char ps[128];
    sprintf(ps,"ps | grep -c %s",process_name);
    strcpy(buff,"ABNORMAL");
    if((ptr=popen(ps, "r")) != NULL)
    {
		LOG("go here -=%s\n",buff);
        if(fgets(buff, 512, ptr) != NULL)
        {
            LOG("buffer--=%s\n",buff);
            if(atoi(buff) > 2)
            {
                pclose(ptr);
                return 0;
            }
        }
		else {
			LOG("ssst get popen result noting  buffer--==============%s\n",buff);
		}
    }
	else{
		LOG("ssst try popen directly  fail buffer--==============%s\n",buff);
		return -1;
	}

    pclose(ptr);
    return -1;
}

#define ETH_ALEN 6
static void inc_sf_mac_addr(char *mac, int inc)
{
	unsigned int mac_calc[ETH_ALEN] = {0};
	int i = 0;
	for(i = 0 ; i < ETH_ALEN ; i++)
	{
		mac_calc[i] = (unsigned int)(mac[i]) & 0xFF;
	}
	mac_calc[ETH_ALEN - 1] += inc;
	mac_calc[ETH_ALEN - 2] += ((mac_calc[ETH_ALEN - 1] & (0x100))  >> 8);
	mac[ETH_ALEN - 1] = mac_calc[ETH_ALEN - 1] & (0xff);

	mac_calc[ETH_ALEN - 3] += ((mac_calc[ETH_ALEN - 2] & (0x100))  >> 8);
	mac[ETH_ALEN - 2] = mac_calc[ETH_ALEN - 2] & (0xff);
	mac[ETH_ALEN - 3] = mac_calc[ETH_ALEN - 3] & (0xff);
	//the first 3 char is reserved
}

/*void get_router_macaddr(char *mac_in)
{
	if(sf_factory_read_operation(READ_FIRST_MAC_ADDRESS, mac_in, ETH_ALEN)){
		mtd_operation(CMD_READ, mac_in, 6, 0);
	}
	inc_sf_mac_addr(&mac_in[0],2);
}*/
