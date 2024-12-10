/*
 * =====================================================================================
 *
 *       Filename:  local_stub.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2015年07月29日 10时32分35秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert , robert.chang@siflower.com.cn
 *        Company:  siflower
 *
 * =====================================================================================
 */

#include "local_stub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <shadow.h>
#include <regex.h>
#include <unistd.h>
#include "cJSON.h"
#include "cloud_common.h"
#include "local_http.h"
#include "local_storage.h"
#include "remote_https.h"
#include "server_intf.h"
//#define LOG printf

extern char CLOUD_CODE_VERSION[64];
extern char CLOUD_ROUTERSUB_OBJECT[20];
extern char CLOUD_ROUTER_OBJECT[20];
extern int CLOUD_BIND;

void clearHistoryMessage()
{
    //prepare message
    cJSON *json = cJSON_CreateObject();
    char sn[50] = "";
    getSfHardwareConfig("sn",sn);
    cJSON_AddStringToObject(json,"sn",sn);
    cJSON_AddStringToObject(json,"version",CLOUD_CODE_VERSION);
    char *json_str = cJSON_Print(json);
    //copy json str out
    void *data = malloc(strlen(json_str) + 256);
    sprintf((char*)data,"%s",json_str);
    if(json) cJSON_Delete(json);
    if(json_str) free(json_str);
    //use remote function to handle this message
    char url[256] = "";
    sprintf(url,"%s/%s",CLOUD_REMOTE_FUNCTION_ADDR,CLOUD_REMOTE_FUNCTION_CLEAR_ROUTER_MESSAGE);
    struct HttpResonseData response;
    response.size = 0;
    LOG("clearHistory--data=%s\n",(char*)data);
    int32_t ret3 = postHttps(url,data,&response,1);
    LOG("clearHistory---ret= %d --response=%s\n",ret3,(response.size > 0) ? (char*)response.data : "");
    free(data);
    if(ret3 != CURLE_OK || response.size == 0){
        LOG("clear Router History message failed--ret= %d\n",ret3);
    }
    if(response.size > 0) free(response.data);
}

void handleHistoryMessage(struct libwebsocket *wsi)
{
    //prepare message
    cJSON *json = cJSON_CreateObject();
    char sn[50] = "";
    getSfHardwareConfig("sn",sn);
    cJSON_AddStringToObject(json,"sn",sn);
    cJSON_AddStringToObject(json,"version",CLOUD_CODE_VERSION);
    char *json_str = cJSON_Print(json);
    //copy json str out
    void *data = malloc(strlen(json_str) + 256);
    sprintf((char*)data,"%s",json_str);
    if(json) cJSON_Delete(json);
    if(json_str) free(json_str);
    //use remote function to handle this message
    char url[256] = "";
    sprintf(url,"%s/%s",CLOUD_REMOTE_FUNCTION_ADDR,CLOUD_REMOTE_FUNCTION_GET_USER_MESSAGE);
    struct HttpResonseData response;
    response.size = 0;
    int32_t ret3 = postHttps(url,data,&response,1);
    LOG("getHistory---ret= %d --response=%s\n",ret3,(response.size > 0) ? (char*)response.data : "");
    free(data);
    if(ret3 != CURLE_OK || response.size == 0){
        LOG("get Router History message failed--ret= %d\n",ret3);
		if(response.size > 0) free(response.data);
		return;
    }

	if(response.size > 0){
		//parse remote history message
		struct cJSON *serverResponse = cJSON_Parse(response.data);
		if (!serverResponse){
			free(response.data);
			return;
		}

		json_str = cJSON_Print(serverResponse);
		int32_t msg_len = strlen(json_str);
		void *userData = libwebsockets_get_protocol(wsi)->user;
		if(userData != NULL){
			CloudUserContext *userContext = (CloudUserContext*)userData;
			struct CloudServerMessage *msg = malloc(sizeof(CloudServerMessage));
			msg->data = malloc(msg_len);
			msg->length = msg_len;
			strcpy(msg->tableName,XCLOUD_REMOTE_TABLE_ROUTER);
			strcpy(msg->objectId,CLOUD_ROUTERSUB_OBJECT);
			strncpy(msg->data, json_str, msg_len);
			msg->context = (void*)userContext;
			sendMessage(userContext->serverMsgQueue,(void*)msg);
		}
		cJSON_Delete(serverResponse);
		free(response.data);
        if(json_str) free(json_str);
	}
}

void deleteMsg(char *routerSubId,char *originMsg)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"method","update");
	//add relation
    cJSON *relation = cJSON_CreateObject();
    cJSON_AddStringToObject(relation,"column","msglist");
    cJSON_AddStringToObject(relation,"method","delete");
    cJSON_AddItemToObject(root,"relation",relation);
	//add object
	cJSON *object = cJSON_CreateObject();
    cJSON_AddStringToObject(object,"objectId",routerSubId);
	//add message list
	cJSON *msglist = cJSON_CreateArray();
	//add one message
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json,"objectId",originMsg);
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
    sprintf(url,"%s%s",XCLOUD_REMOTE_DATA_ADDR,XCLOUD_REMOTE_TABLE_ROUTERSUB);
    struct HttpResonseData response;
    response.size = 0;
    LOG("deletemsg--data=%s\n",(char*)data);
    int32_t ret3 = postHttps(url,data,&response,1);
    LOG("deletemsg---ret= %d --response=%s\n",ret3,(response.size > 0) ? (char*)response.data : "");
    free(data);
    if(ret3 != CURLE_OK || response.size == 0){
        LOG("deletemsg fail ret= %d\n",ret3);
    }
    if(response.size > 0) free(response.data);
}

int32_t postMessageToUser(char *msgId,char *userId,char *subId,int type,char *messageData,char *originMessageObject)
{
	int ret = 0;
    LOG("postMessageToUser--msgid %s userId %s subId %s type %d data %s origin %s",msgId,userId,subId,type,messageData,originMessageObject? originMessageObject : "");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"method","update");
	//add relation
    cJSON *relation = cJSON_CreateObject();
    cJSON_AddStringToObject(relation,"column","msglist");
    cJSON_AddStringToObject(relation,"method","insert");
    cJSON_AddItemToObject(root,"relation",relation);
	//add object
	cJSON *object = cJSON_CreateObject();
    cJSON_AddStringToObject(object,"objectId",subId);
	//add message list
	cJSON *msglist = cJSON_CreateArray();
	//add one message
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json,"userid",userId);
    cJSON_AddStringToObject(json,"routerid",CLOUD_ROUTER_OBJECT);
    cJSON_AddStringToObject(json,"subid",CLOUD_ROUTERSUB_OBJECT);
    cJSON_AddNumberToObject(json,"msgid",(atoi(msgId)));
    cJSON_AddNumberToObject(json,"type",type);
    cJSON_AddStringToObject(json,"data",messageData);
	cJSON_AddItemToArray(msglist,json);

	cJSON_AddItemToObject(object,"msglist",msglist);
    cJSON_AddItemToObject(root,"object",object);

    char *json_str = cJSON_Print(root);
    //copy json str out
    void *data = malloc(strlen(json_str) + 256);
    sprintf((char*)data,"%s",json_str);
    if(root) cJSON_Delete(root);
    if(json_str) free(json_str);
    //use remote function to handle this message
    char url[256] = "";
    sprintf(url,"%s%s",XCLOUD_REMOTE_DATA_ADDR,XCLOUD_REMOTE_TABLE_USERSUB);
    struct HttpResonseData response;
    response.size = 0;
    LOG("postMessageToUser--data=%s\n",(char*)data);
    int32_t ret3 = postHttps(url,data,&response,1);
    LOG("postMessageToUser---ret= %d --response=%s\n",ret3,(response.size > 0) ? (char*)response.data : "");
    free(data);
    if(ret3 != CURLE_OK || response.size == 0){
        LOG("post message %p to user fail ret= %d\n",msgId,ret3);
        ret = -1;
    }else{
		ret = 0;
	}
    if(response.size > 0) free(response.data);
	//if(originMessageObject != NULL) deleteMsg(CLOUD_ROUTERSUB_OBJECT,originMessageObject);
	return ret;
}

int32_t postMessageToUserbyws(char *msgId,char *userId,char *subId,int type,char *messageData,char *originMessageObject)
{
	int ret = 0;
	int32_t  n = -1;
	int32_t data_len;
	int i = 0;
    char *json_tmp = NULL;

    LOG("postMessageToUser--msgid %s userId %s subId %s type %d data %s origin %s",msgId,userId,subId,type,messageData,originMessageObject? originMessageObject : "");
	cJSON *object = cJSON_CreateObject();
    cJSON_AddNumberToObject(object,"mode",3);
    cJSON_AddNumberToObject(object,"messageType",0);
    cJSON_AddStringToObject(object,"destId",userId);
	cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json,"userid",userId);
    cJSON_AddStringToObject(json,"routerid",CLOUD_ROUTER_OBJECT);
    cJSON_AddStringToObject(json,"subid",CLOUD_ROUTERSUB_OBJECT);
    cJSON_AddNumberToObject(json,"msgid",(atoi(msgId)));
    cJSON_AddNumberToObject(json,"type",type);
    cJSON_AddStringToObject(json,"data",messageData);
    json_tmp = cJSON_Print(json);
    cJSON_AddStringToObject(object,"data",json_tmp);

    char *json_str = cJSON_Print(object);
	data_len = strlen(json_str) + LWS_SEND_BUFFER_POST_PADDING;
    void *data = malloc(LWS_SEND_BUFFER_PRE_PADDING + data_len);
	memset(data,'\1',LWS_SEND_BUFFER_PRE_PADDING);
	memset(data+LWS_SEND_BUFFER_PRE_PADDING+data_len-LWS_SEND_BUFFER_POST_PADDING,'\1',LWS_SEND_BUFFER_POST_PADDING);
    sprintf((char*)data + LWS_SEND_BUFFER_PRE_PADDING,"%s",json_str);
    if(object) cJSON_Delete(object);
    if(json_str) free(json_str);
    if(json_tmp) free(json_tmp);
	while (i < 10){
		if(gContext->status == WS_CLIENT_CONNECT_ESTABLISH){
			LOG("ws is establish\n");
			n = libwebsocket_write(gContext->wsi, data + LWS_SEND_BUFFER_PRE_PADDING, data_len, LWS_WRITE_TEXT);
			break;
		}
		++i;
		usleep(10*1000);
	}
	if(n <= 0){
		LOG("ws post message fail\n");
        ret = -1;
	}
	free(data);
	//if(originMessageObject != NULL) deleteMsg(CLOUD_ROUTERSUB_OBJECT,originMessageObject);
	return ret;
}

int32_t postMessageToUserbyhttp(char *msgId,char *userId,char *subId,int type,char *messageData,char *originMessageObject)
{
	int ret = 0;
    char *json_tmp = NULL;

	LOG("postMessageToUserbyhttp--msgid %s userId %s subId %s type %d data %s origin %s",msgId,userId,subId,type,messageData,originMessageObject? originMessageObject : "");
	//add object
	cJSON *object = cJSON_CreateObject();
	cJSON_AddStringToObject(object,"userid",userId);
	cJSON_AddStringToObject(object,"usersubid",subId);
	//add one message
	cJSON *json = cJSON_CreateObject();
	cJSON_AddStringToObject(json,"userid",userId);
	cJSON_AddStringToObject(json,"routerid",CLOUD_ROUTER_OBJECT);
	cJSON_AddStringToObject(json,"subid",CLOUD_ROUTERSUB_OBJECT);
	cJSON_AddNumberToObject(json,"msgid",(atoi(msgId)));
	cJSON_AddNumberToObject(json,"type",type);
	cJSON_AddStringToObject(json,"data",messageData);
    json_tmp = cJSON_Print(json);
	cJSON_AddStringToObject(object,"message",json_tmp);

	char *json_str = cJSON_Print(object);
	//copy json str out
	void *data = malloc(strlen(json_str) + 256);
	sprintf((char*)data,"%s",json_str);
	if(object) cJSON_Delete(object);
    if(json_str) free(json_str);
    if(json_tmp) free(json_tmp);
	//use remote function to handle this message
	char url[256] = "";
	sprintf(url,"%s/%s",CLOUD_REMOTE_FUNCTION_ADDR,CLOUD_REMOTE_FUNCTION_REPLY_MESSAGE);
	struct HttpResonseData response;
	response.size = 0;
	LOG("postMessageToUserbyhttp--data=%s\n",(char*)data);
	int32_t ret3 = postHttps(url,data,&response,1);
	LOG("postMessageToUserbyhttp---ret= %d --response=%s\n",ret3,(response.size > 0) ? (char*)response.data : "");
	free(data);
	if(ret3 != CURLE_OK || response.size == 0){
		LOG("post message %p to user fail ret= %d\n",msgId,ret3);
		ret = -1;
	}
	if(response.size > 0) free(response.data);
	return ret;
}

int32_t forwardUserHeartbeatResponse(char *userId,char *error)
{
    char cmdBuffer[1024] = "";
    sprintf(cmdBuffer,"SUBE need-callback -data {\"action\": 1,\"userid\": \"%s\"}",userId);
    char retBuf[1024] = "";
    int32_t ret = sendCmdToSockServer(cmdBuffer,retBuf);
    LOG("forwardUserHeartbeatResponse--ret = %d result-%s",ret,retBuf);
    if(ret == 0){
        char msgRet[32] = "";
        //paser ret Buffer
        parseStringFromData("ret",retBuf,msgRet);
        parseStringFromData("reason",retBuf,error);
        ret = !strcmp(msgRet,"success") ? 0 : -1;
    }else{
        sprintf(error,"%s","send heartbeat update fail internal");
    }
    return ret;
}

int32_t forwardUserSubscribeRequest(char *userId,int32_t subType,char *data,char *error)
{
    char cmdBuffer[1024] = "";
    if(!data || strlen(data) > 1024){
        LOG("sub param data too long--");
        return -1;
    }

    sprintf(cmdBuffer,"SUBE need-callback -data {\"action\": 0,\"type\": %d,\"userid\": \"%s\",\"data\": %s}",subType,userId,data);
    char retBuf[1024] = "";
    int32_t ret = sendCmdToSockServer(cmdBuffer,retBuf);
    LOG("forwardUserSubscribeRequest--ret = %d result-%s",ret,retBuf);
    if(ret == 0){
        char msgRet[32] = "";
        //paser ret Buffer
        parseStringFromData("ret",retBuf,msgRet);
        parseStringFromData("reason",retBuf,error);
        ret = !strcmp(msgRet,"success") ? 0 : -1;
    }else{
        sprintf(error,"%s","send subrequest fail internal");
    }
    return ret;
}

struct CloudReplyMessage* transferUserMessage(char *msgId,char *userId,char *subId,int type,char *objId,char *msgData)
{
    char postData[16] = "";
    char errorMessage[256] = "";
    struct HttpResonseData *response = (struct HttpResonseData *)malloc(sizeof(struct HttpResonseData));
    response->size = 0;
    response->data = NULL;
	response->next = NULL;
    char *data = msgData;
    char *path = NULL;
    char *body = NULL;
    char *data_str = NULL;
    //check if data is json_string_type case c_json will translate all '\"' to '"' when parse data from message
    //data of type normal should not be json format
	struct cJSON *json_data =cJSON_Parse(data);
    if(type != CLOUD_USER_MESSAGE_TYPE_NORMAL ) {
		if(json_data) data_str = (char*)cJSON_Print(json_data);
	}
    LOG("transferUserMessage++++++++++++++++%s",data);
    switch(type){
        case CLOUD_USER_MESSAGE_TYPE_NORMAL:
            {
				//struct cJSON* json = cJSON_Parse(msgData);
				struct cJSON *json = cJSON_GetObjectItem(json_data,"path");
				if(json != NULL){
					path = (char*)(json->valuestring);
				}else{
					LOG("++++++++++++++++get path error");
					goto  DATAERR;
				}

				json = cJSON_GetObjectItem(json_data,"body");
				//json_data = cJSON_GetObjectItem(json_data,"version");
				if(json != NULL){
					//body = (char*)(json_data->valuestring);
					body = cJSON_Print(json);
				}else{
					LOG("++++++++++++++++get body error");
					goto  DATAERR;
				}

				int32_t timeout = -1;
				//set timeout for netdetect command
				if(strstr(path,"netdetect")){
					timeout = 40;
				}else if(strstr(path,"wds_getrelip")){
					timeout = 60;
				}else if(strstr(path,"wds_enable")){
					timeout = 60;
				}
                //1 post it to local uhttp server and wait for response
                int ret = postDataToHttpd(path,body,response,userId,timeout,1);
                if(body) free(body);
				if(response->size > 0){
					response = JoinHttpResponseData(response);
					LOG("get httpd response----ret=%d---data: %s --time=%ld s\n",ret,(response->size > 0) ? (char*)(response->data) : "NULL",time(NULL));
				}else{
					sprintf(errorMessage,"{\"code\": -1 ,\"msg\":\"post message to uhttpd fail ret=%d\"}",ret);
				}
                break;
DATAERR:
                LOG("get json data nil");
                sprintf(errorMessage,"{\"code\": -1 ,\"msg\":\"error normal message--%s, get path or body nil\"}",data ? data : "NULL");
                break;
            }
        case CLOUD_USER_MESSAGE_TYPE_SUB_ROUTER:
            {
                int32_t ret = forwardUserSubscribeRequest(userId,0,data,errorMessage);
                if(ret != 0){
                    sprintf(errorMessage,"{\"code\": -1 ,\"msg\":\"%s\"}",errorMessage);
                }else{
                    sprintf(errorMessage,"{\"code\": 0 ,\"msg\":\"OK\"}");
                }
                break;
            }
        case CLOUD_USER_MESSAGE_TYPE_SUB_DEVICE:
            {
                int32_t ret = forwardUserSubscribeRequest(userId,1,data,errorMessage);
                if(ret != 0){
                    sprintf(errorMessage,"{\"code\": -1 ,\"msg\":\"%s\"}",errorMessage);
                }else{
                    sprintf(errorMessage,"{\"code\": 0 ,\"msg\":\"OK\"}");
                }
                break;
            }
        case CLOUD_USER_MESSAGE_TYPE_HEARTBEAT:
            {
                int action = 0;
                char heartBeatData[256] = "";
                if(strlen(data) > 256) goto PARSERERR;
                //not data of message is translate like "{\"action\": 0}" by json_c,we have to parse another once
                sprintf(heartBeatData,"{\"data\": %s}",data);
                struct cJSON *heartBeatJson = cJSON_Parse(heartBeatData);
                if(heartBeatJson){
                    cJSON *heartBeatJsonData = cJSON_GetObjectItem(heartBeatJson,"data");
                    if(heartBeatJsonData){
                        if(parseIntFromData("action",(void *)(heartBeatJsonData->valuestring),&action) == 0){
                            if(action == 0){
                                //send reply
                                //prepare response of heartbeat reply
                                response->data = malloc(response->size = 128);
                                //prepare heartbeat data
                                cJSON *json = cJSON_CreateObject();
                                cJSON_AddNumberToObject(json,"code",0);
                                cJSON_AddStringToObject(json,"msg","OK");
                                char * json_tmp = cJSON_Print(json);
                                sprintf((char*)response->data,"%s",json_tmp);
                                if(json) cJSON_Delete(json);
                                if(json_tmp) free(json_tmp);
                            }else if(action == 1){
                                int32_t ret2 = forwardUserHeartbeatResponse(userId,errorMessage);
                                if(ret2 != 0){
                                    sprintf(errorMessage,"{\"code\": -1 ,\"msg\":\"%s\"}",errorMessage);
                                }else{
                                    sprintf(errorMessage,"{\"code\": 0 ,\"msg\":\"OK\"}");
                                }
                            }else{
                                sprintf(errorMessage,"{\"code\": -1 ,\"msg\":\"action not support1\"}");
                            }
                            break;
                        };
                    }else{
                        goto PARSERERR;
                    }
                    if(heartBeatJson) cJSON_Delete(heartBeatJson);
                }else{
                    goto PARSERERR;
                }
PARSERERR:
                LOG("heartbeat param error");
                sprintf(errorMessage,"{\"code\": -1 ,\"msg\":\"error format action data of heartbeat message--%s\"}",data ? data : "NULL");
                break;
            }
        default:
            {
                sprintf(errorMessage,"{\"code\": -1 ,\"msg\":\"unknown message type %d\"}",type);
                break;
            }
    }
    //save error message
    if(response->size == 0){
        response->size = strlen(errorMessage) + 1;
        response->data = (void *)malloc(response->size);
        strcpy((char*)response->data,errorMessage);
    }
    struct CloudReplyMessage *replyMessage = (struct CloudReplyMessage*)malloc(sizeof(CloudReplyMessage));
    strcpy(replyMessage->messageId,msgId);
    strcpy(replyMessage->userId,userId);
    strcpy(replyMessage->subId,subId);
    strcpy(replyMessage->objId,objId);
    replyMessage->type = CLOUD_ROUTER_MESSAGE_TYPE_NORMAL;
    replyMessage->rtype = type;
    replyMessage->response = response;
    if(json_data) cJSON_Delete(json_data);
    if(data_str) free(data_str);
    return replyMessage;
}
