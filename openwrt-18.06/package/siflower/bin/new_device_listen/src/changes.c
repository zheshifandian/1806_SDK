/*
 * =====================================================================================
 *
 *       Filename:  changes.c
 *
 *    Description:  :
 *
 *        Version:  1.0
 *        Created:  07/25/2015 03:40:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *       Author:  franklin , franklin.wang@siflower.com.cn
 *       Company:  Siflower Communication Tenology Co.,Ltd
         TODO:
         1)wifi's some attribute has not been parsed, txpower(transfer to our definition), mac, sn
         2)router's state has not been parsed
         3)device's info has not been parsed
         4)use etc/config to obtain all tables' objectid
         5)if we are not using mtk7620 and mtk7610e, the wifi parsed code should add parse code according to the special module
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uci.h>
#include <curl/curl.h>

#include "utils.h"
#include "cJSON.h"
#include "changes.h"
#include "http.h"
#include "ssst_request.h"
#include "queue.h"

#define SN_LENGTH   50

MesageQueue *g_changes_msg = NULL;

int32_t InitChangesQueue()
{
    g_changes_msg = createMsgQueue();
    my_assert(g_changes_msg);
    return 0;
}


int32_t do_uci_changes(char *data, char **callback)
{
    int ret = -1;
    ChangesMessage *msg = (ChangesMessage *)malloc(sizeof(ChangesMessage));

    LOG( "strlen(data) = %d", strlen(data));
    LOG( "sizeof(data) = %d", sizeof(data));

    char sn[SN_LENGTH];
    memset(sn, 0, SN_LENGTH);
    if(getSfHardwareConfig("sn",sn) != 0){
        LOG( "[server] get sn from hardware config fail!\n");
        return -1;
    }
    cJSON *root=cJSON_Parse(data);
    cJSON_AddStringToObject(root, "sn", sn);
    msg->data = cJSON_Print(root);
    msg->try_count = 0;
    cJSON_Delete(root);
    sendMessage(g_changes_msg, (void *)msg);
    ret = 0;
    return ret;
}
