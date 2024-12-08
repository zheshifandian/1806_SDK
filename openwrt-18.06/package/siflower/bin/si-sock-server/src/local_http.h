/*
 * =====================================================================================
 *
 *       Filename:  local_http.h
 *
 *    Description:  define methord to interactive with local uhttpd luci web service
 *
 *        Version:  1.0
 *        Created:  2015年08月12日 15时51分41秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert (), robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#ifndef LOCAL_HTTP_H
#define LOCAL_HTTP_H

#include "stdlib.h"
#include <stdint.h>
#include <syslog.h>
#include "utils.h"

#define LOCAL_COMMAND_INIT_INFO                 "/api/sfsystem/init_info"
#define LOCAL_COMMAND_MAIN_STATUS               "/api/sfsystem/main_status"
#define LOCAL_COMMAND_WIFI_DETAIL               "/api/sfsystem/wifi_detail"
#define LOCAL_COMMAND_DEVICE_LIST_BACKSTAGE     "/api/sfsystem/device_list_backstage"
#define LOCAL_COMMAND_UPDATE_QOS                "/api/sfsystem/update_qos_local"
#define LOCAL_COMMAND_DEVICE_LIST               "/api/sfsystem/device_list"
#define LOCAL_COMMAND_GET_ROUTER_FEATURE        "/api/sfsystem/getrouterfeature"
#define LOCAL_COMMAND_QOS_INFO                  "/api/sfsystem/qos_info"
#define LOCAL_COMMAND_GET_WIFI_FILTER           "/api/sfsystem/get_wifi_filter"
#define LOCAL_COMMAND_GET_WAN_TYPE              "/api/sfsystem/get_wan_type"
#define LOCAL_COMMAND_ARP_CHECK_DEV				"/api/sfsystem/arp_check_dev"
#define LOCAL_COMMAND_GET_UPGRADE_PROCESS       "/api/sfsystem/ota_upgrade"
#define LOCAL_COMMAND_GET_AC_UPGRADE_PROCESS	"/api/sfsystem/ac_ota_upgrade"

#define ID_BUF_S 256
#define MANAGERFILE "/etc/config/simanager"

struct HttpResonseData{
    size_t size;
    void *data;
    long code;
	struct HttpResonseData *next;
};

struct HttpCookieData{
    size_t size;
    void *data;
};

//local request to uhttpd-luci
//response is type of struct *HttpCookieData
int postDataToHttpd(char *command,char *postData,void *response,char *userId,int32_t timeout,int32_t count);

//no user id
int postDataToHttpdCommon(char *command,char *postData,void *response);

//clean cookie cache in the end
void cleanCookieCache();

//join HttpResonseData link list to one data
struct HttpResonseData *JoinHttpResponseData(struct HttpResonseData *response);
#endif
