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
#include "status_sync.h"
#include "ssst_request.h"
#include "cJSON.h"
#include "utils.h"
#include "token.h"
#include "local_http.h"
#include "mtd.h"
#include "sf_factory_read.h"

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

void get_router_feature(cJSON **ppjson)
{
	int32_t ret = 0;
	char postData[36] = "{\"version\":\"V18\"}";
	struct HttpResonseData response;
	response.size = 0;
	cJSON *json_code = NULL;
	ret = postDataToHttpdCommon(LOCAL_COMMAND_GET_ROUTER_FEATURE,postData,&response);
	LOG("get response----ret=%d---data: %s\n",ret,(response.size > 0) ? (char*)(response.data) : "NULL");
	if(ret == CURLE_OK && response.size > 0){
		//parse http response data
		*ppjson = cJSON_Parse(response.data);
		json_code = cJSON_GetObjectItem(*ppjson, "code");
		if (json_code) {
			ret = json_code->valueint;
			if(ret == 0)
			{
				cJSON_DeleteItemFromObject(*ppjson, "code");
				cJSON_DeleteItemFromObject(*ppjson, "msg");
			}
		}
	}
	if(response.size > 0) free(response.data);
	LOG( "[ssst]===function:%s===after getrouterfeature ret=%d===", __func__, ret);
}

int32_t checkRouterDataFromServer(char* sn_str,char *mac, char * bindid,char* managerid,char* out_sn, char* product_key)
{
	char idtmp[ID_LEN + 1], url[256] = "";
	char* json_str = NULL, *res_sn = NULL, *obj_tmp = NULL;
	void* data = NULL;
	int i = 0, res_code, j = 0;
	int32_t ret3 = -1, try_count = 0, ret = 0;
	FILE *mfile;
	struct HttpResonseData response;
	struct cJSON *obj;
	LOG("function:%s sn=%s mac=%s\n", __func__, sn_str,mac);
	LOG(" bindid=%s manager list=%s\n", bindid, managerid);
	cJSON *json_res = NULL, *json_feat = NULL, *json = NULL, *midlist = NULL;
    cJSON *object_data = NULL, *object_code = NULL, *object_sn = NULL;
	get_router_feature(&json_feat);
	//prepare message
	json = cJSON_CreateObject();
	cJSON_AddStringToObject(json,"sn",sn_str);
	cJSON_AddStringToObject(json,"mac",mac);
	cJSON_AddStringToObject(json,"bindid",bindid);
	cJSON_AddStringToObject(json,"routerkey",product_key);
	if(json_feat)
	{
		cJSON_AddItemToObject(json,"feature",json_feat);
	}
	midlist = cJSON_CreateArray();
	if(managerid[0] != '\0'){
		idtmp[ID_LEN] = '\0';
		while(managerid[i] != '\0' && managerid[i] != '\n' && i < ID_BUF_S - ID_LEN){
			strncpy(idtmp, managerid+ i, ID_LEN);
			cJSON_AddItemToArray(midlist, cJSON_CreateString((const char *)idtmp));
			i = i + ID_LEN;
		}
	}
	cJSON_AddItemToObject(json,"mgrlist",midlist);
	LOG("prepare data ok\n");
	json_str = cJSON_Print(json);
	//copy json str out
	LOG("strlen(json_str) ======= %d",strlen(json_str));
	data = malloc(strlen(json_str) + 256);
	sprintf((char*)data,"%s",json_str);
	if(json_str) free(json_str);
	if(json) cJSON_Delete(json);
	//use remote function to handle this message
	sprintf(url,"%s/%s",XCLOUD_REMOTE_ADDR,XCLOUD_REMOTE_FUNCTION_ROUTER_DATA_SYNC);
	response.size = 0;
	LOG("publish event--data=%s\n",(char*)data);
	for(try_count = 0; try_count<3; try_count++){
		ret3 = postHttpCommon(url,data,&response,1,0);
		LOG("publish event---try_time= %d\n", try_count);
		LOG("publish event---ret= %d --response=%s\n",ret3,(response.size > 0) ? (char*)response.data : "");
		if(ret3 == CURLE_OK){
			break;
		}
	}
	free(data);
	if(ret3 != CURLE_OK || response.size == 0){
		LOG("publish router data sync  event fail ret= %d\n",ret3);
		if(response.size > 0) free(response.data);
		return -1;
	}
	//process data

	LOG("respnse data %s \n",(char *)response.data);
	if(response.size > 0){
		json_res = cJSON_Parse(response.data);
		if(!json_res) goto jsonerror;
		object_code = cJSON_GetObjectItem(json_res,"code");
		if(!object_code) goto jsonerror;

		res_code = object_code->valueint;

		LOG("get code in res here %d\n",res_code);
		switch(res_code){
			case DATA_SYNC_PARSE_PARAM_ERROR:
			case DATA_SYNC_ALREADY_HAVE_MAC :
			case DATA_SYNC_FAIL_CREATE_SN   :
				LOG(" return fail %d\n", res_code);
				goto jsonerror;
				break;
			case DATA_SYNC_SUCCESS		  :
				LOG("success data sync\n");
				break;
			case DATA_SYNC_NO_SN_MAC	:
				{
					object_data = cJSON_GetObjectItem(json_res,"data");
					if(!object_data) goto jsonerror;

					object_sn = cJSON_GetObjectItem(object_data,"sn");
					if(!object_sn) goto jsonerror;

					res_sn = (char*)(object_sn->valuestring);
					LOG("get server string %s\n",res_sn);
					strncpy(out_sn, res_sn, SN_STR_LEN - 1);
					//if(res_sn) free(res_sn);
					ret = 1;
					break;
				}
			case DATA_SYNC_MGR_LIST_SYNC:
				{
					object_data = cJSON_GetObjectItem(json_res,"data");
					if(!object_data) goto jsonerror;

					for(;j < cJSON_GetArraySize(object_data ); j++) {
						obj = cJSON_GetArrayItem(object_data , j);
						obj_tmp = (char*)(obj->valuestring);
						strncpy(managerid + j*ID_LEN, obj_tmp, ID_LEN);
					//	if(obj_tmp) free(obj_tmp);
					}
					managerid[j*ID_LEN] = '\0';
					mfile = fopen(MANAGERFILE,"w");
					if (mfile){
						fputs(managerid, mfile);
						fputc('\0', mfile);
						fclose(mfile);
					}else LOG("open manager id file fail\n");
					ret = 2;
					break;
				}
			case DATA_SYNC_SERVER_UNBIND :
			case DATA_SYNC_BIND_DIFERENT :
				{
					LOG("start unbind router\n");
					ret = setRouterValueToUci("cloudrouter","bind","0");
					if(ret){
						LOG("CheckBindMgr: unbind fail\n");
					}else{
						setRouterValueToUci("cloudrouter","binder","");
						system("/etc/init.d/subservice stop");
					}
					ret = 3;
					break;
				}
			default:
				goto jsonerror;
		}
		if(json_res) cJSON_Delete(json_res);
		if(response.size > 0) free(response.data);
	}
	return ret;
jsonerror:
	LOG("data sync fail\n");
	if(json_res) cJSON_Delete(json_res);
	if(response.size > 0) free(response.data);
	return -2;
}

int dailyHeart()
{
    char SN[SN_LEN] = "";
    char sn_str[SN_STR_LEN] = {'\0'}, url[256] = "";
    char *json_str = NULL;
    void *data = NULL;
    int ret = -1, try_count = 0, i = 0;

    cJSON *json=NULL;
    struct HttpResonseData response;

    // get sn
	if(sf_factory_read_operation(READ_SN, SN, sizeof(SN)))
		mtd_operation(CMD_READ, SN, sizeof(SN), SN_OFFSET);

    for(i = 0; i < SN_LEN; i++){
		sprintf(sn_str + i*2,"%02x",SN[i]);
	}
	LOG("sn str : %s\n",sn_str);

    json = cJSON_CreateObject();
	cJSON_AddStringToObject(json,"sn",sn_str);

    json_str = cJSON_Print(json);
	//copy json str out
	LOG("strlen(json_str) ======= %d",strlen(json_str));
	data = malloc(strlen(json_str) + 256);
	sprintf((char*)data,"%s",json_str);
	if(json_str) free(json_str);
	if(json) cJSON_Delete(json);

    //use remote function to handle this message
	sprintf(url,"%s/%s",XCLOUD_REMOTE_ADDR,XCLOUD_REMOTE_FUNCTION_DAILY_HEART_BEAT);
	response.size = 0;
	LOG("dailyHeart--data=%s\n",(char*)data);
	for(try_count = 0; try_count<3; try_count++){
		ret = postHttpCommon(url,data,&response,1,0);
		LOG("dailyHeart---try_time= %d\n", try_count);
		LOG("dailyHeart---ret= %d --response=%s\n",ret,(response.size > 0) ? (char*)response.data : "");
		if(ret == CURLE_OK){
			break;
		}
	}
	free(data);
	if(ret != CURLE_OK || response.size == 0){
		LOG("daily Heart beat to server fail ret= %d\n",ret);
		if(response.size > 0) free(response.data);
		return -1;
	}

    LOG("respnse data %s \n",(char *)response.data);
    if(response.size > 0) free(response.data);
    return ret;
}

char *get_file_name(char *path, char *flag)
{
    FILE *fstream = NULL;
    char cmd[256] = {0};
    char *filename = NULL;
    if(strcmp(flag, "0x1") == 0){
        sprintf(cmd, "mkdir /tmp/senderr ; dmesg > /tmp/senderr/dmesg.txt ; mv %s /tmp/senderr ; tar -zcvf /tmp/senderr/data.tar.gz -C /tmp/senderr/ .", path);
        LOG("cmd is :%s\n",cmd);
        fstream = popen(cmd, "r");
        pclose(fstream);
        filename = "/tmp/senderr/data.tar.gz";
    }
    if(strcmp(flag, "0x2") == 0){
        sprintf(cmd, "mkdir /tmp/senderr ; top -n 1 > /tmp/senderr/top.txt ; cat /proc/meminfo > /tmp/senderr/mem.txt mv %s /tmp/senderr ; tar -zcvf /tmp/senderr/data.tar.gz -C /tmp/senderr/ .", path);
        fstream = popen(cmd, "r");
        pclose(fstream);
        filename = "/tmp/senderr/data.tar.gz";
    }
    if(strcmp(flag, "0x4") == 0){
        sprintf(cmd, "mkdir /tmp/senderr ; mv /etc/config/* /tmp/senderr/ ; mv %s /tmp/senderr ; tar -zcvf /tmp/senderr/data.tar.gz -C /tmp/senderr/ .", path);
        fstream = popen(cmd, "r");
        pclose(fstream);
        filename = "/tmp/senderr/data.tar.gz";
    }
    if(strcmp(flag, "0x8") == 0){
        sprintf(cmd, "mkdir /tmp/senderr ; cat /sys/devices/platform/factory-read/hw_ver > /tmp/senderr/hw.txt ; mv %s /tmp/senderr ; tar -zcvf /tmp/senderr/data.tar.gz -C /tmp/senderr/ .", path);
        fstream = popen(cmd, "r");
        pclose(fstream);
        filename = "/tmp/senderr/data.tar.gz";
    }
    return filename;
}

int kernelLogToServer(char *type, char *text, char *path, char *module, char *code, char *flag)
{
    char SN[SN_LEN] = "";
    char sn_str[SN_STR_LEN] = {'\0'}, url[256] = "", timestamp[11] = "", cmd[64] = {0};
    int ret = -1, i = 0;
    char *filename = NULL;
    FILE *fstream = NULL;
    time_t t;
    struct HttpResonseData response;
    // get sn
	if(sf_factory_read_operation(READ_SN, SN, sizeof(SN)))
		mtd_operation(CMD_READ, SN, sizeof(SN), SN_OFFSET);
	for(i = 0; i < SN_LEN; i++){
		sprintf(sn_str + i*2,"%02x",SN[i]);
	}
	LOG("sn str : %s\n",sn_str);

    //get timestamp
    t = time(NULL);
    sprintf(timestamp, "%ld", time(&t));

    //get filename
    if(flag != NULL){
        filename = get_file_name(path, flag);
    }
    else{
        filename = path;
    }

    sprintf(url,"%s/%s",XCLOUD_REMOTE_ADDR,XCLOUD_REMOTE_FUNCTION_LOG_UPLOAD);
	response.size = 0;
    ret = postHttpFile(url, filename, sn_str, text, module, code, timestamp, &response);
    LOG("url is %s\n",url);
    if (ret != 0)
    {
        LOG("send ker log to server fail\n");
    }
	LOG("respnse data %s \n",(char *)response.data);
    if(response.size > 0) free(response.data);
    
    sprintf(cmd, "rm -rf /tmp/senderr");
    fstream = popen(cmd, "r");
    return ret;
}

void dailyData()
{
    FILE *fstream = NULL;
    char cmd[64] = {0};
    int ret;

    system("/bin/daily.sh");
    ret = kernelLogToServer("DAILY","router daily data","/tmp/dailyData.txt","0","0",NULL);
    if(ret != 0){
        LOG("daily data send fail\n");
    }
    // rm /tmp/dailyData.txt
    sprintf(cmd, "rm /tmp/dailyData.txt");
    fstream = popen(cmd, "r");
    pclose(fstream);
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

extern struct user_info_list g_list;
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
