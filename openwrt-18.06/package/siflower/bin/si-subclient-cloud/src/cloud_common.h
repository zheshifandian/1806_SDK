/*
 * =====================================================================================
 *
 *       Filename:  cloud_common.h
 *
 *    Description:  define cloud global variants
 *
 *        Version:  1.0
 *        Created:  2015年07月29日 11时02分43秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert (), robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */



#ifndef CLOUD_COMMON_H
#define CLOUD_COMMON_H

#include <stdlib.h>
#include <syslog.h>
#include <stdbool.h>
#include <stdint.h>
#define LOG(X,...) syslog(LOG_CRIT,X,##__VA_ARGS__)
//common value for cloud app
static const char *CLOUD_SUB_CLASS_ROUTER = "RouterSub";

//when query relation type vaue of $param1 from table $param2

static const int CLOUD_HEARTBEAT_ACTION_SEND = 0;
static const int CLOUD_HEARTBEAT_ACTION_REPLY = 1;


extern char SF_WS_ADDRESS[64];
extern char XCLOUD_REMOTE_DATA_ADDR[64];
extern char XCLOUD_REMOTE_ADDR[64];
extern char CLOUD_REMOTE_FUNCTION_ADDR[64];
extern char SF_WS_HOST[64];
extern int SF_WS_PORT;
extern char macaddr[32];
extern char routersn[64];
extern char token[256];
extern struct CloudUserContext *gContext;
/*
 * define cloud type, which you want to use.
 * USE_CLOUD_TYPE 0 : use xcloud alibaba
 * USE_CLOUD_TYPE 1 : use xcloud local
 * USE_CLOUD_TYPE 2 : use cloud
 *
 */
#define USE_CLOUD_TYPE 0

static const char *SF_WS_FAKE_ORIGIN = NULL;
static const char *CLOUD_REMOTE_FUNCTION_REPLY_MESSAGE = "replyToUser";
static const char *REMOTE_REPLY_MSG_ATTR_USERSUB = "usersubid";
static const char *REMOTE_REPLY_MSG_ATTR_PHONE = "phonenumber";

static const char *CLOUD_REMOTE_FUNCTION_GET_USER_MESSAGE = "getUserMessage";
static const char *CLOUD_REMOTE_FUNCTION_CLEAR_ROUTER_MESSAGE = "clearRouterMessage";
static const char *CLOUD_REMOTE_FUNCTION_CHECK_USERINFO = "checkUserInfo";
static const char *XCLOUD_REMOTE_TABLE_USERSUB = "userSub";
static const char *XCLOUD_REMOTE_TABLE_ROUTERSUB = "routerSub";
static const char *XCLOUD_REMOTE_TABLE_ROUTER = "routers";
static const char *CLOUD_REMOTE_FUNCTION_CHECK_BINDMGR = "checkBindMgr";


//define user message type value
#define CLOUD_USER_MESSAGE_TYPE_NORMAL 0
#define CLOUD_USER_MESSAGE_TYPE_SUB_ROUTER 1
#define CLOUD_USER_MESSAGE_TYPE_SUB_DEVICE 2
#define CLOUD_USER_MESSAGE_TYPE_HEARTBEAT 3


//define router message the same as usermessage
//all reply message get type 0
#define CLOUD_ROUTER_MESSAGE_TYPE_NORMAL 0
#define CLOUD_ROUTER_MESSAGE_TYPE_UNBIND 1
#define CLOUD_ROUTER_MESSAGE_TYPE_MANAGER 2
#define CLOUD_ROUTER_MESSAGE_TYPE_HEARTBEAT 3
#define CLOUD_ROUTER_MESSAGE_TYPE_SHARE 4

#define MSG_ID_NONE "0"
#define CLOUD_ROUTER_MESSAGE_CODE_OK 0
#define CLOUD_ROUTER_MESSAGE_RET_SUCCESS "OK"

#define SUBSERVICE_ERROR_NO_EXIST -100


/*
extern char CLOUD_ROUTERSUB_OBJECT[20] = "";
extern char CLOUD_ROUTER_OBJECT[20] = "";
extern char CLOUD_KEY_APK[64] = "";
extern char CLOUD_KEY_REST[64] = "";
extern int CLOUD_BIND = 0;
*/


#ifdef DEBUG_BLOCK
static int gDebugBlockState = 0;
static unsigned long gDebugFetchTime = 0;
static int gDebugUpload = 0;
#define ADD_DEBUG_POINT(x) \
gDebugBlockState = x; \
if(gDebugBlockState == 0) gDebugFetchTime = getUptime()
#else
#define ADD_DEBUG_POINT(x)
#endif

int32_t parseStringFromData(const char *key,void *data,char *value);

int32_t parseIntFromData(const char *key,void *data,int *value);

void getSubstr(char *stbuf,const char*str, unsigned start, unsigned end);

char *my_strstr(char * ps,char *pd);

//bin to hex
void bin2hex(void *src,int length,char *hex);

int32_t sendCmdToSockServer(char *cmd,char *result);

char CLOUD_CODE_VERSION[64];

int32_t formatObjectId(char *in,char *out);

void getVersion(char *version);
//get time from boot
int32_t getUptime();
#endif
