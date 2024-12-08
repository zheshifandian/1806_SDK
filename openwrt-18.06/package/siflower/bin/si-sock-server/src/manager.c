/*
 * =====================================================================================
 *
 *       Filename:  manager.c
 *
 *    Description:  implementation of router manager operations
 *
 *        Version:  1.0
 *        Created:  2015年08月20日 16时28分19秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert , robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#include "manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "local_storage.h"
#include "utils.h"
#include "ssst_request.h"
#include "cJSON.h"


#define SSST_RESPONSE_SUCCESS "{\"result\":\"OK\"}"
#define XCLOUD_RESPONSE_SUCCESS "{\"code\":0,\"msg\":\"ok\",\"data\":null,\"other\":null}"

//define attributes of user manager message
#define SSST_USER_MESSAGE_MANAGER_ACTION "action"
#define SSST_USER_MESSAGE_MANAGER_USERID "userid"
#define SSST_USER_MESSAGE_MANAGER_USERNAME "username"
#define SSST_USER_MESSAGE_MANAGER_PHONE "phonenumber"
#define SSST_USER_MESSAGE_MANAGER_TAG "tag"
#define SSST_USER_MESSAGE_MANAGER_HOSTUSERID "hostuserid"
#define SSST_USER_MESSAGE_MANAGER_ACTION_INVITE 0
#define SSST_USER_MESSAGE_MANAGER_ACTION_REMOVE 1
#define SSST_USER_MESSAGE_MANAGER_ACTION_ACCEPT 2
#define SSST_USER_MESSAGE_MANAGER_ACTION_REJECT 3
#define SSST_USER_MESSAGE_MANAGER_ACTION_EXIT 4
#define SSST_USER_MESSAGE_MANAGER_ACTION_CANCEL_INVITE 5

extern char slversion[64];

void preparePostMessage(char **json_string,int32_t action,char *userid,char *phonenumber,char *username,char *hostuserid,char *tag)
{
    cJSON *json = cJSON_CreateObject();
    char routerId[32];
    getRouterValueUci("routerid",routerId);
    char routerSubId[32];
    getRouterValueUci("subid",routerSubId);
    //prepare post data
    cJSON_AddNumberToObject(json,"action", action);
    cJSON_AddStringToObject(json,"userid",userid);
    cJSON_AddStringToObject(json,"routerid",routerId);
    cJSON_AddStringToObject(json,"subid",routerSubId);
    cJSON_AddStringToObject(json,"phonenumber",phonenumber);
    cJSON_AddStringToObject(json,"username",username);
    cJSON_AddStringToObject(json,"tag",tag);
    cJSON_AddStringToObject(json,"hostuserid",hostuserid);
    cJSON_AddStringToObject(json, "version", slversion);
    *json_string = cJSON_Print(json);
    LOG("prepare message %s------\n",*json_string);
    cJSON_Delete(json);
}

int32_t checkCloudExcute(struct HttpResonseData *response,int32_t ret)
{
    int32_t ret1 = 0;
    LOG("--xcloud response=%s\n",(response->size > 0) ? (char*)response->data : "");
    if(ret != CURLE_OK || response->size == 0){
        ret1 = -1;
    }
    else if(strcmp((char*)response->data, XCLOUD_RESPONSE_SUCCESS) != 0){
        LOG("CLOUD PROGRAM EXCUTE FAILED------\n");
        ret1 = -1;
    }
    else{
        ret1 = 0;
    }
    return ret1;
}

int32_t runCloudcode(int32_t action, char *userid,char *phonenumber,char *username,char *hostuserid,char *tag){
    int32_t ret = 0;
    char *json_string = NULL;
    char url[256] = "";
    struct HttpResonseData response;
    response.size = 0;
    preparePostMessage(&json_string,action,userid,phonenumber,username,hostuserid,tag);
    sprintf(url,"%s/%s",XCLOUD_REMOTE_FUNCTION_ADDR,XCLOUD_REMOTE_FUNCTION_SHARE_ROUTER);
    int32_t ret1 = postHttps(url,json_string,&response);
    ret = (checkCloudExcute(&response,ret1));
    if(response.size > 0) free(response.data);
    free(json_string);
    return ret;
}


int32_t onRemoveManager(int32_t action,char *userid,char *phonenumber,char *username,char *hostuserid,char *error)
{
    if(!userid || strlen(userid) == 0){
        sprintf(error,"%s","empty user id\n");
        return -1;
    }
    //check local
    int32_t ret = 0;
    char *tag = "";
    ret = runCloudcode(action,userid,phonenumber,username,hostuserid,tag);
    return ret;
}

int32_t invitUser(char *userid,char *phonenumber,char *username,char *hostuserid,char *error)
{
    LOG("invitUser------\n");
    if(!userid && !phonenumber){
        sprintf(error,"%s","empty userId and phone\n");
        return -1;
    }

    int32_t ret = 0;
    int32_t action = 0;
    char tag[32] = "";
    createUniqeTag(tag);
    LOG("create unique tag--%s\n",tag);
    ret = runCloudcode(action,userid,phonenumber,username,hostuserid,tag);
    if(ret == 0){
        insertUnFinishedInvitMsg(tag,userid,phonenumber,username);
    }
    return ret;
}

int32_t onUserInvitReponse(int32_t action,char *tag,char *userid,char *phonenumber,char *username,char *error)
{
    LOG("onUserInvitReponse------\n");
    //first check tag from local storage
    if(checkUnFinishedInvitMsg(tag,userid,phonenumber,username,0) != 0){
        sprintf(error,"%s","incognizant message tag");
        return -1;
    }
    LOG("checkUnFinishedInvitMsg exist------\n");
    int32_t ret = 0;
    char *hostuserid = userid;
    ret = runCloudcode(action,userid,phonenumber,username,hostuserid,tag);
    if(ret == 0){
        deleteUnFinishedInvitMsg(tag,userid,phonenumber,username);
    }
    return ret;
}

int32_t cancelInvite(char *userid,char *phonenumber,char *username,char *hostuserid,char *error)
{
    LOG("cancelInvite------\n");
    int32_t ret = 0;
    int32_t action = 5;
    char *tag = "";
    ret = runCloudcode(action,userid,phonenumber,username,hostuserid,tag);
    if(ret == 0){
        deleteTagMsg(userid,phonenumber);
    }
    return ret;
}


int32_t handleUserManagerAction(char *data,char *error)
{
    LOG("handleUserManagerAction--data=%s\n",data);
    int32_t res = -1;
    //1 parse response
    cJSON *json;
    json = cJSON_Parse(data);
    if(!json) goto clean;
    //get action
    cJSON *object = cJSON_GetObjectItem(json, SSST_USER_MESSAGE_MANAGER_ACTION);
    if (!object || object->type != cJSON_Number) goto clean;
    int action = object->valueint;
    //get userid
    object = cJSON_GetObjectItem(json,SSST_USER_MESSAGE_MANAGER_USERID);
    if(!object || object->type != cJSON_String) goto clean;
    char destUserId[20] = "";
    sprintf(destUserId,"%s",(char*)object->valuestring);
    //get username
    object = cJSON_GetObjectItem(json,SSST_USER_MESSAGE_MANAGER_USERNAME);
    if(!object || object->type != cJSON_String) goto clean;
    char destUserName[64] = "";
    sprintf(destUserName,"%s",(char*)object->valuestring);
    //get tag
    object = cJSON_GetObjectItem(json,SSST_USER_MESSAGE_MANAGER_TAG);
    if(!object || object->type != cJSON_String) goto clean;
    char tag[32] = "";
    sprintf(tag,"%s",(char*)object->valuestring);
    //get phonenumber
    object = cJSON_GetObjectItem(json,SSST_USER_MESSAGE_MANAGER_PHONE);
    if(!object || object->type != cJSON_String) goto clean;
    char phonenumber[20] = "";
    sprintf(phonenumber,"%s",(char*)object->valuestring);
    //get hostuserid
    object = cJSON_GetObjectItem(json,SSST_USER_MESSAGE_MANAGER_HOSTUSERID);
    if(!object || object->type != cJSON_String) goto clean;
    char hostuserid[20] = "";
    sprintf(hostuserid,"%s",(char*)object->valuestring);
    LOG("handle user manager action--%d destUserId %s destUserName %s phone %s\n",action,destUserId,destUserName,phonenumber);
    cJSON_Delete(json);
    json = NULL;
    switch(action){
        case SSST_USER_MESSAGE_MANAGER_ACTION_INVITE:
            {
                return invitUser(destUserId,phonenumber,destUserName,hostuserid,error);
                break;
            }
        case SSST_USER_MESSAGE_MANAGER_ACTION_REMOVE:
            {
                return onRemoveManager(1,destUserId,phonenumber,destUserName,hostuserid,error);
                break;
            }
        case SSST_USER_MESSAGE_MANAGER_ACTION_ACCEPT:
            {
                return onUserInvitReponse(2,tag,destUserId,phonenumber,destUserName,error);
                break;
            }
        case SSST_USER_MESSAGE_MANAGER_ACTION_REJECT:
            {
                return onUserInvitReponse(3,tag,destUserId,phonenumber,destUserName,error);
                break;
            }
        case SSST_USER_MESSAGE_MANAGER_ACTION_EXIT:
            {
                return onRemoveManager(4,destUserId,phonenumber,destUserName,hostuserid,error);
                break;
            }
        case SSST_USER_MESSAGE_MANAGER_ACTION_CANCEL_INVITE:
            {
                return cancelInvite(destUserId,phonenumber,destUserName,hostuserid,error);
            }
        default:
            {
                sprintf(error,"%s","unknown action of usermessage about user\n");
                return -1;
            }
    }
clean:
    sprintf(error,"%s","parse invite message data fail\n");
    if(json) cJSON_Delete(json);
    return res;

}
