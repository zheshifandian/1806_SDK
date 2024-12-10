/*
 * =====================================================================================
 *
 *       Filename:  cloud_common.c
 *
 *    Description:  common utils
 *
 *        Version:  1.0
 *        Created:  2015年07月30日 10时57分52秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert , robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <regex.h>
#include <curl/curl.h>
#include <uci.h>
#include "cJSON.h"
#include "remote_https.h"
#include "cloud_common.h"
#include "local_http.h"
#include "local_storage.h"
#define UNIX_DOMAIN "/tmp/UNIX.domain"


char CLOUD_ROUTERSUB_OBJECT[20] = "";
char CLOUD_ROUTER_OBJECT[20] = "";
int CLOUD_BIND = 0;

int32_t parseStringFromData(const char *key,void *data,char *value)
{
    int32_t res = -1;
    cJSON* json = cJSON_Parse(data);
    if(!json) goto clean;
    cJSON *object = cJSON_GetObjectItem(json,key);
    if(!object){
        LOG("object not exist\n");
        goto clean;
    }
    sprintf(value,"%s",(char*)(object->valuestring));
    res = 0;
clean:
    //free
    if(json) cJSON_Delete(json);
    return res;
}

int32_t parseIntFromData(const char *key,void *data,int *value)
{
    int32_t res = -1;
    cJSON* json = cJSON_Parse(data);
    if(!json) goto clean;
    cJSON *object = cJSON_GetObjectItem(json,key);
    if(!object){
        LOG("object not exist\n");
        goto clean;
    }
    *value = (int)(object->valueint);
    res = 0;
clean:
    //free
    if(json) cJSON_Delete(json);
    return res;
}

void getSubstr(char *stbuf,const char*str, unsigned start, unsigned end)
{
    unsigned n = end - start;
    memcpy(stbuf, str + start, n);
    *(stbuf + n) = 0;
}


char *my_strstr(char * ps,char *pd)
{
    char *pt = pd;
    int c = 0;
    while(*ps != '\0')
    {
        if(*ps == *pd)
        {
            while(*ps == *pd && *pd!='\0')
            {
                ps++;
                pd++;
                c++;
            }
        }else
        {
            ps++;
        }
        if(*pd == '\0')
        {
            return (ps - c);
        }
        c = 0;
        pd = pt;
    }
    return 0;
}

const char nixio__bin2hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

void bin2hex(void *src,int length,char *hex)
{
    char *data = (char *)src;
    int i;
    for (i = 0; i < length; i++) {
        hex[2*i]   = nixio__bin2hex[(data[i] & 0xf0) >> 4];
        hex[2*i+1] = nixio__bin2hex[(data[i] & 0x0f)];
    }
}

int32_t sendCmdToSockServer(char *cmd,char *result)
{
    int32_t connect_fd;
    struct sockaddr_un un_addr;
    struct sockaddr_in in_addr;
    struct sockaddr* con_addr = NULL;
    int32_t sock_size = 0;
    int32_t ret = 0;
    connect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(connect_fd < 0)
    {
        LOG("[CLIENT]client create socket failed");
        return -1;
    }
    un_addr.sun_family = AF_UNIX;
    strcpy(un_addr.sun_path, UNIX_DOMAIN);
    con_addr = (struct sockaddr *)&un_addr;
    sock_size = sizeof(struct sockaddr_un);
    ret = connect(connect_fd, con_addr, sock_size);
    if(ret == -1)
    {
        LOG("[CLIENT]connect to server failed!");
        close(connect_fd);
        return ret;
    }
    int32_t tx_num = write(connect_fd, cmd, strlen(cmd));
    if(tx_num > 0){
        if(my_strstr(cmd,"need-callback") == 0){
            ret = 0;
        }else{
            char rcv_buf[4096];
            memset(rcv_buf, 0, 4096);
            int32_t rcv_num = read(connect_fd, rcv_buf, 4096);
            LOG("recive sock-server result num = %d resonse=%s \n",rcv_num,rcv_buf);
            if(rcv_num > 0){
                sprintf(result,"%s",rcv_buf);
            }
            ret = (rcv_num > 0) ? 0 : -1;
        }
    }else{
        ret = -1;
    }
    close(connect_fd);
    return ret;
}

int32_t getUptime()
{
    struct sysinfo info;
    if (!sysinfo(&info)) {
        return info.uptime;
    }else{
        LOG("Failed to get sysinfo, errno:%u, reason:%s\n",errno, strerror(errno));
        return 0;
    }
}

int32_t formatObjectId(char *in,char *out)
{
    static const char *STR_CLOUD_PATTERN = "([^a-z^0-9^A-F]?)([a-z0-9A-F]+)([^a-z^0-9^A-F]?)?";
    regex_t regex;
    int32_t errcode = regcomp(&regex, STR_CLOUD_PATTERN , REG_EXTENDED);
    if(errcode != 0){
        LOG("regcomp pattern error %d\n",errcode);
        return -1;
    }
    regmatch_t value[6];
    errcode = regexec(&regex, (char *)in,6, value, 0);
    if(errcode != 0){
        LOG("regexec does't match error %d\n",errcode);
        return -1;
    }
    if(value[2].rm_so != -1){
        getSubstr(out,in,value[2].rm_so,value[2].rm_eo);
    }else{
        LOG("not format objectid\n");
        regfree(&regex);
        return -1;
    }
    regfree(&regex);
    return 0;
}

int checkUserInfo(void)
{
    int ret = 0;
    cJSON *json = cJSON_CreateObject();
    char sn[50] = "";
    getSfHardwareConfig("sn",sn);
	if(strlen(sn) == 0){
		return -1;
	}
    cJSON_AddStringToObject(json,"sn",sn);

    char *json_str = (char*)(json->valuestring);
    void *data = malloc(strlen(json_str) + 256);
    sprintf((char*)data,"%s",json_str);
    if(json) cJSON_Delete(json);
    char url[256] = "";
    sprintf(url,"%s/%s",CLOUD_REMOTE_FUNCTION_ADDR,CLOUD_REMOTE_FUNCTION_CHECK_USERINFO);
    struct HttpResonseData response;
    response.size = 0;
    LOG("subcloud checkUserinfo upload--data=%s\n",(char*)data);
    int32_t ret3 = postHttps(url,data,&response,1);
    LOG("subcloud checkUserinfo upload---ret= %d --response=%s\n",ret3,(response.size > 0) ? (char*)response.data : "");
    free(data);
    if(ret3 != CURLE_OK){
        LOG("subcloud checkUserinfo failed--ret= %d\n",ret3);
        ret = -1;
    }
    if(response.size > 0) free(response.data);
    return ret;
}

void getVersion(char *version){
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, "sicloud", &p) == UCI_OK){
        struct uci_section *addr = uci_lookup_section(ctx, p, "addr");
        strcpy(version,uci_lookup_option_string(ctx, addr, "version"));
    }
    uci_unload(ctx,p);
    uci_free_context(ctx);
}
