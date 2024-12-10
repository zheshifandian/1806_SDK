/*
 * =====================================================================================
 *
 *       Filename:  message.h
 *
 *    Description:  define message for message queue
 *
 *        Version:  1.0
 *        Created:  2015年09月08日 10时39分49秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert (), robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include "stdlib.h"
#include "local_http.h"
#include "queue.h"
#include <stdint.h>

typedef struct CloudServerMessage{
    struct TagMessage tagMessage;
    char tableName[64];
	char objectId[32];
	char *data;
    int32_t length;
    void *context;
} CloudServerMessage;

typedef struct CloudReplyMessage{
    struct TagMessage tagMessage;
    char messageId[32];
    char userId[32];
    char subId[32];
    char objId[32];
    int32_t type;
    int32_t rtype; // app request type
    struct HttpResonseData *response;
    //message handle count if fail
    int32_t retryCount;
    void *context;
} CloudReplyMessage;

#endif
