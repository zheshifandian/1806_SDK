/*
 * =====================================================================================
 *
 *       Filename:  local_stub.h
 *
 *    Description:  i
 *
 *        Version:  1.0
 *        Created:  2015年07月29日 10时34分04秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert (), robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#ifndef LOCAL_STUB_H
#define LOCAL_STUB_H

#include "message.h"
#include <libwebsockets.h>

struct CloudReplyMessage* transferUserMessage(char *msgId,char *userId,char *subId,int type,char *objId,char *msgData);

void forwardResultToUser(char *msgId,char *userId,char *subId,int type,char *ret);

void finishHandleMessage(char *msgId,char *objId);

void clearHistoryMessage();
void handleHistoryMessage(struct libwebsocket *wsi);

int32_t postMessageToUser(char *msgId,char *userId,char *subId,int type,char *messageData,char *originMessageObject);

#endif
