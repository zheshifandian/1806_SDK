/*
 * =====================================================================================
 *
 *       Filename:  queue.h
 *
 *    Description:  message queue 
 *
 *        Version:  1.0
 *        Created:  2015年08月24日 13时56分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert (), robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <semaphore.h>

typedef struct TagMessage{
    //unique tag now
    char tag[32];
    //special type
    int type;
    struct TagMessage *next;
} TagMessage;

typedef struct MesageQueue{        
    pthread_mutex_t lock;
    sem_t  sem;
    int count;
    struct TagMessage *header;
    struct TagMessage *tail;
}MesageQueue;

extern MesageQueue *createMsgQueue();

extern int destoryMsgQueue(MesageQueue *msgQueue);

extern int sendMessage(MesageQueue *msgQueue,void *msg);

extern struct TagMessage* fetchMsg(MesageQueue *msgQueue,int blocked,int timeouts);


//fetch one message by special tag&type defined in TagMessage
extern int fetchMsgByTagAndType(MesageQueue *msgQueue,char *tag,int type,struct TagMessage **findMessage);

//return 0 if find success
extern int deleteMsgByTag(MesageQueue *msgQueue,char *tag);

//return 0 if find success
extern int deleteMsgByTagAndType(MesageQueue *msgQueue,char *tag,int type);

//get message count by special type defined in TagMessage
extern int getMsgCountByType(MesageQueue *msgQueue,int type);

//get message tag list in comma delimiter
//buffer size of testOut should enough by count*sizeof(tag)
extern void printMessageTagComma(MesageQueue *msgQueue,char *textOut,int size);

#endif
