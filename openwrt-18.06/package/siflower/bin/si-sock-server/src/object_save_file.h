/*
 * =====================================================================================
 *
 *       Filename:  object_save_file.h
 *
 *       Description:  save object as files
 *
 *        Version:  1.0
 *        Created:  2015年08月12日 14时01分34秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert (), robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */


#ifndef OBJECT_SAVE_FILE_H
#define OBJECT_SAVE_FILE_H

#include "router_data.h"

int32_t has_binded_file();


int32_t has_inited_file(
            router_table *rt, router_sub_table *subt, router_state_table *rst,
            wireless_table **wt, int32_t *wnum, device_table **dst, int32_t *dnum
            );

int32_t delete_bind_file(int32_t all);

int32_t save_bind_file(router_table *rt, router_sub_table *subt, router_state_table *rst,
    wireless_table *wt, int32_t wnum, device_table *dst, int32_t dnum,char *binder_id);

int32_t get_table_file(char *object, int32_t table, int32_t order);    

int32_t save_bind_file(router_table *rt, router_sub_table *subt, router_state_table *rst,
                        wireless_table *wt, int32_t wnum, device_table *dst, int32_t dnum,char *binder_id);


#endif

