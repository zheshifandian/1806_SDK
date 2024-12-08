/*
 * =====================================================================================
 *
 *       Filename:  ssst_request.h
 *
 *    Description:  common util for ssst request
 *
 *        Version:  1.0
 *        Created:  2015年09月01日 09时39分28秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert (), robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#ifndef SSST_REQUEST_H
#define SSST_REQUEST_H

#include <stdlib.h>
#include <stdint.h>
#include "common.h"

#define SSST_REMOTE_FUNCTION_REPLY_MESSAGE "replyMessageToUser"

#define XCLOUD_REMOTE_FUNCTION_UNBIND "unbindRouter"
#define XCLOUD_REMOTE_FUNCTION_BIND "bindRouter"
#define XCLOUD_REMOTE_FUNCTION_SHARE_ROUTER "shareRouter"
#define XCLOUD_REMOTE_FUNCTION_PUBLISH_EVENT "publishEvent"
#define XCLOUD_REMOTE_TABLE_USERSUB "userSub"
#define XCLOUD_REMOTE_FUNCTION_GET_SN "getSn"
#define XCLOUD_REMOTE_FUNCTION_ROUTER_DATA_SYNC "routerDataSync"
#define XCLOUD_REMOTE_FUNCTION_ADD_ROUTER_USER "addRouterUser"

extern char XCLOUD_REMOTE_FUNCTION_ADDR[];
extern char XCLOUD_REMOTE_ADDR[];
extern char XCLOUD_REMOTE_DATA_ADDR[];

size_t OnHttpReponseData(void* buffer, size_t size, size_t nmemb, void* lpVoid);

int32_t postHttps(char *url,void *data,void *response);

int32_t postHttp(char *url,void *data,void *response);

#endif
