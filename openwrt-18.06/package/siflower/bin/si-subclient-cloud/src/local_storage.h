/*
 * =====================================================================================
 *
 *       Filename:  local_storage.h
 *
 *    Description:  local storage for server to cache remote objects of unfinished business
 *
 *        Version:  1.0
 *        Created:  2015年08月19日 11时19分32秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert (), robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#ifndef LOCAL_STORAGE_H
#define LOCAL_STORAGE_H
#include <stdlib.h>
#include <stdint.h>

//store unfinished business to local storage
void insertUnFinishedInvitMsg(char *tag,char *destUserId,char *phone,char *destUserName,char *destUserSub,char *time);

//checkunfinished business of local storage
//return 0 if check success
int32_t checkUnFinishedInvitMsg(char *tag,char *destUserId,char *phone,char *destUserName,char *destUserSub);

//0--diconnect 1 --connect
void writeServerConnectState(int32_t connected);

//get config from siwifi
int32_t getSfHardwareConfig(const char *key,char *retBuffer);

void get_router_macaddr(char *mac_in);
#endif
