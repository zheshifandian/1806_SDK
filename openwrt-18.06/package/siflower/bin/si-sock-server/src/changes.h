/*
 * =====================================================================================
 *
 *       Filename:  changes.h
 *
 *    Description: when some configs have been changed, we should upload the changes to the
                    private cloud storage,so our remote users could see or be notified this
                    information
 *        Version:  1.0
 *        Created:  07/25/2015 03:40:13 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  franklin (), franklin.wang@siflower.com.cn
 *        Company:  Siflower Communication Tenology Co.,Ltd
 *
 * =====================================================================================
 */
#ifndef  __SOCK_SERVER_CHANGES_H__
#define __SOCK_SERVER_CHANGES_H__

#include "queue.h"

typedef struct{
    TagMessage tag;
    char *data;
    int try_count;
}ChangesMessage;


/********************global var**********************/
//extern MesageQueue *g_changes_msg = NULL;


/*
func:
prams:
return:
*/
int32_t do_uci_changes(char *data, char **ret);
int32_t InitChangesQueue(void);
void *update_process(void *args);
#endif
