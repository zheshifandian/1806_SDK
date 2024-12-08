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
#include <uci.h>
struct SsstObjectId{
    char objectId[20];
};

//store unfinished business to local storage
void insertUnFinishedInvitMsg(char *tag,char *destUserId,char *phone,char *destUserName);

//checkunfinished business of local storage
//return 0 if check success
int32_t checkUnFinishedInvitMsg(char *tag,char *destUserId,char *phone,char *destUserName,int32_t delete);

int32_t deleteUnFinishedInvitMsg(char *tag,char *destUserId,char *phone,char *destUserName);

//bin to hex
void bin2hex(void *src,int length,char *hex);

void createUniqeTag(char *tag);

void deleteTagMsg(char *userid, char *phonenumber);

int32_t getRouterValueUci(const char *key,char *retBuffer);

int32_t setRouterValueUci(const char *key,char *value);

int32_t getServerConnectedState();

int32_t saveRouterManager(const char *managerObjectId);

int32_t delRouterManager(const char *managerObjectId);

int32_t getRouterManagerList(int32_t *num,struct SsstObjectId **objects);

int32_t uciDeleteOption(struct uci_context *ctx,struct uci_package *p,struct uci_section *section,const char *option);

void uciSetValue(struct uci_context *ctx,struct uci_ptr *ptr,const char *key,char *retBuffer);

int32_t setRouterValueToUci(char *utilssectionName,const char *key,char *retBuffer);
#endif
