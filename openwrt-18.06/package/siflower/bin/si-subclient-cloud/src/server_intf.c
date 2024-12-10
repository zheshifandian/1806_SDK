#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <sys/timeb.h>
#include <uci.h>
#include <regex.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include <sys/types.h>
#include <lws_config.h>
#include <libwebsockets.h>

#include "cJSON.h"
#include "server_intf.h"
#include "cloud_common.h"
#include "local_stub.h"
#include "remote_https.h"

extern char CLOUD_CODE_VERSION[64];
extern char CLOUD_ROUTERSUB_OBJECT[20];
extern char CLOUD_ROUTER_OBJECT[20];
extern int CLOUD_BIND;
extern char SIVERSION[8];

#define type_disconnect 0
#define type_connect 1
#define type_heartbeat 2
#define type_message 3
#define type_json 4
#define type_event 5
#define type_ack 6
#define type_error 7
#define type_noop 8

#define ACTION_XCLOUD_SUB_INSERT 1
#define ACTION_XCLOUD_SUB_UPDATE 2
#define ACTION_XCLOUD_SUB_DELETE 3

#define FromRouter		0

#define MODE_XCLOUD_SUB_HEARTBEAT 0
#define MODE_XCLOUD_SUB_START 1
#define MODE_XCLOUD_SUB_STOP 2

void sendPacket(struct libwebsocket *wsi,int32_t  type,char *data)
{
	char typeStr[16] = "";
	void *messageData = NULL;
	sprintf(typeStr,"%d",type);
	switch(type){
		case type_heartbeat:
			{
				LOG("send reply heartbeat message\n");
				if(strlen(data) != 0){
					//char heartbeat_msg[100] = "";
					//sprintf(heartbeat_msg,"{\"mode\": %d}",MODE_XCLOUD_SUB_HEARTBEAT);
					messageData = malloc(strlen(data) + 1);
					memcpy(messageData,data,strlen(data));
					*((char*)messageData + strlen(data)) = 0;
				}
				break;
			}
		case type_event:
			{
				if(strlen(data) != 0){
					messageData = malloc(strlen(data) + 1);
					memcpy(messageData,data,strlen(data));
					*((char*)messageData + strlen(data)) = 0;
				}
				break;
			}
		default:
			{
				LOG("send unknown packet \n");
				return;
			}
	}
	void *sendBuffer = NULL;
	int32_t  len = LWS_SEND_BUFFER_PRE_PADDING + (messageData == NULL ? 0 : strlen((char*)messageData)) + 10
		+ LWS_SEND_BUFFER_POST_PADDING;
	sendBuffer = malloc(len);
	memset(sendBuffer,'\1',LWS_SEND_BUFFER_PRE_PADDING);
	sprintf((char*)(sendBuffer + LWS_SEND_BUFFER_PRE_PADDING),"%s",(messageData == NULL) ? "" : (char *)messageData);
	LOG("send buffer %s\n",(char*)sendBuffer + LWS_SEND_BUFFER_PRE_PADDING);

	int32_t  n = libwebsocket_write(wsi, sendBuffer + LWS_SEND_BUFFER_PRE_PADDING, strlen((char*)sendBuffer) - LWS_SEND_BUFFER_PRE_PADDING, LWS_WRITE_TEXT);
	LOG("send return %d\n",n);
	if (n < 0) {
		LOG("send packet fail---\n");
	}
	if(messageData != NULL) free(messageData);

	if(sendBuffer) free(sendBuffer);
}


static const char *XCLOUD_SUB_FORMAT = "{\"tableName\": \"%s\",\"mode\": %d,\"listenerId\": %d,\"action\": %d,\"subId\":\"%s\" ,\"objectId\":\"%s\"}";
static const char *XCLOUD_SUB_START_FORMAT = "{\"tableName\": \"%s\",\"mode\": %d,\"listenerId\": %d,\"action\": %d,\"subId\":\"%s\" ,\"objectId\":\"%s\",\"messageType\":\"%d\"}";

void subRow(struct libwebsocket *wsi,const char *tableName,const char *subId, const char *objectId)
{
	char data[512] = "";
	sprintf(data,XCLOUD_SUB_START_FORMAT,tableName,MODE_XCLOUD_SUB_START,0,ACTION_XCLOUD_SUB_UPDATE,subId,objectId,FromRouter);
	sendPacket(wsi,type_event,data);
}

void unsubRow(struct libwebsocket *wsi,const char *tableName,const char *subId, const char *objectId,int hashcode)
{
	char data[512] = "";
	sprintf(data,XCLOUD_SUB_FORMAT,tableName,MODE_XCLOUD_SUB_STOP,hashcode,ACTION_XCLOUD_SUB_DELETE,subId,objectId);
	sendPacket(wsi,type_event,data);
}

enum CloudSubClientError get_web_socket_path(char *path_in)
{
	sprintf(path_in,"/%s/%s",SIVERSION,XCLOUD_WEBSOCKET_PATH);
	return WS_CLIENT_OK;
}

void appendCommonWbsHeader(char *src_header)
{
	//do nothing
}


void parseUserMessage(void *buffer,size_t size,void *context)
{
    LOG("parseUserMessage---size-%d context %p buffer-%s \n",(int)size,context,(char *)buffer);
    char *str = malloc(size + 1);
    memcpy(str,buffer,size);
    *(str + size) = '\0';
    //parse json
    cJSON* json = cJSON_Parse(str);
    //parse msg
    if(!json) goto clean;
    LOG("parseUserMessage success--\n");
    cJSON *data = cJSON_GetObjectItem(json,"data");
	if(!data){
        LOG("data not exist\n");
        goto clean;
	}
    cJSON *msglist = cJSON_GetObjectItem(data,"msglist");
    if(!msglist){
        LOG("results not exist\n");
        goto clean;
    }
    if(msglist->type != cJSON_Array){
        LOG("UserMessage result not array\n");
        goto clean;
    }
    int length = cJSON_GetArraySize(msglist);
    if(length <= 0){
        LOG("UserMessage object empty\n");
        goto clean;
    }
    int i;
    LOG("get %d count of UserMessage--\n",length);
    for(i = 0;i < length;i++) {
        //userid objectId msgid data
        cJSON *json_msg = cJSON_GetArrayItem(msglist,i);
        if(json_msg){
            char *msg_obj_id = NULL;
            char msg_msg_id[32] = "";
            char *msg_user_id = "";
            char msg_user_id_format[32] = "";
            char *msg_sub_id = "";
            char msg_sub_id_format[32] = "";
            char *msg_data = NULL;
            int msg_type = 0;

            cJSON *obj_msg = cJSON_GetObjectItem(json_msg,"objectId");
            if(obj_msg != NULL) msg_obj_id = (char*)(obj_msg->valuestring);
            cJSON *id_msg = cJSON_GetObjectItem(json_msg,"msgid");
            if(id_msg != NULL) sprintf(msg_msg_id,"%d",id_msg->valueint);
            cJSON *type_msg = cJSON_GetObjectItem(json_msg,"type");
            if(type_msg != NULL) msg_type = (type_msg->valueint);
            //get userid
            cJSON *user_msg = cJSON_GetObjectItem(json_msg,"userid");
            msg_user_id = (char*)(user_msg->valuestring);
            if(msg_user_id != NULL){
                formatObjectId(msg_user_id,msg_user_id_format);
            }
            //get subid
            cJSON *sub_msg = cJSON_GetObjectItem(json_msg,"subid");
            msg_sub_id = (char*)(sub_msg->valuestring);
            if(msg_sub_id != NULL){
                formatObjectId(msg_sub_id,msg_sub_id_format);
            }
            //get data
            cJSON *data_msg = cJSON_GetObjectItem(json_msg,"data");
            if(data_msg != NULL)
				msg_data = (char*)(data_msg->valuestring);
			if(context != NULL){
				struct CloudUserContext *userContext = (struct CloudUserContext *)(context);
				userContext->onUserMessage(msg_msg_id,msg_user_id_format,msg_sub_id_format,msg_type,msg_obj_id,msg_data,context);
			}
        }
    }

clean:
    if(str) free(str);
    //free
    if(json) cJSON_Delete(json);
}

int32_t checkMessageFromServer(struct CloudServerMessage *cloudMsg)
{
	if(cloudMsg->length > 0){
		//parser user message
		parseUserMessage(cloudMsg->data,cloudMsg->length,cloudMsg->context);
		free(cloudMsg->data);
		cloudMsg->length = 0;
		return 0;
	}
	ADD_DEBUG_POINT(105);
    return -1;
}

#define XCLOUD_PUB_TYPE_SUB_RESPONSE 1
#define XCLOUD_PUB_TYPE_DATA 2

void onPayload(struct libwebsocket *wsi,void *in, size_t len)
{
	int hash = 0;
	int code = 0;
	int type = 0;
	if(len <= 0){
		LOG("onPayload length error");
		return;
	}
	if(parseIntFromData("code",in,&code) || parseIntFromData("type",in,&type)){
		LOG("parser server pub message fail\n");
	}
	if(code != 0){
		LOG("parse server pub code = %d fail\n",code);
		goto bail;
	}

	switch(type){
		case XCLOUD_PUB_TYPE_DATA:
		{
			void *userData = libwebsockets_get_protocol(wsi)->user;
			if(userData != NULL){
				CloudUserContext *userContext = (CloudUserContext*)userData;
				//case the default handle event handle message once receive,so should we sent message once we receive
				struct CloudServerMessage *msg = malloc(sizeof(CloudServerMessage));
				msg->data = malloc(len);
				msg->length = len;
				strcpy(msg->tableName,XCLOUD_REMOTE_TABLE_ROUTER);
				strcpy(msg->objectId,CLOUD_ROUTERSUB_OBJECT);
				strncpy(msg->data,(char *)in, len);
				msg->context = (void*)userContext;
				LOG("handleServerPubMessage--send message--%p\n",msg);
				sendMessage(userContext->serverMsgQueue,(void*)msg);
			}
			break;
		}
		case XCLOUD_PUB_TYPE_SUB_RESPONSE:
		{
			char data[256] = {0};
			if(len > sizeof(data)){
				LOG("data size large then %d",sizeof(data));
				goto bail;
			}
			int res = parseStringFromData("data",in,data);
			if(res == 0){
				res = parseIntFromData("hashcode",data,&hash);
				if(res == 0){
					LOG("sub success------------hash=%d\n",hash);
				}
			}else{
				LOG("parse hash code data fail\n");
			}
			break;
		}
		default:{
			LOG("unknown msg type = %d fail\n",type);
		}
	}
bail:
	return;
}

/*
 * IMPORTANT NOTICE!
 *
 * When sending with websocket protocol (LWS_WRITE_TEXT or LWS_WRITE_BINARY)
 * the send buffer has to have LWS_SEND_BUFFER_PRE_PADDING bytes valid BEFORE
 * buf, and LWS_SEND_BUFFER_POST_PADDING bytes valid AFTER (buf + len).
 *
 * This allows us to add protocol info before and after the data, and send as
 * one packet on the network without payload copying, for maximum efficiency.
 */
void sendPing(struct libwebsocket *wsi)
{
	int n = 0;
	unsigned char pingbuf[LWS_SEND_BUFFER_PRE_PADDING + MAX_PING_PAYLOAD + LWS_SEND_BUFFER_POST_PADDING];

	memset(&pingbuf[LWS_SEND_BUFFER_PRE_PADDING], 0, MAX_PING_PAYLOAD);
	sprintf((char *)&pingbuf[LWS_SEND_BUFFER_PRE_PADDING], "{\"routerid\":\"%s\"}", CLOUD_ROUTER_OBJECT);
	LOG("ping data len=%d content='%s'---time=%ld\n", MAX_PING_PAYLOAD, &pingbuf[LWS_SEND_BUFFER_PRE_PADDING], time(NULL));
	n = libwebsocket_write(wsi, &pingbuf[LWS_SEND_BUFFER_PRE_PADDING], MAX_PING_PAYLOAD, LWS_WRITE_PING);
	if (n < 0)
	  LOG("%s, send ping error!\n", __func__);
}
