/*
 * =====================================================================================
 *
 *       Filename:  device_info.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2015年09月04日 11时55分11秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  kevin.feng (), kevin.feng@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#ifndef  __DEVICE_INFO_H__
#define __DEVICE_INFO_H__



#define IP_PATTERN            "([0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3})"
#define MAC_PATTERN           "([0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2})"
#define NAME_PATTERN          "([0-9]+\134\056[0-9]+\134\056[0-9]+\134\056[0-9]+\040.*\040)"


#define ARP_PATH "/proc/net/arp"
#define ARP_ITEM_MAX_LENGTH 100
#define WLDEVLIST "wldevlist"
#define DHCP_PATH "/tmp/dhcp.leases"
#define DHCP_ITEM_MAX_LENGTH 100


typedef struct{
    char ipaddr[16];
    int ipavailable;
    int dhcp;
} ipdetail;

typedef struct{
    char macaddr[20];
    ipdetail iplist[12];
    char interface[12];
    int curipnum;
    int ipnum;
    char online[2];
    char lan[2];
    char internet[2];
    int last_online_state;
    char hostname[50];
    int newdevice;
    char is_wireless[2];
    int notify;
    long associate_time;
} devinfo;

typedef struct{
    int enable;
    int mode;
} Pushinfo;


static int strsub(char *str, char start, char end);
static int regex_match(char *str, const char *pattern);
extern int GetDevNotify(devinfo *device);
static int GetDevIp(devinfo *device);
static int check_ip_available(devinfo *device);
extern int32_t GetDevName(devinfo *device);
extern int GetDevLowInfo(devinfo *device);

#endif
