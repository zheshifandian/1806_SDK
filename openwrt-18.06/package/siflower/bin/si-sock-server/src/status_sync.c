/*
 * =====================================================================================
 *
 *       Filename:  status_sync.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  07/27/2015 03:59:39 PM
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
#include <syslog.h>
#include <string.h>
#include <curl/curl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <pthread.h>

#include "queue.h"
#include "publish.h"
#include "cJSON.h"
#include "utils.h"
#include "ssst_request.h"
#include "http.h"
#include "bind.h"
#include "local_storage.h"
#include "status_sync.h"
#include "local_http.h"

#define DEBUGING 1

int32_t g_upgrade_process = 0;


extern char slversion[64];

struct StatusSyncContext g_sync_context;


void init_sync_context()
{
    g_sync_context.stateSubRequestQueue = createMsgQueue();
    g_sync_context.unresolvedEventQueue = createMsgQueue();
    g_sync_context.lastHeartbeatSendTime = 0;
    pthread_mutex_init(&g_sync_context.g_sync_mutex,NULL);
    pthread_cond_init(&g_sync_context.g_sync_cond,NULL);
}

void destroy_sync_context(){
    destoryMsgQueue(g_sync_context.stateSubRequestQueue);
    destoryMsgQueue(g_sync_context.unresolvedEventQueue);
    pthread_mutex_destroy(&g_sync_context.g_sync_mutex);
    pthread_cond_destroy(&g_sync_context.g_sync_cond);
}

void do_status_sync(char *data, char **callback)
{
    on_sub_event(data,callback);
}

#if DEBUGING
int32_t periodSendDevinfoEvent()
{
    UpdateLocalInfoToUciEvent *UpdateLocalDeviceEventMsg = NULL;
    fetchMsgByTagAndType(g_sync_context.unresolvedEventQueue, DEVICE_INFO_TAG, UPDATE_LOCAL_INFO_EVENT, (struct TagMessage **)(&UpdateLocalDeviceEventMsg));
    if(!UpdateLocalDeviceEventMsg){
        UpdateLocalDeviceEventMsg = (UpdateLocalInfoToUciEvent *)malloc(sizeof(UpdateLocalInfoToUciEvent));
        if(UpdateLocalDeviceEventMsg){
            UpdateLocalDeviceEventMsg->unresolvedEvent.tagMessage.type = UPDATE_LOCAL_INFO_EVENT;
            strcpy(UpdateLocalDeviceEventMsg->unresolvedEvent.tagMessage.tag, DEVICE_INFO_TAG);
            UpdateLocalDeviceEventMsg->unresolvedEvent.data = NULL;
            UpdateLocalDeviceEventMsg->unresolvedEvent.firstEnqueueTime = time(NULL);
            UpdateLocalDeviceEventMsg->lastUpdateTime = time(NULL);
            UpdateLocalDeviceEventMsg->enable = 1;
        }else{
            LOG( "[server][func--%s]===no enough memory to malloc===", __func__);
            return -1;
        }
    }
    sendMessage(g_sync_context.unresolvedEventQueue,(void*)UpdateLocalDeviceEventMsg);
    return 0;
}
#endif

void updateSubHeartbeat(char *user)
{
    struct MesageQueue *msgQueue = g_sync_context.stateSubRequestQueue;
    LOG("updateSubHeartbeat--user=%s sub-count=%d\n",user,msgQueue->count);
    if(msgQueue->count > 0){
        //update heartbeat of certain user
        pthread_mutex_lock(&msgQueue->lock);
        struct TagMessage* header = msgQueue->header;
        while(header){
            if(!strcmp(header->tag,user)){
                ((struct BaseSubRequest*)header)->hearbeatUpdateTime = time(NULL);
            };
            header = header->next;
        }
        pthread_mutex_unlock(&g_sync_context.stateSubRequestQueue->lock);
    }
}

#define SYNC_INTERVAL_SECONDS 10

int32_t handle_sub_event(char *user_json,char *error)
{
    LOG( "[ssst]===function:%s==", __func__);
    if(!user_json){
        LOG("handle sub event json NULL");
        return  -1;
    }
    int32_t action = 0;
    if(parseIntFromJson(SUB_EVENT_ACTION_STR,user_json,&action) != 0){
        sprintf(error,"%s","handle_sub_event--json parse error");
       // LOG(error);
        return -1;
    }
    if(action == 1){
        //heart beat event
        char user[16] = "";
        if(parseStringFromJson(SUB_EVENT_USERID,user_json,user) != 0){
            sprintf(error,"%s","handle_sub_event--json parse user error-");
         //   LOG(error);
            return -1;
        }else{
            updateSubHeartbeat(user);
            return 0;
        }
    }
    //parse data for sub request
    int32_t res = -1;
    int subType = 0;
    char userId[16] = "";
    char data[256] = "";
    if(parseIntFromJson(SUB_EVENT_TYPE_STR,user_json,&subType) != 0
            || parseStringFromJson(SUB_EVENT_USERID,user_json,userId) != 0
            || parseStringFromJson(SUB_EVENT_DATA,user_json,data) != 0)
    {
        sprintf(error,"%s","handle_sub_event--json parser error-\n");
       // LOG(error);
        return -1;
    }
    LOG( "[ssst]===function:%s===subType=%d===", __func__, subType);
    switch(subType){
        case SUB_EVENT_TYPE_ROUTERSTATE:
            {
                //parse user data
                int32_t action = 0;
                if(parseIntFromJson("action",data,&action) != 0){
                    sprintf(error,"%s","parse user router state sub request action fail");
                    //LOG(error);
                    res = -1;
                    break;
                }
                if(action == 0){
                    //sub request start
                    struct RouterStateSubRequest *rtRequest = NULL;
                    //query if we have stored one,if there,we fetch out and update it
                    if(fetchMsgByTagAndType(g_sync_context.stateSubRequestQueue,userId,SUB_EVENT_TYPE_ROUTERSTATE,(struct TagMessage **)(&rtRequest)) != 0){
                        rtRequest = (struct RouterStateSubRequest *)malloc(sizeof(RouterStateSubRequest));
                        LOG("handle sub---1----create new request--userid=%s",userId);
                    }
                    memset(rtRequest,0,sizeof(RouterStateSubRequest));
                    int32_t interest = 0;
                    sprintf(rtRequest->baseRequest.tagMessage.tag,"%s",userId);
                    rtRequest->baseRequest.tagMessage.type = SUB_EVENT_TYPE_ROUTERSTATE;
                    rtRequest->baseRequest.interest = (parseIntFromJson("interest",data,&interest) == 0) ? interest : -1;
                    rtRequest->baseRequest.hearbeatUpdateTime = time(NULL);
                    sendMessage(g_sync_context.stateSubRequestQueue,(void*)rtRequest);
                    res= 0;
                }else{
                    //sub request cancel
                    deleteMsgByTagAndType(g_sync_context.stateSubRequestQueue,userId,SUB_EVENT_TYPE_ROUTERSTATE);
                    res = 0;
                }
                break;
            }
        case SUB_EVENT_TYPE_DEVICESTATE:
            {
                int32_t action = 0;
                if(parseIntFromJson("action",data,&action) != 0){
                    sprintf(error,"%s","parse user device state sub request action fail");
                    res = -1;
                  //  LOG(error);
                    break;
                }
                if(action == 0){
                    //sub request start
                    struct DeviceStateSubRequest *rtRequest = NULL;
                    //query if we have stored one,if there,we fetch out and update it
                    if(fetchMsgByTagAndType(g_sync_context.stateSubRequestQueue,userId,SUB_EVENT_TYPE_DEVICESTATE,(struct TagMessage **)(&rtRequest)) != 0)
                        rtRequest = (struct DeviceStateSubRequest *)malloc(sizeof(DeviceStateSubRequest));
                    memset(rtRequest,0,sizeof(DeviceStateSubRequest));
                    int32_t interest = 0;
                    sprintf(rtRequest->baseRequest.tagMessage.tag,"%s",userId);
                    rtRequest->baseRequest.tagMessage.type = SUB_EVENT_TYPE_DEVICESTATE;
                    rtRequest->baseRequest.interest = (parseIntFromJson("interest",data,&interest) == 0) ? interest : -1;
                    parseStringFromJson("devicelist",data,rtRequest->devicelist);
                    rtRequest->baseRequest.hearbeatUpdateTime = time(NULL);
                    sendMessage(g_sync_context.stateSubRequestQueue,(void*)rtRequest);
                    res = 0;
                }else{
                    //sub request cancel
                    deleteMsgByTagAndType(g_sync_context.stateSubRequestQueue,userId,SUB_EVENT_TYPE_DEVICESTATE);
                    res = 0;
                }
                break;
            }
        default:
            {
                sprintf(error,"unknow sub type %d",subType);
                //LOG(error);
                return -1;
            }
    }
    pthread_mutex_lock(&g_sync_context.g_sync_mutex);
    pthread_cond_signal(&g_sync_context.g_sync_cond);
    pthread_mutex_unlock(&g_sync_context.g_sync_mutex);
    return res;
}

void *notifyUpgradeProcess(void *args) {
    int32_t startFlag = 1;
    char userid[32] = "";
    char usersubid[32] = "";
    char msgid_str[32] = "";
    int32_t msgid = 0;
	int ota_type = 0;
	int mode = 0;
	char postData[36] = "{\"version\":\"V17\",\"check\":1}";
	char post_ac_Data[50] = {'\0'};

    periodGetUpgradeProcessEvent_t *periodGetUpgradeProcessMsg = (periodGetUpgradeProcessEvent_t *)args;
    strcpy(userid, periodGetUpgradeProcessMsg->userid);
    strcpy(usersubid, periodGetUpgradeProcessMsg->usersubid);
    strcpy(msgid_str, periodGetUpgradeProcessMsg->msgid);
	ota_type = periodGetUpgradeProcessMsg->ota_type;
	mode  = periodGetUpgradeProcessMsg->mode;
	free(periodGetUpgradeProcessMsg);
	sprintf(post_ac_Data,"{\"version\":\"V17\",\"check\":1,\"mode\":%d}",mode);
    msgid = atoi(msgid_str);
    while(startFlag){
        struct HttpResonseData local_response;
        int ret = -1;
		if(ota_type == 0 )
		  ret = postDataToHttpdCommon(LOCAL_COMMAND_GET_UPGRADE_PROCESS,postData,&local_response);
		else
		  ret = postDataToHttpdCommon(LOCAL_COMMAND_GET_AC_UPGRADE_PROCESS,post_ac_Data,&local_response);

        LOG( "===========periodGetUpgradeProcessEvent======ret=%d",ret);
        LOG( "===========periodGetUpgradeProcessEvent======usersubid=%s",usersubid);
        if(local_response.size > 0){
            int downloaded = 0;
            int status = 0;
            char routerid[32] = "";
            char subid[32] = "";
            cJSON *tmp_parse = cJSON_Parse((char *)(local_response.data));
            cJSON *downloaded_obj = cJSON_GetObjectItem(tmp_parse, "downloaded");
            cJSON *status_obj = cJSON_GetObjectItem(tmp_parse, "status");

            cJSON_GetValueInt(downloaded_obj, &downloaded);
            cJSON_GetValueInt(status_obj, &status);
            if(downloaded>=100 || status == 3){
                g_upgrade_process = 0;
                startFlag = 0;
            }
            free(tmp_parse);

            getRouterValueUci("routerid", routerid);
            getRouterValueUci("subid", subid);
			cJSON *root = cJSON_CreateObject();
			cJSON_AddStringToObject(root,"method", "update");
			//add relation
			cJSON *relation = cJSON_CreateObject();
			cJSON_AddStringToObject(relation,"column", "msglist");
			cJSON_AddStringToObject(relation,"method", "insert");
			cJSON_AddItemToObject(root,"relation",relation);
			//add object
			cJSON *object = cJSON_CreateObject();
			cJSON_AddStringToObject(object,"objectId", usersubid);
			//add message list
			cJSON *msglist = cJSON_CreateArray();
			//add one message
			cJSON *json = cJSON_CreateObject();
			cJSON_AddStringToObject(json,"userid",userid);
			cJSON_AddStringToObject(json,"routerid",routerid);
			cJSON_AddStringToObject(json,"subid",subid);
			cJSON_AddNumberToObject(json,"msgid",msgid);
			cJSON_AddNumberToObject(json,"type",0);
			cJSON_AddStringToObject(json,"data",(char *)(local_response.data));
			cJSON_AddItemToArray(msglist,json);

			cJSON_AddItemToObject(object,"msglist",msglist);
			cJSON_AddItemToObject(root,"object",object);

			char *json_str = cJSON_Print(root);
			//copy json str out
			void *data = malloc(strlen(json_str) + 256);
			sprintf((char*)data,"%s",json_str);
			if(root) cJSON_Delete(root);
			if(json_str) free(json_str);

            char url[256] = "";
            sprintf(url,"%s%s",XCLOUD_REMOTE_DATA_ADDR,XCLOUD_REMOTE_TABLE_USERSUB);
			struct HttpResonseData response;
			response.size = 0;
			LOG("postMessageToUser--data=%s\n",(char*)data);
			int32_t ret3 = postHttps(url,data,&response);
			LOG("postMessageToUser---ret= %d --response=%s\n",ret3,(response.size > 0) ? (char*)response.data : "");
			free(data);
			if(ret3 != CURLE_OK || response.size == 0){
			    LOG("post message %p to user fail ret= %d\n",(void *)msgid,ret3);
			    if(response.size > 0) free(response.data);
				    ret = -1;
			}
			if(response.size > 0) free(response.data);
			ret = 0;
            sleep(3);
        }
    }
    return NULL;
}

/*json data like {"action": 0}*/
int32_t handle_system_event(char *user_json,char *error)
{
    int32_t res = -1;
    g_upgrade_process = 0;
    cJSON* json = cJSON_Parse(user_json);
    if(!json) goto jsonerror;
    cJSON *object = cJSON_GetObjectItem(json,SYSTEM_EVENT_ACTION_STR);
    if(!object) goto jsonerror;
    int action = -1;
    cJSON_GetValueInt(object, &action);
    LOG("[server] handle_system_event--user_json=%s\n",user_json);
    switch(action){
        case SYSTEM_EVENT_ACTION_REMOTE_SERVER_CONNECTED:
            {
                //send SYNC_EVENT to unresolvedEventQueue
                TotallySyncEvent *SyncEventMsg = (TotallySyncEvent *)malloc(sizeof(TotallySyncEvent));
                SyncEventMsg->unresolvedEvent.tagMessage.type = SYNC_EVENT;

                SyncEventMsg->unresolvedEvent.data = NULL;
                SyncEventMsg->unresolvedEvent.firstEnqueueTime = time(NULL);
                sendMessage(g_sync_context.unresolvedEventQueue,(void *)SyncEventMsg);

                //send UPDATE_LOCAL_INFO_EVENT to unresolvedEventQueue
                periodSendDevinfoEvent();



                pthread_mutex_lock(&g_sync_context.g_sync_mutex);
                pthread_cond_signal(&g_sync_context.g_sync_cond);
                pthread_mutex_unlock(&g_sync_context.g_sync_mutex);
                break;
            }
        case SYSTEM_EVENT_ACTION_UPGRADE:
            {
                periodGetUpgradeProcessEvent_t *periodGetUpgradeProcessMsg = (periodGetUpgradeProcessEvent_t *)malloc(sizeof(periodGetUpgradeProcessEvent_t));
                periodGetUpgradeProcessMsg->tagMessage.type = GET_UPGRADE_PROCESS_EVENT;
                sprintf(periodGetUpgradeProcessMsg->tagMessage.tag, "%s", GET_UPGRADE_PROCESS_TAG);
                periodGetUpgradeProcessMsg->startFlag = 1;
                periodGetUpgradeProcessMsg->last_stamp = 0;
                periodGetUpgradeProcessMsg->firstEnqueueTime = time(NULL);
                periodGetUpgradeProcessMsg->ota_type = 0;
                periodGetUpgradeProcessMsg->mode = 0;

				cJSON *userid_obj = cJSON_GetObjectItem(json, "userid");
                char *userid_tmp = cJSON_Print(userid_obj);
				if (userid_tmp == NULL) {
					break;
				}
                snprintf(periodGetUpgradeProcessMsg->userid, strlen(userid_tmp) - 1, "%s", userid_tmp + 1);

                cJSON *msgid_obj = cJSON_GetObjectItem(json, "msgid");
                char *msgid_tmp = cJSON_Print(msgid_obj);
                sprintf(periodGetUpgradeProcessMsg->msgid, "%s", msgid_tmp);

				cJSON *usersubid_obj = cJSON_GetObjectItem(json, "usersubid");
                char *usersubid_tmp = cJSON_Print(usersubid_obj);
                snprintf(periodGetUpgradeProcessMsg->usersubid, strlen(usersubid_tmp) - 1, "%s", usersubid_tmp + 1);
                free(userid_tmp);
                free(msgid_tmp);
                free(usersubid_tmp);
				cJSON *ota_type_obj = cJSON_GetObjectItem(json, "ota_type");
				if(ota_type_obj)
                  cJSON_GetValueInt(ota_type_obj, &(periodGetUpgradeProcessMsg->ota_type));

				cJSON *mode_obj = cJSON_GetObjectItem(json, "mode");
				if(mode_obj)
                  cJSON_GetValueInt(mode_obj, &(periodGetUpgradeProcessMsg->mode));

                pthread_t upgradeProcess;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, 1);
                pthread_create(&upgradeProcess, &attr, notifyUpgradeProcess, periodGetUpgradeProcessMsg);

            }
        case SYSTEM_EVENT_ACTION_UPGRADE_DONE:
            {
                g_upgrade_process = 1;
                int32_t count = 0;
                for(count=0; count<5; count++){
                    if(g_upgrade_process == 0){
                        break;
                    }else{
                        sleep(1);
                    }
                }
            }
        case SYSTEM_EVENT_ACTION_REBOOT:
        case SYSTEM_EVENT_ACTION_RESET:
            {
                //notify server online
                break;
            }
        default:
            {
                sprintf(error,"%s","unknown action\n");
                break;
            }
    }
    if(json) cJSON_Delete(json);
    if(action != SYSTEM_EVENT_ACTION_UPGRADE_DONE){
        res = publishRtStatusChangeEvent(action);
        if(res != 0) sprintf(error,"%s","handle action fail\n");
    }else{
        res = 0;
    }
    return res;
jsonerror:
    sprintf(error,"%s","parse invite message data fail\n");
    if(json) cJSON_Delete(json);
    return -1;
}

void prepareCallbackData(char **callback,char *ret,char *reason)
{
    if(callback){
        cJSON *json = cJSON_CreateObject();
        cJSON_AddStringToObject(json,"ret",ret);
        cJSON_AddStringToObject(json,"reason",reason);
        *callback = (char *)malloc(strlen(reason) + 256);
        char *json_tmp = cJSON_Print(json);
        sprintf(*callback,"%s", json_tmp);
        if(json) cJSON_Delete(json);
        if(json_tmp) free(json_tmp);
    }
}

void on_system_event(char *data, char **callback)
{
    LOG("[server]%s, args : %s\n",__func__, data ? data : "NULL");
    if(!data){
        LOG("[server] args is null!\n");
        return;
    }
    char tmp[256] = "";
    int32_t ret = handle_system_event(data,tmp);
    if(callback){
        prepareCallbackData(callback,ret < 0 ? "fail" : "success",tmp);
        LOG("[server] write system event ret=%d callback:%s!\n",ret,*callback);
    }
}

void on_sub_event(char *data, char **callback)
{
    LOG("[server]%s, args : %s\n",__func__, data ? data : "NULL");
    if(!data){
        LOG("[server] args is null!\n");
        return;
    }
    char tmp[256] = "";
    int32_t ret = handle_sub_event(data,tmp);
    if(callback){
        prepareCallbackData(callback,ret < 0 ? "fail" : "success",tmp);
        LOG("[server] write system event ret=%d callback:%s!\n",ret,*callback);
    }
}


struct user_info_list g_list;

static int32_t user_info_list_create(const char *ip, int32_t port)
{
	user_info *p_info = NULL;

	/*creat chain table for save user info*/
	if(g_list.head == NULL){
		p_info = (user_info *)malloc(sizeof(user_info));
		memset(p_info, 0, sizeof(user_info));
		strcpy(p_info->ip, ip);
		p_info->port = port;
		p_info->next = NULL;
		g_list.head = p_info;
		g_list.tail = p_info;
		g_list.list_len = 1;
	}else{
		user_info *check = g_list.head;
		/*check if user_info_list has the same ip to new ip,replace it's port if diffrent*/
		while(check != NULL){
			if(!strncmp(check->ip, ip, sizeof(check->ip))){
				if(check->port != port)
				  check->port = port;
				LOG("[server]%s, find user has the same ip,replace it's port if diffrent\n",__func__);
				return 0;
			}
			LOG("[server]%s, check user_info_list ip: %s port: %d\n",__func__, check->ip, check->port);
			check = check->next;
		}
		/*if new mac diffrent from user_info_list,save it*/
		p_info = (user_info *)malloc(sizeof(user_info));
		memset(p_info, 0, sizeof(user_info));
		strcpy(p_info->ip, ip);
		p_info->port = port;
		p_info->next = NULL;
		g_list.tail->next = p_info;
		g_list.tail = p_info;
		g_list.list_len += 1;
	}

	return 0;
}

/*delete an entry in user_info_list*/
static user_info *
user_info_list_delete(user_info *entry)
{
	user_info *tmp = NULL;

	if(entry == g_list.head){
		g_list.head = entry->next;
		free(entry);
		return g_list.head;
	}else{
		tmp = g_list.head;
		while(tmp != NULL){
			if(tmp->next == entry){
				tmp->next = entry->next;
				free(entry);
				break;
			}
			tmp = tmp->next;
		}
		return tmp->next;
	}
}

static int32_t deleteUselessUserInfo(char *ip_list[], int32_t list_len)
{
	int i = 0, flag = 0;
	user_info *check = g_list.head;
	while(check != NULL){
		for(i = 0; i < list_len; i++){
			if(!strcmp(ip_list[i], check->ip)){
				flag = 1;
				break;
			}
		}

		/*if online ip_list contain check,do nothing.or delet check*/
		if(flag){
			flag = 0;
			check = check->next;
		}else{
			LOG("%s, free user info for it's no exist in devlist ip: %s port : %d\n",__func__,check->ip, check->port);
			check = user_info_list_delete(check);
			g_list.list_len -= 1;
		}
	}
	LOG( "%s, after deleteUselessUserInfo devlist_len: %d",__func__,g_list.list_len);
	return 0;
}

/*case app user connect to router, save user info in chain table for sent message to user later*/
/*json data like {"action": 0}*/
int32_t do_save_user_info(char *data, char **callback)
{
	//LOG("[server]%s, info-data : %s\n",__func__, data ? data : "NULL");
	char tmp[256] = "";
	int32_t i = 0, ret = -1;
	cJSON* json = cJSON_Parse(data);
	if(!json) goto jsonerror;
	cJSON *ip_obj = cJSON_GetObjectItem(json, "ip");
	if(!ip_obj) goto jsonerror;
	cJSON *port_obj = cJSON_GetObjectItem(json, "port");
	if(!port_obj) goto jsonerror;

	//const char *ip = cJSON_Print(ip_obj);
	const char *ip = (char*)(ip_obj->valuestring);
	int32_t port = -1;
    cJSON_GetValueInt(port_obj, &port);

	ret = user_info_list_create(ip, port);
	LOG("[server]%s, save the last info-data ip: %s port: %d list_len: %d\n",__func__, ip, port, g_list.list_len);

	/*if chain table length over 10, we will check and clean the table*/
	if(g_list.list_len > 10){
		int32_t count = getOnlineDeviceNumberFromUci();
		char *ip_list[count];
		for(i = 0; i < count; i++)
		  ip_list[i] = (char *)malloc(16);

		getOnlineDeviceIpFromUci(ip_list);
		ret = deleteUselessUserInfo(ip_list, count);

		for(i = 0; i < count; i++)
		  free(ip_list[i]);
	}

	if(json) cJSON_Delete(json);
	if(callback){
		prepareCallbackData(callback,ret < 0 ? "fail" : "success",tmp);
		LOG("[server] write system event ret=%d callback:%s!\n",ret,*callback);
	}
	return ret;

jsonerror:
	sprintf(tmp,"%s","do_save_user_info--data parse error\n");
	if(json) cJSON_Delete(json);
	if(callback){
		prepareCallbackData(callback,ret < 0 ? "fail" : "success",tmp);
		LOG("[server] write system event ret=%d callback:%s!\n",ret,*callback);
	}
	return ret;
}
