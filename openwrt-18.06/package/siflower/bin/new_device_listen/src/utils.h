/*
 * =====================================================================================
 *
 *       Filename:  utils.h
 *
 *    Description:  :
 *
 *        Version:  1.0
 *        Created:  07/28/2015 07:30:38 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  franklin (), franklin.wang@siflower.com.cn
 *        Company:  Siflower Communication Tenology Co.,Ltd
 *
 * =====================================================================================
 */
#ifndef __SOCK_SERVER_UTILS_H__
#define __SOCK_SERVER_UTILS_H__

#include "router_data.h"
#include "cJSON.h"
#include "device_info.h"
#include <syslog.h>

#define DEBUG_LOG 1
#define URLLIST 1
#define USE_CUSTOM_SOCKET 1

// #define DB_SYSLOG 1

#ifdef DB_SYSLOG
#define DB_LOG(X,...) syslog(LOG_CRIT,X,##__VA_ARGS__)
#else
#define DB_LOG(X,...) do{}while(0)
#endif

#define LOG(X,...) syslog(LOG_CRIT,X,##__VA_ARGS__)
// #define LOG(X,...) printf(X,##__VA_ARGS__)

#define UNIX_DOMAIN "/tmp/UNIX.domain"
#define my_assert(a) {\
    if(!(a)){   \
        syslog(LOG_CRIT,"[server]assert happen, file :%s , line : %d\n", __FILE__, __LINE__);   \
        void *b = NULL; \
        b = 0;  \
    }   \
}

#ifndef bool
#define bool int32_t
#define false 0
#define true 1
#endif

#define FAKE_WIFI_NUM 2
#define FAKE_DEVICES_NUM 10


#define WIFI_2_4_G 1
#define WIFI_5_G 2

//sn related info
#define SN_LEN 16
#define SN_STR_LEN  33
#define SN_OFFSET 6
#define SN_VERIFY_OFFSET 22
#define SN_SYSINFO_LEN  8
#define SN_RANDOM_LEN  8
#define SN_VERIFY_VALUE 0x33

extern struct MesageQueue *g_periodEventQueue;

//common methord to get value from json item
int32_t get_json_value_string(cJSON *item,char *key,char *buffer);
int32_t get_json_value_int(cJSON *item,char *key,int32_t *buffer);
int32_t get_json_pointer_attr_string(cJSON *root,char *name,char *key,char *buffer);
int32_t get_json_pointer_attr_int(cJSON *root,char *name,char *key,int32_t *buffer);
int32_t postHttpCommon(char *url,void *data,void *response,int https,int usetoken);
int32_t parseStringFromJson(const char *key,void *data,char *value);
int32_t parseIntFromJson(const char *key,void *data,int *value);
int32_t parseIntFromData(const char *key,void *data,int *value);
int32_t parseStringFromData(const char *key,void *data,char *value);
size_t OnHttpReponseData(void* buffer, size_t size, size_t nmemb, void* lpVoid);

/*
func: get siwifi hardware configfrom uci
param key: option in siwifi.hardware section
param retBuffer: buffer to save result value
return:
-1 get failed
0, get success
 */
int32_t getSfHardwareConfig(const char *key,char *retBuffer);
//global information
/*
func:has been binded already and give the router objectid if the user want(set router_id not null)
params:
return:
    0 ---has been binded already
    -1----not binded, it is a refresh router
 */
int32_t getUciConfig(const char *file, const char *node, const char *key,char *retBuffer);
int32_t has_binded();

/*
func: save the tables object id
params: all tables
return:
-1 save failed
0, save success
 */
int32_t save_bind(router_table *rt, char *binder_id, int32_t bind);

/*
func:delete the bind information
params: all if > 0 delete all the tables, otherwise only change the bind state
return:
-1 failed
0, success
 */
int32_t delete_bind(int32_t all);

/*
func:get the special table's objectid
params:
return:-1 failed, 0 success
 */
int32_t get_table(char *object, int32_t table, int32_t order);

//list operation
/*
func:push the msg to the plist's tail. User should take a mutex lock when use this function
     change the message list's tail after calling this function
params:
    my_msg_list *plist  the msg list to insert,see the struct my_msg_list
    my_message *msg     see the struct my_message
return:return the message list size after push;an error happen(like null params passed in) when return -1
 */
int32_t push(my_msg_list *plist, my_message *msg);


/*
func:pop the plist's head Node out.User should take a mutex lock when use this function
    change the plist's header after calling this function
params:
    my_msg_list *plist the msg list to pop out,see the struct my_msg_list
return:return the head message of the plist, if noting, NULL will be return
 */
my_message *pop(my_msg_list *plist);


int32_t getOnlineDeviceNumberFromUci();
void getOnlineDeviceIpFromUci(char *ip_list[]);

/*
func:get table value
params:
return:-1 failed, 0 success
 */
int32_t get_router_table_value(router_table *rt);

/*
func:get table value
params:
return:-1 failed, 0 success
 */
int32_t get_router_state_table_value(router_state_table *rst);

/*
func:get table value
params:
return:-1 failed, 0 success
 */
int32_t get_router_sub_table_value(router_sub_table *subt);

/*
func:get table value
params:
return:-1 failed, 0 success
 */
int32_t get_wireless_table_info(wireless_table **wt, int32_t *wnum);

/*
func:get table value
params:
return:-1 failed, 0 success
 */
int32_t get_devices_info(device_table **dst, int32_t *dnum);

int32_t add_router_to_json(cJSON *root, int32_t isknown, router_table router);

int32_t add_router_sub_to_json(cJSON *root, int32_t isknown, router_sub_table routersub);

int32_t add_router_state_to_json(cJSON *root, int32_t isknown, router_state_table routerstate);

int32_t add_wifi_info_to_json(cJSON *root, int32_t isknown, wireless_table *routerwifi, int32_t wifinum);

int32_t get_5G_ssid(char *buf);

int32_t add_devices_info_to_json(cJSON *root, int32_t isknown, device_table *routerdevice, int32_t devicenum);

/*************************************************/
//function: isnewdevice
//brief: check the macaddr is new device
//param: macaddr the mac address to be checked
//return: -1 calloc error
//         0 the macaddr isn't new device
//         1 the macaddr is new device
/*************************************************/
//int32_t isnewdevice(char *macaddr);



//int32_t updateDevlistUci(char *macaddr, char *isonline, char *ip);
int32_t updateDevlistUci(devinfo *device, int newdev);


int32_t getCurTime(struct tm *t);


int32_t getPushSetting(Pushinfo *push_setting);



/****************************************/
//function: strrpl
//brief: use dpl to replace spl in str
//param: str-----the source string
//       spl-----the char to be replaced
//       dpl-----the char used to replace spl
//return: void
/****************************************/
extern void strrpl(char *str, char spl, char dpl);


extern void strupper(char *str);

extern void strlower(char *str);

char slversion[64];

//int detect_process(char *process_name);

//get wifi mac address when data sync or getToken
void get_router_macaddr(char *mac_in);

#endif
