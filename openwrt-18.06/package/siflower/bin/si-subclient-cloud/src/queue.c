/*
 * =====================================================================================
 *
 *       Filename:  queue.c
 *
 *    Description:  implementation of message queue
 *
 *        Version:  1.0
 *        Created:  2015年08月24日 13时57分18秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert , robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */


#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "queue.h"
#include "cloud_common.h"


MesageQueue * createMsgQueue(unsigned int msgMax,unsigned int msgSize)
{
    MesageQueue *newQueue = (MesageQueue *)malloc(sizeof(MesageQueue));
    newQueue->count = 0;
    newQueue->header = NULL;
    newQueue->tail = NULL;
    pthread_mutex_init(&newQueue->lock,NULL);
    sem_init (&newQueue->sem, 0, 0);
    return newQueue;
}

int destoryMsgQueue(MesageQueue *msgQueue)
{
    int ret = -1;
    if(msgQueue)
    {
        //destroy messages
        pthread_mutex_lock(&msgQueue->lock);
        struct TagMessage *msg = msgQueue->header;
        while(msg){
            struct TagMessage *next = msg->next;
            free(msg);
            msg = next;
        }
        msgQueue->header = NULL;
        msgQueue->tail = NULL;
        ret = sem_destroy(&msgQueue->sem);
        pthread_mutex_unlock(&msgQueue->lock);
        ret |= pthread_mutex_destroy(&msgQueue->lock);
        free(msgQueue);
    }
    return ret;
}

int sendMessage(MesageQueue *msgQueue,void *msg)
{
    int ret = -1;
    pthread_mutex_lock(&msgQueue->lock);
    struct TagMessage *tagMsg = (struct TagMessage *)msg;
    tagMsg->next = NULL;
    if(!msgQueue->tail){
        tagMsg->next = NULL;
        msgQueue->tail = tagMsg;
        msgQueue->header = tagMsg;
    }else{
        struct TagMessage *tail = msgQueue->tail;
        tail->next = tagMsg;
        msgQueue->tail = tagMsg;
    }
    msgQueue->count++;
    ret = sem_post(&msgQueue->sem);
    pthread_mutex_unlock(&msgQueue->lock);
    return ret;
}

struct TagMessage* fetchMsg(MesageQueue *msgQueue,int blocked,int timeouts)
{
    struct TagMessage* ret = NULL;
    if(blocked)
    {
        if(timeouts == -1){
            sem_wait(&msgQueue->sem);
        }else{
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += timeouts;
            sem_timedwait(&msgQueue->sem, &ts);
        }
    }
    else
    {
        sem_trywait(&msgQueue->sem);
    }
    pthread_mutex_lock (&msgQueue->lock);
    if(msgQueue->header == NULL){
        ret = NULL;
    }else{
        ret = msgQueue->header;
        msgQueue->header = ret->next;
        if(ret == msgQueue->tail) msgQueue->tail = NULL;
        msgQueue->count--;
    }
    pthread_mutex_unlock (&msgQueue->lock);
    return ret;
}

int deleteMsgByTag(MesageQueue *msgQueue,char *tag)
{
    int ret = -1;
    pthread_mutex_lock (&msgQueue->lock);
    if(msgQueue->header == NULL){
        ret = -1;
    }else{
        struct TagMessage* header = msgQueue->header;
        struct TagMessage* pre = header;
        while(header){
            if(!strcmp(header->tag,tag)){
                //at header
                if(header == pre){
                    msgQueue->header = header->next;
                    if(msgQueue->tail == header) msgQueue->tail = NULL;
                    free(header);
                }else{
                    pre->next = header->next;
                    if(msgQueue->tail == header) msgQueue->tail = pre;
                    free(header);
                }
                msgQueue->count--;
                ret = 0;
                break;
            }else{
                pre = header;
                header = header->next;
            }
        }
    }
    pthread_mutex_unlock (&msgQueue->lock);
    return ret;
}
