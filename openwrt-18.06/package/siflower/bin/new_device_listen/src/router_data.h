/*
 * =====================================================================================
 *
 *       Filename:  router_data.h
 *
 *    Description:  define router data model
 *
 *        Version:  1.0
 *        Created:  2015年08月12日 10时55分42秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert (), robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#ifndef ROUTER_DATA_H
#define ROUTER_DATA_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

enum{
    TABLE_ROUTER = 0x10,
    TABLE_ROUTER_SUB,
    TABLE_ROUTER_STATE,
    TABLE_ROUTER_WIFI,
    TABLE_ROUTER_DEVICE
};

typedef struct {
    int32_t enable;
    int32_t mode;
}general_setting;

typedef struct {
    int32_t type;
    int32_t autodns;
    char pppname[32];
    char ppppwd[32];
    char ip[32];
    char mask[32];
    char dns1[32];
    char dns2[32];
    char gateway[32];
}wantype_t;

typedef struct{
    int32_t  signal;
    int32_t  enable;
    int32_t  channel;
    char    band[32];
    char    ifname[32];
    char    mac[32];
    char    encryption[32];
    char    password[32];
    char    ssid[32];
    char    network[32];
    char    sn[50];
    int32_t reserverd;

    char    objectid[32];
}wireless_table;

typedef struct{
	int32_t ac;
	int32_t wifi;
	int32_t dhcp;
	int32_t upnp;
	int32_t ddns;
	int32_t freqInter;
	int32_t usb;
	int32_t onlineWarn;
	int32_t guestWifi;
	int32_t leaseWiFi;
	int32_t leaseWeb;
	int32_t led;
	int32_t externalPA;
	int32_t virtualServer;
	int32_t staticRouter;
	int32_t homeControl;
	int32_t wds;
	int32_t dmz;
}router_feature;

typedef struct {
    char    mac[32];
    char    name[32];
    char    romversion[32];
    int32_t romtype;
	int32_t zigbee;
	int32_t storage;
    char    sn[50];
    char    account[32];
    int32_t romversionnumber;
    int32_t disk;
    char    hardware[32];
    char    romtime[32];
    general_setting qos;
    general_setting wifi_filter;
    wantype_t wan_type;
    wireless_table *wireless;
    router_feature feat;
    int32_t wnum;
    char    objectid[32];
}router_table;

typedef struct{
    char sn[50];

    char objectid[32];
}router_sub_table;

typedef struct{
    char    sn[50];
    int32_t status;
    int32_t cpuload;
    int32_t devicecount;
    int32_t downloadingcount;
    int32_t downspeed;
    int32_t memoryload;
    int32_t upspeed;
    int32_t useablespace;

    char    objectid[32];
}router_state_table;


typedef struct{
    int32_t internet;
    int32_t wan;
    int32_t lan;
    int32_t notify;
    int32_t speedlvl;
    int32_t disk;
    int32_t limitup;
    int32_t limitdown;
}device_authority;

typedef struct{
    int32_t maxdownloadspeed;
    int32_t uploadtotal;
    int32_t upspeed;
    int32_t downspeed;
    int32_t online;
    int32_t maxuploadspeed;
    int32_t downloadtotal;
}device_speed;

typedef struct{
    char    originname[32];
    char    hostname[32];
    char    sn[50];
    char    mac[32];
    int32_t online;
    char    ip[32];
    int32_t port;
    char    dev[32];
    char    icon[32];
    device_authority authority;
    device_speed    speed;

    char    objectid[32];
}device_table;

typedef struct _my_message{
    /*message's cmd */
    int32_t cmd;
    int32_t client;
    char *args;
    char *data;
    char *callback;
    struct _my_message *prev;
    struct _my_message *next;
}my_message;


typedef struct _my_msg_list{
    int32_t size;
    my_message *head;
    my_message *tail;
}my_msg_list;

#endif
