/*
 * =====================================================================================
 *
 *       Filename:  object_save_file.c
 *
 *    Description:  implementation of object file save
 *
 *        Version:  1.0
 *        Created:  2015年08月12日 14时04分05秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert , robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#include "object_save_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "cJSON.h"
#include "utils.h"

#define OBJECT_FILE_PATH "/root/siwifi_object"


int32_t has_binded_file()
{
    syslog(LOG_CRIT, "[server]%s\n",__func__);
    FILE *file = fopen(OBJECT_FILE_PATH, "r");
    if(file){
        long size = 0;
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        fseek(file, 0, SEEK_SET);
        if(size <= 0){
            fclose(file);
            return -1;
        }
        char *buffer = (char *)malloc(size);
        if(fread(buffer, 1, size, file) != size){
            syslog(LOG_CRIT, "[server] can not read empty the file\n");
            fclose(file);
            free(buffer);
            return -1;
        }
        fclose(file);
        cJSON *root = cJSON_Parse(buffer);
        free(buffer);
        if(!root){
            syslog(LOG_CRIT, "[server] can not parse the buffer(%s) to json!\n", buffer);
            return -1;
        }
        cJSON *binded = cJSON_GetObjectItem(root,"binded");
        if(binded && binded->valueint > 0){
            cJSON_Delete(root);
            return 0;
        }
        cJSON_Delete(root);
        return -1;
    }else{
        syslog(LOG_CRIT,"[server] can not open the file %s!\n", OBJECT_FILE_PATH);
        return -1;
    }
}

int32_t has_inited_file(router_table *rt, router_sub_table *subt, router_state_table *rst,
                wireless_table **wt, int32_t *wnum, device_table **dst, int32_t *dnum)
{
    FILE *file = fopen(OBJECT_FILE_PATH, "r");
    wireless_table *warray = NULL;
    int32_t wifi_num = 0;
    device_table *darray = NULL;
    int32_t device_num = 0;
    int binded = 0;
    if(file){
        long size = 0;
        int32_t i = 0;
        int32_t j = 0;
        int32_t ret = -1;
        cJSON *root = NULL;
        char *buffer = NULL;
        //check the file has some data already
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        fseek(file, 0, SEEK_SET);
        if(size <= 0){
            syslog(LOG_CRIT, "[server] file not exist!\n");
            goto DONE;
        }
        buffer = (char *)malloc(size);
        if(fread(buffer, 1, size, file) != size){
            syslog(LOG_CRIT, "[server] can not read empty the file\n");
            goto DONE;
        }
        root = cJSON_Parse(buffer);
        if(!root){
            syslog(LOG_CRIT, "[server] can not parse the buffer(%s) to json!\n", buffer);
            goto DONE;
        }
        for(i = 0; i < cJSON_GetArraySize(root); i++){
            cJSON *subitem = cJSON_GetArrayItem(root, i);
            if(!subitem->string)
                continue;
            if(!strcmp(subitem->string, "binded")){
                binded = subitem->valueint;
                syslog(LOG_CRIT, "[server] binded : %d\n", binded);
            }else if(!strcmp(subitem->string, "router")){
                strcpy(rt->objectid, subitem->valuestring);
                syslog(LOG_CRIT, "[server] rt->objectid : %s\n", rt->objectid);
            }else if(!strcmp(subitem->string, "routersub")){
                strcpy(subt->objectid, subitem->valuestring);
                syslog(LOG_CRIT, "[server] subt->objectid : %s\n", subt->objectid);
            }else if(!strcmp(subitem->string, "routerstate")){
                strcpy(rst->objectid, subitem->valuestring);
                syslog(LOG_CRIT, "[server] rst->objectid : %s\n", rst->objectid);
            }else if(!strcmp(subitem->string, "wireless")){
                if(warray)
                    free(warray);
                wifi_num = cJSON_GetArraySize(subitem);
                warray = (wireless_table *)malloc(wifi_num * sizeof(wireless_table));
                memset((void *)warray, 0 , wifi_num * sizeof(wireless_table));
                for(j = 0; j < wifi_num; j++){
                    cJSON *subsubitem = cJSON_GetArrayItem(subitem, j);
                    my_assert(!strcmp(subsubitem->string, "__object"));
                    strcpy(warray[j].objectid, subsubitem->valuestring);
                    syslog(LOG_CRIT,"[server] wireless[%d] objectid : %s\n", j, warray[j].objectid);
                }
            }else if(!strcmp(subitem->string, "devices")){
                if(darray)
                    free(darray);
                device_num = cJSON_GetArraySize(subitem);
                darray = (device_table *)malloc(device_num * sizeof(device_table));
                memset((void *)darray, 0 , device_num * sizeof(device_table));
                for(j = 0;j < device_num; j++){
                    cJSON *subsubitem = cJSON_GetArrayItem(subitem, j);
                    my_assert(!strcmp(subsubitem->string, "__object"));
                    strcpy(darray[j].objectid, subsubitem->valuestring);
                    syslog(LOG_CRIT,"[server] device[%d] objectid : %s\n", j, darray[j].objectid);
                }
            }else{
                syslog(LOG_CRIT, "[server] unknown node : %s\n", subitem->string);
            }
        }
        *wt = warray;
        *wnum = wifi_num;
        *dst = darray;
        *dnum = device_num;
        ret = 0;
DONE:
        if(file)
            fclose(file);
        if(buffer)
            free(buffer);
        if(root)
            cJSON_Delete(root);
        return ret;
    }else{
        syslog(LOG_CRIT, "[server] can not open the bind file : %s\n", OBJECT_FILE_PATH);

        return -1;
    }

}

int32_t delete_bind_file(int32_t all)
{
    if(all){
        //delete the file
        char cmd[256];
        sprintf(cmd,"rm %s", OBJECT_FILE_PATH);
        system(cmd);
        return 0;
    }else{
        FILE *file = fopen(OBJECT_FILE_PATH, "r+");
        if(file){
            long size = 0;
            char *buffer = NULL;
            cJSON *root = NULL;
            int32_t ret = -1;
            fseek(file, 0, SEEK_END);
            size = ftell(file);
            fseek(file, 0, SEEK_SET);
            if(size <= 0){
                fclose(file);
                return ret;
            }
            buffer = (char *)malloc(size);
            if(fread(buffer, 1, size, file) != size){
                syslog(LOG_CRIT, "[server] can not read empty the file\n");
                goto DONE;
            }
            root = cJSON_Parse(buffer);
            if(!root){
                syslog(LOG_CRIT, "[server] can not parse the buffer(%s) to json!\n", buffer);
                goto DONE;
            }
            cJSON *binded = cJSON_GetObjectItem(root,"binded");
            int32_t bind_value = -1;
            if(!binded || cJSON_GetValueInt(binded, &bind_value) < 0){
                syslog(LOG_CRIT, "[server] can not get the binded value from %s\n", buffer);
                goto DONE;
            }
            if(bind_value){
                binded->valueint = 0;
                binded->valuedouble = 0;
                char *render = cJSON_Print(root);
                if(render){
                    fseek(file, 0, SEEK_SET);
                    if(fwrite(render, 1, strlen(render), file) != strlen(render)){
                        syslog(LOG_CRIT, "[server] can not write %s to file\n", render);
                    }
                    free(render);
                    ret = 0;
                }
            }
DONE:
            if(file)
                fclose(file);
            if(buffer)
                free(buffer);
            if(root)
                cJSON_Delete(root);
            return ret;
        }else{
            syslog(LOG_CRIT,"[server] can not open the file %s!\n", OBJECT_FILE_PATH);
            return -1;
        }
        return 0;
    }

}

int32_t save_bind_file(router_table *rt, router_sub_table *subt, router_state_table *rst,
                wireless_table *wt, int32_t wnum, device_table *dst, int32_t dnum,char *binder_id)
{
    FILE *file = fopen(OBJECT_FILE_PATH, "w");
    if(file){
        syslog(LOG_CRIT, "[server] open the bind file : %s\n", OBJECT_FILE_PATH);
        int32_t i = 0;
        //check the file has some data already
        cJSON *root=cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "binded", 1);
        cJSON_AddStringToObject(root, "binder_id", binder_id);
        cJSON_AddStringToObject(root, "router", rt->objectid);
        cJSON_AddStringToObject(root, "routersub", subt->objectid);
        cJSON_AddStringToObject(root, "routerstate", rst->objectid);
        cJSON *wireless=cJSON_CreateObject();
        cJSON_AddItemToObject(root,"wireless", wireless);
        for(i = 0; i < wnum; i++){
            cJSON_AddStringToObject(wireless, "__object", wt[i].objectid);
        }
        cJSON *devices=cJSON_CreateObject();
        cJSON_AddItemToObject(root,"devices", devices);
        for(i = 0; i < dnum; i++){
            cJSON_AddStringToObject(devices, "__object", dst[i].objectid);
        }
        char *all_data = cJSON_Print(root);
        cJSON_Delete(root);
        if(all_data){
            if(fwrite(all_data, 1, strlen(all_data), file) != strlen(all_data))
                syslog(LOG_CRIT, "[server] can not write %d bytes to the file(%s)\n", strlen(all_data), OBJECT_FILE_PATH);
            fclose(file);
            free(all_data);
        }else{
            fclose(file);
        }
        return 0;
    }else{
        syslog(LOG_CRIT, "[server] can not open the bind file : %s\n", OBJECT_FILE_PATH);
        return -1;
    }
}

int32_t get_table_file(char *object, int32_t table, int32_t order)
{
    FILE *file = fopen(OBJECT_FILE_PATH, "r");
    if(file){
        long size = 0;
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        if(size <= 0){
            fclose(file);
            return -1;
        }
        fseek(file, 0, SEEK_SET);
        char *buffer = (char *)malloc(size);
        if(fread(buffer, 1, size, file) != size){
            syslog(LOG_CRIT, "[server] can not read empty the file\n");
            fclose(file);
            free(buffer);
            return -1;
        }
        fclose(file);
        cJSON *root = cJSON_Parse(buffer);
        free(buffer);
        if(!root){
            syslog(LOG_CRIT, "[server] can not parse the buffer(%s) to json!\n", buffer);
            return -1;
        }
        cJSON *tmp = NULL;
        switch(table){
            case TABLE_ROUTER:
                tmp = cJSON_GetObjectItem(root,"router");
                break;
            case TABLE_ROUTER_SUB:
                tmp = cJSON_GetObjectItem(root,"routersub");
                break;
            case TABLE_ROUTER_STATE:
                tmp = cJSON_GetObjectItem(root,"routerstate");
                break;
            case TABLE_ROUTER_WIFI:
                tmp = cJSON_GetObjectItem(root,"wireless");
                break;
            case TABLE_ROUTER_DEVICE:
                tmp = cJSON_GetObjectItem(root,"devices");
                if(order >= cJSON_GetArraySize(tmp)){
                    cJSON_Delete(root);
                    return -1;
                }
                tmp = cJSON_GetArrayItem(tmp, order);
                break;
            default:
                cJSON_Delete(root);
                return -1;
        }
        if(!tmp){
            cJSON_Delete(root);
            return -1;
        }
        strcpy(object, tmp->valuestring);
        cJSON_Delete(root);
        return 0;
    }else{
        syslog(LOG_CRIT,"[server] can not open the file %s!\n", OBJECT_FILE_PATH);
        return -1;
    }
}
