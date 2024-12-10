/*
   on
 * =====================================================================================
 *
 *       Filename:  publish.c
 *
 *    Description:  implement for router event publish
 *
 *        Version:  1.0
 *        Created:  2015年08月31日 19时48分49秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert , robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#include "publish.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "local_storage.h"
#include "ssst_request.h"
#include "cJSON.h"
#include "utils.h"
#include "local_http.h"

#define SYSTEM_EVENT_ACTION_REMOTE_SERVER_CONNECTED 0
#define SYSTEM_EVENT_ACTION_UPGRADE 1
#define SYSTEM_EVENT_ACTION_REBOOT 2
#define SYSTEM_EVENT_ACTION_RESET 3

#define SN_LENGTH 50
#define ID_LEN 18
extern char slversion[64];

void prepareStatusChangeData(int32_t action,char *out)
{
	char *tmp_out = NULL;
	cJSON *root = NULL;
	router_table router;
	char romtime[32];
	memset(&router, 0, sizeof(router_table));
	memset(romtime, 0, 32);

	switch(action){
		case SYSTEM_EVENT_ACTION_REMOTE_SERVER_CONNECTED:
			root = cJSON_CreateObject();
			cJSON_AddNumberToObject(root, "action", action);
			tmp_out = cJSON_PrintUnformatted(root);
			strcpy(out, tmp_out);
			free(tmp_out);
			cJSON_Delete(root);
			break;
		default:
			root = cJSON_CreateObject();
			cJSON_AddNumberToObject(root, "action", action);
			tmp_out = cJSON_PrintUnformatted(root);
			strcpy(out, tmp_out);
			free(tmp_out);
			cJSON_Delete(root);
			break;
	}
}

void prepareOnlineChangeData(int32_t strange,char *name,char *mac,char *out)
{
	char ssid[32] = {0};
	char *json_tmp = NULL;
	cJSON *json = NULL;
	json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "strange", strange);
	cJSON_AddStringToObject(json, "name", name);
	cJSON_AddStringToObject(json, "mac", mac);
	if(get_5G_ssid(ssid) == 0){
		cJSON_AddStringToObject(json, "ssid5G", ssid);
	}
	json_tmp = cJSON_Print(json);
	sprintf(out,"%s",json_tmp);
	if(json) cJSON_Delete(json);
	if(json_tmp) free(json_tmp);
}

void prepareStorageChangeData(int32_t status, char *out)
{
	char * json_tmp = NULL;
	cJSON *json = NULL;
	json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "action", status);
	json_tmp = cJSON_Print(json);
	sprintf(out,"%s",json_tmp);
	if(json) cJSON_Delete(json);
	if(json_tmp) free(json_tmp);
}

struct user_info_list g_list;
int32_t publishEventToUser(int32_t type,char *eventData)
{
	LOG("function:%s eventData=%s\n", __func__, eventData);

	struct HttpResonseData response;
	int32_t ret3 = -1, try_count = 0;
	cJSON *json = NULL;
	char *json_str = NULL;
	void *data = NULL;
	char sn[SN_LENGTH], url[256] = "";
	memset(sn, 0, SN_LENGTH);
	if(getSfHardwareConfig("sn",sn) != 0){
		LOG( "[server] get sn from hardware config fail!\n");
		return -1;
	}

	//prepare message
	json = cJSON_CreateObject();
	cJSON_AddStringToObject(json,"sn",sn);
	cJSON_AddNumberToObject(json,"type",type);
	cJSON_AddStringToObject(json,"event",eventData);
	cJSON_AddStringToObject(json,"version",slversion);
	json_str = cJSON_Print(json);
	//copy json str out
	LOG("strlen(json_str) ======= %d",strlen(json_str));
	data = malloc(strlen(json_str) + 256);
	sprintf((char*)data,"%s",json_str);
	if(json) cJSON_Delete(json);
	if(json_str) free(json_str);
	//post message to remote app user
	user_info *tmp = g_list.head;
	while(tmp != NULL){
		sprintf(url,"http://%s:%d/%s",tmp->ip,tmp->port,XCLOUD_REMOTE_FUNCTION_PUBLISH_EVENT);
		LOG("%s APP user address for post message to: %s",__func__,url);
		response.size = 0;
		LOG("publish event--data=%s\n",(char*)data);
		for(try_count = 0; try_count<3; try_count++){
			ret3 = postHttps(url,data,&response);
			LOG("publish event---try_time= %d\n", try_count);
			LOG("publish event---ret= %d --response=%s\n",ret3,(response.size > 0) ? (char*)response.data : "");
			if(ret3 == CURLE_OK){
				break;
			}
		}
		if(ret3 != CURLE_OK || response.size == 0){
			LOG("publish event fail ret= %d\n",ret3);
		}
		if(response.size > 0) free(response.data);
		tmp = tmp->next;
	}
	if(data) free(data);
	return 0;
}

int32_t publishRouterEvent(int32_t type,char *eventData)
{
	LOG("function:%s eventData=%s\n", __func__, eventData);

	struct HttpResonseData response;
	cJSON *json = NULL;
	int32_t ret3 = -1, try_count = 0;
	char *json_str = NULL;
	void *data = NULL;
	char sn[SN_LENGTH], url[256] = "";
	memset(sn, 0, SN_LENGTH);
	if(getSfHardwareConfig("sn",sn) != 0){
		LOG( "[server] get sn from hardware config fail!\n");
		return -1;
	}

	if (has_binded()){
		LOG( "[server] %s: router has not binded, do not publish event!\n", __func__);
		return -1;
	}

	//prepare message
	json = cJSON_CreateObject();
	cJSON_AddStringToObject(json,"sn",sn);
	cJSON_AddNumberToObject(json,"type",type);
	cJSON_AddStringToObject(json,"event",eventData);
	cJSON_AddStringToObject(json,"version",slversion);
	json_str = cJSON_Print(json);
	//copy json str out
	LOG("strlen(json_str) ======= %d",strlen(json_str));
	data = malloc(strlen(json_str) + 256);
	sprintf((char*)data,"%s",json_str);
	if(json) cJSON_Delete(json);
	if(json_str) free(json_str);
	//use remote function to handle this message
	sprintf(url,"%s/%s",XCLOUD_REMOTE_FUNCTION_ADDR,XCLOUD_REMOTE_FUNCTION_PUBLISH_EVENT);
	response.size = 0;
	LOG("publish event--data=%s\n",(char*)data);
	for(try_count = 0; try_count<3; try_count++){
		ret3 = postHttps(url,data,&response);
		LOG("publish event---try_time= %d\n", try_count);
		LOG("publish event---ret= %d --response=%s\n",ret3,(response.size > 0) ? (char*)response.data : "");
		if(ret3 == CURLE_OK){
			break;
		}
	}
	if(data) free(data);
	if(ret3 != CURLE_OK || response.size == 0){
		LOG("publish event fail ret= %d\n",ret3);
		if(response.size > 0) free(response.data);
		return -1;
	}
	if(response.size > 0) free(response.data);
	return 0;
}

int32_t publishDeviceOnlineEvent(int32_t strange,char *name,char *mac)
{
	LOG("publishDeviceOnlineEvent--strange=%d name=%s mac=%s\n",strange,name,mac);
	char data[256];
	prepareOnlineChangeData(strange,name,mac,data);
	int32_t ret = publishRouterEvent(ROUTER_EVENT_TYPE_ONLINE,data);
	return ret;
}

int32_t publishRtStatusChangeEvent(int32_t action)
{
	LOG("publishRtStatusChangeEvent--action=%d \n",action);
	char data[4096];
	int ret = -1;
	prepareStatusChangeData(action,data);
	LOG("function:%s  data====%s", __func__, data);
	ret = publishRouterEvent(ROUTER_EVENT_TYPE_STATUS_CHANGE,data);
	return ret;
}
