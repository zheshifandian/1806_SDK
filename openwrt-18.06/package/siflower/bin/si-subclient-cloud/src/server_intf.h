/*
 * =====================================================================================
 *
 *       Filename:  subscribe.h
 *
 *    Description:  header file for message with server
 *
 *        Version:  1.0
 *        Created:  2016年12月02日 11时53分47秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert (), robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */


#ifndef _SERVER_INTF_H
#define _SERVER_INTF_H

#include <libwebsockets.h>
#include "cloud_common.h"
#include "message.h"

//#define USE_CLOUD 1

#define SF_WS_BUFFER_MAX 256

/*
 * this is specified in the 04 standard, control frames can only have small
 * payload length styles
 */
#define MAX_PING_PAYLOAD 125
#define MAX_MUX_RECURSION 2
#define LWS_SEND_BUFFER_PRE_PADDING (4 + 10 + (2 * MAX_MUX_RECURSION))
#define LWS_SEND_BUFFER_POST_PADDING 4


enum CloudSubClientError{
   //normal status when running
   WS_CLIENT_OK,
   //cloud client connect success
   WS_CLIENT_CONNECT_ESTABLISH,
   //cloud client context create fail
   WS_CLIENT_CONTEXT_FAIL,
   //cloud client connect fail
   WS_CLIENT_CONNECT_FAIL,
   //cloud heartbeat timeout
   WS_CLIENT_HEARTBEAT_TIMEOUT,
   //cloud connection error
   WS_CLIENT_CONNECTION_ERR,
   //cloud connection connection closed
   WS_CLIENT_CONNECTION_CLOSED,
   //cloud client get webscoket from cloud.io fail
   WS_CLIENT_WS_PATH_FAIL
};

typedef void (callback_init)(struct libwebsocket *wsi, const char *type);
typedef void (handle_user_msg)(char *msgId,char *userId,char *subId,int type,char *objId,char *data,void *context);

typedef struct CloudUserContext{
    callback_init *onInit;
	handle_user_msg *onUserMessage;
    //message queue for handle server message
    struct MesageQueue *serverMsgQueue;
    //message queue for send reply message
    struct MesageQueue *replyMsgQueue;
    //message queue to stroe unfinished message
    struct MesageQueue *freshMessageList;
    uint32_t status;
    uint32_t heartbeatUpdateTime;
	struct libwebsocket *wsi;
} CloudUserContext;

#define SUB_WHEN_CONNECTED 1
//skip self signed check when verify ssl
#define WS_USE_SSL 2
//indicate the heartbeat int32_t erval with the websocket server
static const int32_t  CLOUD_HEARTBEART_TIME_S = 60;
extern struct CloudUserContext *gContext;
//static const char *SF_WS_ADDRESS = "120.76.161.56";
//static const char *SF_WS_ADDRESS = "192.168.1.12";
//static const int  SF_WS_PORT = 8090;
//static const char *SF_WS_HOST = "120.76.161.56:8090";
//static const char *SF_WS_HOST = "192.168.1.12:8090";
//static const char *SF_WS_FAKE_ORIGIN = NULL;

//static const char *XCLOUD_ADDRESS = "https://120.76.161.56:8090/routers/data/";
//static const char *XCLOUD_ADDRESS = "https://192.168.1.12:8090/routers/data/";
//static const char *XCLOUD_TABLE_ROUTER = "routers";
//static const char *XCLOUD_TABLE_ROUTERSUB = "routerSub";

//gloable defines
static const char *XCLOUD_WEBSOCKET_PATH = "websocketNew";
enum CloudSubClientError get_web_socket_path(char *path_in);

void appendCommonWbsHeader(char *src_header);
//sub interface
void subRow(struct libwebsocket *wsi,const char *tableName,const char *subId,const char *objectId);
void unsubRow(struct libwebsocket *wsi,const char *tableName,const char *subId,const char *objectId,int hashcode);
//data handler
void onPayload(struct libwebsocket *wsi,void *in, size_t len);
void sendPing(struct libwebsocket *wsi);
int32_t checkMessageFromServer(struct CloudServerMessage *cloudMsg);

#endif
