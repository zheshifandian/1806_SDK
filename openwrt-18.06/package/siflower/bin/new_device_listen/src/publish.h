/*
 * =====================================================================================
 *
 *       Filename:  publish.h
 *
 *    Description:  publish important status change information to app
 *
 *        Version:  1.0
 *        Created:  2015年08月31日 14时25分22秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert (), robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#ifndef PUBLISH_H
#define PUBLISH_H

#include <stdlib.h>
#include <stdint.h>


#define ROUTER_EVENT_TYPE_STATUS_CHANGE 0
#define ROUTER_EVENT_TYPE_ONLINE 1
#define ROUTER_EVENT_STORAGE_CHANGE 2

// error, siwifi service would fail
#define DATA_SYNC_PARSE_PARAM_ERROR      2
#define DATA_SYNC_ALREADY_HAVE_MAC       403
#define DATA_SYNC_FAIL_CREATE_SN        -1

// get new sn and should unbind
#define DATA_SYNC_NO_SN_MAC				410

// local should be unbind
#define DATA_SYNC_SERVER_UNBIND         407
#define DATA_SYNC_BIND_DIFERENT         408

//update mgr list
#define DATA_SYNC_MGR_LIST_SYNC         409

//success  do noting
#define DATA_SYNC_SUCCESS                0
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

typedef struct UserObjectId{
    char objectId[32];
} UserObjectId;

typedef struct RouterEvent{
    int32_t userNum;
    struct UserObjectId *userlist;
    int32_t type;
    char *data;
    char alert[256];
}RouterEvent;

void prepareStatusChangeData(int32_t action,char *out);

void prepareOnlineChangeData(int32_t strange,char *name,char *mac,char *out);

void prepareStorageChangeData(int32_t status, char *out);

/*publish hot plug event to app user*/
int32_t publishEventToUser(int32_t type,char *eventData);

/*publish router event to remote server*/
int32_t publishRouterEvent(int32_t type,char *eventData);

/*
publish device online event to managers and binder.
@param strange   0---online alert 1---strangers connect
@param name      name of the device
@param mac       mac address of the device
@return          Boolean whether operation succeeded
*/
int32_t publishDeviceOnlineEvent(int32_t strange,char *name,char *mac);

/*
publish router status change event to managers and binder.
@param action    0---online 1--offline
@return          Boolean whether operation succeeded
*/
int32_t publishRtStatusChangeEvent(int32_t action);

#endif
