/*
 * =====================================================================================
 *
 *       Filename:  sf_factory_read.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:	11/14/2018 10:03:37 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  star , star.jiang@siflower.com.cn
 *        Company:  Siflower Communication Tenology Co.,Ltd
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "sf_factory_read.h"

static int sf_factory_read(char *data, int len, char *ps)
{
	FILE *ptr = NULL;
    char buff[128];
    char temp[128];
	int i = 0;

    if((ptr=popen(ps, "r")) != NULL){
        while(fgets(buff, 6, ptr) != NULL)
        {
            //LOG("buffer--=%s\n",buff);
			temp[i] = ((char)strtol(buff, NULL, 16) & 0xFF);
			i++;
        }
		if(!i){
			LOG("No value from popen\n");
			pclose(ptr);
			return -1;
		}
    }
	else{
		LOG("can not get value through %s\n",ps);
		return -1;
	}
	pclose(ptr);
    memcpy(data, temp, len);
	return 0;
}

int sf_factory_read_operation(enum sf_factory_read_action action, char* data, int len)
{
	char ps[128];
	if(data == NULL){
		LOG("data is NULL\n");
		return -1;
	}
	switch (action) {
		case READ_MAC_ADDRESS:
			strcpy(ps,"cat /sys/devices/platform/factory-read/macaddr");
			break;
		case READ_SN:
			strcpy(ps,"cat /sys/devices/platform/factory-read/sn");
			break;
		case READ_SN_FLAG:
			strcpy(ps,"cat /sys/devices/platform/factory-read/sn_flag");
			break;
		case READ_WAN_MAC_ADDRESS:
			strcpy(ps,"cat /sys/devices/platform/factory-read/macaddr_wan");
			break;
		case READ_WIFI_LB_MAC_ADDRESS:
			strcpy(ps,"cat /sys/devices/platform/factory-read/macaddr_lb");
			break;
		case READ_WIFI_HB_MAC_ADDRESS:
			strcpy(ps,"cat /sys/devices/platform/factory-read/macaddr_hb");
			break;
		case READ_FIRST_MAC_ADDRESS:
			strcpy(ps,"cat /sys/devices/platform/factory-read/macaddr0");
			break;
		default:break;
	}
	if(sf_factory_read(data, len, ps))
		return -1;
	return 0;
}
