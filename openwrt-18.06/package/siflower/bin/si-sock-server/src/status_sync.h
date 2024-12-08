/*
 * =====================================================================================
 *
 *       Filename:  status_sync.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  07/27/2015 03:58:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  franklin (), franklin.wang@siflower.com.cn
 *        Company:  Siflower Communication Tenology Co.,Ltd
 *
 * =====================================================================================
 */
#ifndef __SOCK_SERVER_STATUSSYNC_H__
#define __SOCK_SERVER_STATUSSYNC_H__

#include "queue.h"
#define SYSTEM_EVENT_ACTION_STR "action"
#define SYSTEM_EVENT_ACTION_REMOTE_SERVER_CONNECTED 0
#define SYSTEM_EVENT_ACTION_UPGRADE 1
#define SYSTEM_EVENT_ACTION_REBOOT 2
#define SYSTEM_EVENT_ACTION_RESET 3
#define SYSTEM_EVENT_ACTION_UPGRADE_DONE 4


//defined for sub event
#define SUB_EVENT_ACTION_STR "action"
#define SUB_EVENT_TYPE_STR "type"
#define SUB_EVENT_USERID "userid"
#define SUB_EVENT_DATA "data"

#define SUB_EVENT_TYPE_ROUTERSTATE 0
#define SUB_EVENT_TYPE_DEVICESTATE 1

#define SYNC_EVENT                      1001
#define UPDATE_LOCAL_INFO_EVENT         1002
#define GET_UPGRADE_PROCESS_EVENT       1003


#define DEVICE_INFO_TAG             "deviceinfo"
#define ROUTER_STATE_TAG            "routerstate"
#define GET_UPGRADE_PROCESS_TAG     "getupgradeprocess"

#define SN_LENGTH   50

typedef struct _user_info{
	char ip[16];
	int32_t port;
	struct _user_info *next;
}user_info;

struct user_info_list{
	int32_t list_len;
	user_info *head;
	user_info *tail;
};

typedef struct{
    struct TagMessage tagMessage;
    int32_t startFlag;
    time_t last_stamp;
    char userid[32];
    char usersubid[32];
    char msgid[32];
    void *data;
    int  mode;
    int  ota_type;
    long firstEnqueueTime;
}periodGetUpgradeProcessEvent_t;

typedef struct BaseSubRequest{
    struct TagMessage tagMessage;
    long hearbeatUpdateTime;
    //to be used in future to mark Indicator of user interest
    int32_t interest;
}BaseSubRequest;

/*type define for local sub */
typedef struct RouterStateSubRequest{
    struct BaseSubRequest baseRequest;
} RouterStateSubRequest;

typedef struct DeviceStateSubRequest{
    struct BaseSubRequest baseRequest;
    //buffer to store value of devicelist when user require update certain devices
    char devicelist[256];
}DeviceStateSubRequest;

typedef struct UnresolvedEvent{
    struct TagMessage tagMessage;
    int32_t type;
    char *data;
    long firstEnqueueTime;
}UnresolvedEvent;

typedef struct UpdateLocalInfoToUciEvent{
    struct UnresolvedEvent unresolvedEvent;
    long lastUpdateTime;
    int32_t enable;
}UpdateLocalInfoToUciEvent;

typedef struct TotallySyncEvent{
    struct UnresolvedEvent unresolvedEvent;
}TotallySyncEvent;

typedef struct StatusSyncContext{
    //container for state subscribe request
    struct MesageQueue *stateSubRequestQueue;
    struct MesageQueue *unresolvedEventQueue;
    pthread_mutex_t g_sync_mutex;
    pthread_cond_t g_sync_cond;
    long lastHeartbeatSendTime;
}StatusSyncContext;


/*
func:
params:
return:
 */
void do_status_sync(char *data, char **ret);

void init_sync_context();
void destroy_sync_context();

void on_system_event(char *data, char **callback);

void on_sub_event(char *data, char **callback);

int32_t do_save_user_info(char *data, char **callback);

void prepareCallbackData(char **callback,char *ret,char *reason);
#endif
