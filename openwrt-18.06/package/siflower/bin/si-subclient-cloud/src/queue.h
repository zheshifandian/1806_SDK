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
#include <pthread.h>

typedef struct TagMessage{
    //unique tag now
    char tag[32];
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

//return 0 if find success
extern int deleteMsgByTag(MesageQueue *msgQueue,char *tag);
#endif
