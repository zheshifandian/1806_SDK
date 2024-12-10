/*
 * libwebsockets-test-client - libwebsockets test implementation
 *
 * Copyright (C) 2011 Andy Green <andy@warmcat.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation:
 *  version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA
 */

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

#include <curl/curl.h>
#include <curl/easy.h>

#include <sys/types.h>
#include <lws_config.h>
#include <libwebsockets.h>

#include "cJSON.h"
#include "local_stub.h"
#include "cloud_common.h"
#include "remote_https.h"
#include "local_http.h"
#include "queue.h"
#include "message.h"
#include "local_storage.h"
#include "thread_ssl.h"
#include "server_intf.h"

//for debug block problem
#define DEBUG_BLOCK 1

extern char CLOUD_CODE_VERSION[64];
extern char CLOUD_ROUTERSUB_OBJECT[20];
extern char CLOUD_ROUTER_OBJECT[20];
extern int CLOUD_BIND;

char SIVERSION[8];
char SF_WS_ADDRESS[64];
char XCLOUD_REMOTE_DATA_ADDR[64];
char XCLOUD_REMOTE_ADDR[64];
char CLOUD_REMOTE_FUNCTION_ADDR[64];
char SF_WS_HOST[64];
int SF_WS_PORT;
struct CloudUserContext *gContext;

static volatile int32_t  g_force_exit_signal = 0;
static volatile int32_t  g_force_reconnect_signal = 0;

static int64_t g_handle_message_count = 0;

//time interval to check wbs action
static int WEBSOCKET_SERVICE_INTERVAL_MS = 200;

extern void checkBindId(void);

int handleReplyMessage(struct CloudReplyMessage *replyMessage)
{
    LOG("handleReplyMessage--%p\n",replyMessage);
    int32_t ret = 0, time1 = getUptime();
	if(replyMessage->rtype == CLOUD_USER_MESSAGE_TYPE_NORMAL){
		//postMessageToUserbyws(replyMessage->messageId,replyMessage->userId,replyMessage->subId,replyMessage->type,replyMessage->response->data,replyMessage->objId);
		ret = postMessageToUserbyhttp(replyMessage->messageId,replyMessage->userId,replyMessage->subId,replyMessage->type,replyMessage->response->data,replyMessage->objId);
	}else{
		ret = postMessageToUser(replyMessage->messageId,replyMessage->userId,replyMessage->subId,replyMessage->type,replyMessage->response->data,replyMessage->objId);
	}

	if (ret == 0){
		//delete message if handle success
		struct CloudUserContext *userContext = (struct CloudUserContext *)(replyMessage->context);
		deleteMsgByTag(userContext->freshMessageList,replyMessage->objId);
		//free response data
		free(replyMessage->response->data);
		free(replyMessage->response);
		free(replyMessage);
	}
	LOG("reply one server message cost %d s\n",getUptime() - time1);
	return ret;
}

void handleUserMessage(char *msgId,char *userId,char *subId,int type,char *objId,char *data,void *context)
{
    ADD_DEBUG_POINT(1001);
    LOG("msgId: %s userId: %s subId: %s type %d objId: %s data: %s--time=%ld s\n",msgId,userId,subId,type,objId,data,time(NULL));
    struct CloudReplyMessage *replyMessage = transferUserMessage(msgId,userId,subId,type,objId,data);
    ADD_DEBUG_POINT(1002);
    if(context){
        replyMessage->context = context;
        struct CloudUserContext *userContext = (struct CloudUserContext *)context;
        struct TagMessage *freshMessage = (struct TagMessage *)malloc(sizeof(struct TagMessage));
        memset(freshMessage->tag,0,sizeof(freshMessage->tag));
        strcpy(freshMessage->tag,objId);
        sendMessage(userContext->freshMessageList,freshMessage);
        ADD_DEBUG_POINT(1003);
        //send reply msg
        sendMessage(userContext->replyMsgQueue,(void*)replyMessage);
        ADD_DEBUG_POINT(1004);
    }else{
        handleReplyMessage(replyMessage);
    }
    ADD_DEBUG_POINT(1005);
    g_handle_message_count++;
}



#define FETCH_MESSAGE_WAIT_TIMEOUT_SECONDS 5
#define MAX_MEESAGE_FETCH_EACH_LOOP  5
#define CHECK_MESSAGE_TIMEOUT_MAX 3

void *onRemoteMessageThread(void *param)
{
    MesageQueue *serverMsgQueue = (MesageQueue *)param;
    int msgCount = 0;
    int timeoutCount = CHECK_MESSAGE_TIMEOUT_MAX;
    while(g_force_exit_signal == 0)
    {
        ADD_DEBUG_POINT(0);
        //LOG("onRemoteMessageThread----before fetch message\n");
        struct TagMessage* msg = fetchMsg(serverMsgQueue,1,FETCH_MESSAGE_WAIT_TIMEOUT_SECONDS);
        ADD_DEBUG_POINT(1);
        if(msg != NULL){
            struct CloudServerMessage *cloudMsg = (struct CloudServerMessage*)msg;
            struct CloudUserContext *userContext = (struct CloudUserContext *)(cloudMsg->context);
            int32_t time1 = getUptime();
            LOG("begin handle server message\n");
            int64_t pre_handle_count = g_handle_message_count;
            int32_t res = checkMessageFromServer(cloudMsg);
            ADD_DEBUG_POINT(2);
            if(res == -1){
                if(timeoutCount > 0){
                    timeoutCount--;
                    ADD_DEBUG_POINT(3);
                    //send message another once of fail
                    sendMessage(serverMsgQueue,(void*)msg);
                }else{
                    //reset timeout and free message
                    timeoutCount = CHECK_MESSAGE_TIMEOUT_MAX;
                    free(msg);
                }
            }else{
                ADD_DEBUG_POINT(4);
                //reset timeout
                timeoutCount = CHECK_MESSAGE_TIMEOUT_MAX;
                LOG("handle %lld server pub message cost %d s\n",g_handle_message_count - pre_handle_count,getUptime() - time1);
                //check if messagequeue is empty and remote user message list is not empty,then we send a message to check server usermessage next loop
                sem_getvalue(&serverMsgQueue->sem,&msgCount);
                if((g_handle_message_count - pre_handle_count) > 1 && userContext->freshMessageList->count < 3){
                    ADD_DEBUG_POINT(5);
                    //post another once
                    sendMessage(serverMsgQueue,(void*)msg);
                }else{
                    free(msg);
                }
            }
        }
    }
    return NULL;
}

static int32_t
callback_sf_websocket(struct libwebsocket_context *this,
			struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
					       void *user, void *in, size_t len)
{
    // if(reason != LWS_CALLBACK_GET_THREAD_ID) LOG("callback reason %d \n",reason);
    CloudUserContext *userContext = NULL;
    void *userData = NULL;
	switch (reason) {
    case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
        LOG("add---header-len %d-\n",(int)len);
        char **p = (char **)in;
        if (len < 100) return 1;
		appendCommonWbsHeader(*p);
        break;

	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		LOG("callback_sf_websocket: LWS_CALLBACK_CLIENT_ESTABLISHED\n");
        userData = libwebsockets_get_protocol(wsi)->user;
        if(userData != NULL) userContext = (CloudUserContext*)userData;
        if(userContext != NULL){
            userContext->heartbeatUpdateTime = getUptime();
            userContext->status = WS_CLIENT_CONNECT_ESTABLISH;
			userContext->wsi = wsi;
			// checkBindId();
#if SUB_WHEN_CONNECTED
			((callback_init*)(userContext->onInit))(wsi,NULL);
#endif
        }
		break;

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		LOG("LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n");
        userData = libwebsockets_get_protocol(wsi)->user;
        if(userData != NULL) userContext = (CloudUserContext*)userData;
        if(userContext != NULL) userContext->status = WS_CLIENT_CONNECTION_ERR;
        break;

	case LWS_CALLBACK_CLOSED:
        LOG("LWS_CALLBACK_CLOSED\n");
        userData = libwebsockets_get_protocol(wsi)->user;
        if(userData != NULL) userContext = (CloudUserContext*)userData;
        if(userContext != NULL) userContext->status = WS_CLIENT_CONNECTION_CLOSED;
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE_PONG:
		// libwebsocket will send pang to remote server automatic, so just update heartbeatUpdateTime
		LOG("pang data len=%d content='%s'---time=%ld\n", (int32_t )len, (char *)in, time(NULL));
		userData = libwebsockets_get_protocol(wsi)->user;
		if(userData != NULL){
			CloudUserContext *userContext = (CloudUserContext*)userData;
			int32_t lastTime = userContext->heartbeatUpdateTime;
			userContext->heartbeatUpdateTime = getUptime();
		}
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		((char *)in)[len] = '\0';
		LOG("rx %d '%s'---time=%ld\n", (int32_t )len, (char *)in,time(NULL));
        onPayload(wsi,in,len);
		break;

	/* because we are protocols[0] ... */

	case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED:
        LOG("extension request: %s\n",(char*)in);
		break;

	default:
		break;
	}

	return 0;
}

int32_t notifyServerConnected()
{
    int32_t ret = sendCmdToSockServer("RSCH -data {\"action\": 0}",NULL);
    LOG("notifyServerConnected-----result--%d\n",ret);
    return ret;
}

//on receive 'connect' message form cloud websocket server
void onServerConnected(struct libwebsocket *wsi, const char *type)
{
    LOG("onServerConnected\n");
    //notify sock-server cloud client connected
    notifyServerConnected();
    //clearHistoryMessage();
    handleHistoryMessage(wsi);
    //write client connect state to uci
    writeServerConnectState(1);
    subRow(wsi,CLOUD_SUB_CLASS_ROUTER,CLOUD_ROUTERSUB_OBJECT,CLOUD_ROUTER_OBJECT);
}

/* list of supported protocols and callbacks */

static struct libwebsocket_protocols protocols[] = {
	{
        "",
		callback_sf_websocket,
		0,
		8192, //max buffer size per packet
        2,
	},
	{ NULL, NULL, 0, 0 } /* end */
};

void sighandler(int32_t  sig)
{
	g_force_exit_signal = 1;
}

void sighandler_reconn(int32_t sig)
{
	g_force_reconnect_signal = 1;
}

static struct option options[] = {
	{ "help",	no_argument,		NULL, 'h' },
	{ "debug",      required_argument,      NULL, 'd' },
	{ NULL, 0, 0, 0 }
};

int32_t runWsSubClient(struct CloudUserContext *cloudContext)
{
    int32_t ret = 0;
    struct lws_context_creation_info info;
    struct libwebsocket *wsi_client;
    struct libwebsocket_context *context;

    cloudContext->status = WS_CLIENT_OK;
    cloudContext->heartbeatUpdateTime = getUptime();

    //init context create information
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    //access it use libwebsockets_get_protocol(wsi)->user
    info.protocols[0].user = (void *)(cloudContext);
    info.extensions = libwebsocket_get_internal_extensions();
    info.gid = -1;
    info.uid = -1;

    /* create a client websocket using cloud websocket protocl */
    context = libwebsocket_create_context(&info);
    if (context == NULL) {
        LOG("creating libwebsocket context failed\n");
        ret = WS_CLIENT_CONTEXT_FAIL;
        goto bail;
    }
	char wsPath[SF_WS_BUFFER_MAX] = "";
	enum CloudSubClientError err = get_web_socket_path(wsPath);
	if(err != WS_CLIENT_OK) goto bail;

    wsi_client = libwebsocket_client_connect(context,
            SF_WS_ADDRESS,//address
            SF_WS_PORT,//port
            WS_USE_SSL,//ifuse ssl
            wsPath,//location path
            SF_WS_HOST,  //host
            SF_WS_FAKE_ORIGIN, //origin fake
            protocols[0].name,  //protocols
            -1                  //user latest
            );


	if (wsi_client == NULL) {
		LOG("libwebsocket connect failed\n");
		ret = WS_CLIENT_CONNECT_FAIL;
        goto bail;
	}
	LOG("Waiting for connect...\n");

	int count_j = 0;
	while(1){
		if (cloudContext->status == WS_CLIENT_OK){
			count_j++;
			libwebsocket_service(context, WEBSOCKET_SERVICE_INTERVAL_MS);
			if (count_j > 20)
				goto bail;
			usleep(250000);
		}else{
			break;
		}
	}

	int n = 0;
	while (n >= 0 && !g_force_exit_signal) {
		if (cloudContext->status != WS_CLIENT_CONNECT_ESTABLISH){
			ret = cloudContext->status;
			break;
		}

		//check heartbeat
		if (cloudContext->status == WS_CLIENT_CONNECT_ESTABLISH){
			int timeInterval = getUptime() - cloudContext->heartbeatUpdateTime;
			if ((timeInterval >= CLOUD_HEARTBEART_TIME_S) && (timeInterval  < (CLOUD_HEARTBEART_TIME_S + 1))){
				sendPing(wsi_client);
			}else if (timeInterval > (CLOUD_HEARTBEART_TIME_S + 10)){
				ret = WS_CLIENT_HEARTBEAT_TIMEOUT;
				break;
			}
		}

		if(g_force_reconnect_signal){
			g_force_reconnect_signal = 0;
			break;
		}
		n = libwebsocket_service(context, WEBSOCKET_SERVICE_INTERVAL_MS);
		if (n < 0) continue;
	}

bail:
	//wsi_client will be freed in this function
    libwebsocket_context_destroy(context);
    return ret;
}


void *onReplyMessageThread(void *param)
{
    MesageQueue *replyMsgQueue = (MesageQueue *)param;
    while(g_force_exit_signal == 0)
    {
        struct TagMessage* msg = fetchMsg(replyMsgQueue,1,FETCH_MESSAGE_WAIT_TIMEOUT_SECONDS);
		int ret = 0;
		if(msg != NULL){
           struct CloudReplyMessage *replyMessage = (struct CloudReplyMessage*)msg;
           struct CloudUserContext *userContext = (struct CloudUserContext *)(replyMessage->context);
           if(userContext == NULL){
               free(replyMessage);
               continue;
           }
           ret = handleReplyMessage(replyMessage);
		   if (ret < 0){
			   //if handleReplyMessage fail, resend this message with a time interval, or it will resend overspeed.
			   sendMessage(replyMsgQueue,(void*)replyMessage);
			   sleep(FETCH_MESSAGE_WAIT_TIMEOUT_SECONDS);
		   }
           //if handle over now,notify the message fetch thread run at once
           LOG("onReplyMessageThread--fresh-count=%d\n",userContext->freshMessageList->count);
        }
    }
    return NULL;
}


void dotest()
{
	LOG("do test begin");
	postMessageToUser("1001","806023768328634368","806023855599517697",0,"{\"code\": 0,\"msg\": \"OK444\"}","806097517698613249");
}

int main(int argc, char **argv)
{
	int32_t n = 0;
	while (n >= 0) {
		n = getopt_long(argc, argv, "d:hta:r:i:s:b:f:p:v:", options, NULL);
		if (n < 0)
			continue;
		switch (n) {
			case 'd':
				lws_set_log_level(atoi(optarg), NULL);
				break;
            case 'i':
                LOG("routerid:%s\n",optarg);
                sprintf(CLOUD_ROUTER_OBJECT,"%s",optarg);
                break;
            case 's':
                LOG("subid:%s\n",optarg);
                sprintf(CLOUD_ROUTERSUB_OBJECT,"%s",optarg);
                break;
            case 'b':
                LOG("bind:%s\n",optarg);
                CLOUD_BIND = atoi(optarg);
                break;
			case 'f':
				LOG("Ip:%s\n",optarg);
				sprintf(SF_WS_ADDRESS,"%s",optarg);
				break;
			case 'v':
				LOG("version:%s\n",optarg);
				sprintf(SIVERSION,"%s",optarg);
				break;
			case 'p':
				SF_WS_PORT = atoi(optarg);
				LOG("Port:%d\n",SF_WS_PORT);
				break;
			case 't':
				dotest();
				return 0;
            case 'h':
                goto usage;
        }
    }

	sprintf(XCLOUD_REMOTE_ADDR,"https://%s:%d/%s/", SF_WS_ADDRESS, SF_WS_PORT, SIVERSION);
	sprintf(XCLOUD_REMOTE_DATA_ADDR,"https://%s:%d/%s/data/", SF_WS_ADDRESS, SF_WS_PORT, SIVERSION);
	sprintf(CLOUD_REMOTE_FUNCTION_ADDR,"https://%s:%d/%s/cloud", SF_WS_ADDRESS, SF_WS_PORT, SIVERSION);
	sprintf(SF_WS_HOST,"%s:%d", SF_WS_ADDRESS, SF_WS_PORT);
	LOG("remote addr is %s\n",XCLOUD_REMOTE_ADDR);
	LOG("function addr is %s\n",CLOUD_REMOTE_FUNCTION_ADDR);
	LOG("data addr is %s\n",XCLOUD_REMOTE_DATA_ADDR);
    getVersion(CLOUD_CODE_VERSION);

    if(strlen(CLOUD_ROUTER_OBJECT) == 0 || strlen(CLOUD_ROUTERSUB_OBJECT) == 0)
    {
        LOG("some config of router id is empty\n");
//        return 1;
    }

	TokenInit();
	GetToken();

    if(CLOUD_BIND != 1){
        LOG("router not bind");
		// add for update bind info to server when reset with not connect to sever
		// while(checkUserInfo() < 0 ){
			// LOG(" check bind with server fail");
			// sleep(5);
		// }
		return 1;
	}

	signal(SIGINT, sighandler);
	signal(SIGILL,sighandler_reconn);

	curl_global_init(CURL_GLOBAL_ALL);
	init_locks();

	//init message queue to handle remote message
	pthread_t serverMessageThread;
    MesageQueue *serverMsgQueue = createMsgQueue();
    pthread_create(&serverMessageThread,NULL,onRemoteMessageThread,serverMsgQueue);

    //init message queue to handle reply message
    pthread_t replyMessageThread;
    MesageQueue *replyMsgQueue = createMsgQueue();
    pthread_create(&replyMessageThread,NULL,onReplyMessageThread,replyMsgQueue);

    MesageQueue *freshMsgQueue = createMsgQueue();
    //init server connect state
    writeServerConnectState(0);
    //init cloud context
    struct CloudUserContext cloudContext;
    cloudContext.onInit = onServerConnected;
	cloudContext.onUserMessage = handleUserMessage;
    cloudContext.serverMsgQueue = serverMsgQueue;
    cloudContext.replyMsgQueue = replyMsgQueue;
    cloudContext.freshMessageList = freshMsgQueue;
	gContext = &cloudContext;
    int32_t count = 0;
    //any reason cause the subclient exit while trigger another creation
    while(!g_force_exit_signal)
    {
        int32_t ret = runWsSubClient(&cloudContext);
        LOG("runWsSubClient ret %d count=%d\n",ret,count);
        //update server connect state
        writeServerConnectState(0);
        if(g_force_exit_signal) return 1;
        int32_t sleepMs = (ret == WS_CLIENT_HEARTBEAT_TIMEOUT) ? 0 : 5000;
        usleep(sleepMs*1000);
        count++;
    }

    //clean cookie cache in the end
    cleanCookieCache();
    //wait for message thread exit
    pthread_join(serverMessageThread,NULL);
    pthread_join(replyMessageThread,NULL);
    //destroy messgequeue
    destoryMsgQueue(serverMsgQueue);
    destoryMsgQueue(replyMsgQueue);
    destoryMsgQueue(freshMsgQueue);
    //destroy ssl locks
    kill_locks();
usage:
    LOG("please run this in init service\n");
    return 1;
}
