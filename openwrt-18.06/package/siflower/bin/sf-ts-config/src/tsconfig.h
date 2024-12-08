/*
 * =====================================================================================
 *
 *       Filename:  tsconfig.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2018年10月30日 18时58分27秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert (), robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#ifndef __TSCONFIG_H
#define __TSCONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "hash.h"

#define IP_TAG_TYPE_GAME 1
#define IP_TAG_TYPE_SOCIAL 2
#define IP_TAG_TYPE_VIDEO 3

#define IP_TAG_FLAG_VALID 0x1
#define IP_TAG_FLAG_INVALID (0x1 << 1)

struct pctl_ip_tag{
	//type defined as IP_TAG_TYPE...
	u_int8_t type;
	//flags of tag,valid/invalid
	u_int8_t flags;
	//unique ip address XXX.XXX.XXX.XXX
	u_int8_t ip[4];
	//sub class for sub item, now reserved
	u_int8_t subclass;
	//dns of tag
	char url[64];
	//description of tag
	char descp[32];
};

struct ts_table{
	int count;
	struct curl_hash iptag_cache;
};

struct ts_tag{
	struct pctl_ip_tag iptag;
};

struct ts_table *ta_table_init(void);
struct ts_tag *ts_table_search_tag(struct ts_table *table,struct pctl_ip_tag *iptag);
void ts_table_insert_tag(struct ts_table *table,struct pctl_ip_tag *iptag);
void ts_table_insert_unique_tag(struct ts_table *table,struct pctl_ip_tag *iptag);
void ts_table_delete_tag(struct ts_table *table,struct pctl_ip_tag *iptag);
void ta_table_deinit(struct ts_table *table);
int sync_table_to_file(struct ts_table *table,const char *filename);

#endif
