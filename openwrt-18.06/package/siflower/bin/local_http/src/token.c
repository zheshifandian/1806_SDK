#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <shadow.h>
#include <regex.h>
#include <uci.h>
#include "cJSON.h"
#include "local_http.h"
#include "token.h"
//#include "utils.h"
#include "mtd.h"
#include "sf_factory_read.h"

//extern char XCLOUD_REMOTE_ADDR[];
char REMOTE_ADDR[64];
char token[256];
static const char *XCLOUD_TOKEN_FORMAT = "{\"sn\": \"%s\",\"mac\":\"%s\" }";
#define XCLOUD_TOKEN "getToken"
#define ETH_ALEN 6

char macaddr[32];
char routersn[64];

int32_t getSfHardwareConfig(const char *key,char *retBuffer)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, "siwifi", &p) == UCI_OK)
    {
        struct uci_section *router = uci_lookup_section(ctx, p, "hardware");
        //lookup values
        if(router != NULL){
            const char *value = uci_lookup_option_string(ctx, router, key);
            if(value != NULL){
                ret = 0;
                sprintf(retBuffer,"%s",value);
            }
        }
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}

static void inc_sf_mac_addr(char *mac, int inc)
{
	unsigned int mac_calc[ETH_ALEN] = {0};
	int i = 0;
	for(i = 0 ; i < ETH_ALEN ; i++)
	{
		mac_calc[i] = (unsigned int)(mac[i]) & 0xFF;
	}
	mac_calc[ETH_ALEN - 1] += inc;
	mac_calc[ETH_ALEN - 2] += ((mac_calc[ETH_ALEN - 1] & (0x100))  >> 8);
	mac[ETH_ALEN - 1] = mac_calc[ETH_ALEN - 1] & (0xff);

	mac_calc[ETH_ALEN - 3] += ((mac_calc[ETH_ALEN - 2] & (0x100))  >> 8);
	mac[ETH_ALEN - 2] = mac_calc[ETH_ALEN - 2] & (0xff);
	mac[ETH_ALEN - 3] = mac_calc[ETH_ALEN - 3] & (0xff);
	//the first 3 char is reserved
}

void get_router_macaddr(char *mac_in)
{
	if(sf_factory_read_operation(READ_FIRST_MAC_ADDRESS, mac_in, ETH_ALEN)){
		mtd_operation(CMD_READ, mac_in, 6, 0);
	}
	inc_sf_mac_addr(&mac_in[0],2);
}

size_t OnHttpReponseData(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    if(size * nmemb > 0){
        char *str = malloc(size*nmemb + 1);
        memcpy(str,buffer,size*nmemb);
        *(str + size*nmemb) = '\0';
        //LOG("get result=%s\n",str);
        if(lpVoid){
            ((struct HttpResonseData*)lpVoid)->size = size * nmemb;
            ((struct HttpResonseData*)lpVoid)->data = str;
        }else{
            free(str);
        }
    }
    return size*nmemb;
}

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
    sprintf(value,"%s", (char*)(object->valuestring));
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
    cJSON_GetValueInt(object, value);
    res = 0;
clean:
    //free
    if(json) cJSON_Delete(json);
    return res;
}

int32_t postHttpsForToken(char *url,void *data,void *response)
{
    struct HttpResonseData *response_tmp = (struct HttpResonseData *)response;
    response_tmp->data = NULL;
    response_tmp->size = 0;
    CURL* curl = curl_easy_init();
    if(NULL == curl) return CURLE_FAILED_INIT;

    long time1 = time(NULL);
    struct curl_slist *slist = NULL;
    char header[100] = "";
    slist = curl_slist_append(slist, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4 );
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnHttpReponseData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_tmp);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 6);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

    int32_t res = curl_easy_perform(curl);

    response_tmp->data = realloc(response_tmp->data, response_tmp->size+1);
    ((char *)(response_tmp->data))[response_tmp->size] = '\0';
	response_tmp->size += 1;

    long responseCode;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    if((responseCode % 200) > 100 || responseCode >= 400){
        LOG("postHttpsForToken request fail %d \n",(int)responseCode);
        res = -1;
    }
    LOG("postHttpsForToken %ld cost %ld s url: %s\n",responseCode,time(NULL) - time1,url);
    curl_easy_cleanup(curl);
    curl_slist_free_all(slist);
    return 0;
}

int8_t GetToken(){
	char url[256] = "";
	char data[256] = "";
	int8_t ret;
	struct HttpResonseData response;
	int32_t retcode;

	response.size = 0;

	if(strlen(routersn) == 0){
		TokenInit();
		if(strlen(routersn) == 0){
			return 0;
		}
	}
	sprintf(data, XCLOUD_TOKEN_FORMAT, routersn, macaddr);
	sprintf(url,"%s/%s",REMOTE_ADDR, XCLOUD_TOKEN);

	ret = postHttpsForToken(url, (void *)data, (void *)&response);
	LOG("token response is %s\n",(char *)response.data);
	if (ret == 0){
		parseIntFromData("code", response.data, &retcode);
		if (retcode == 0){
			parseStringFromData("data", response.data, token);
			LOG("token is %s\n",token);
		}else{
			LOG("get token fail\n");
		}
	}

	if(response.size > 0)
		free(response.data);

	return ret;
}

void TokenPrepareCallbackData(char **callback,char *ret,char *reason)
{
    if(callback){
        cJSON *json = cJSON_CreateObject();
        cJSON_AddStringToObject(json,"ret",ret);
        cJSON_AddStringToObject(json,"reason",reason);
        cJSON_AddStringToObject(json,"token",token);
        *callback = (char *)malloc(strlen(reason) + strlen(token) + 256);
        char *json_tmp = cJSON_Print(json);
        sprintf(*callback,"%s", json_tmp);
        cJSON_Delete(json);
        free(json_tmp);
    }
}

void DoToken(char *data, char **callback)
{
    LOG("[server]%s, args : %s\n",__func__, data ? data : "NULL");

	char tmp[32] = "";
    int32_t ret = GetToken();
    if(callback){
        TokenPrepareCallbackData(callback,ret < 0 ? "fail" : "success",tmp);
        LOG("[server] write system event ret=%d callback:%s!\n",ret,*callback);
    }
}

int8_t TokenInit(){
	uint8_t base_mac[6] = "";

	get_router_macaddr(base_mac);
	sprintf(macaddr,"%02x:%02x:%02x:%02x:%02x:%02x",base_mac[0],base_mac[1],base_mac[2],base_mac[3],base_mac[4],base_mac[5]);

    getSfHardwareConfig("sn",routersn);

	LOG("sn is %s, mac addr is %s", routersn, macaddr);

	return 0;
}
void remote_addr_init(char* addr)
{
    memset(REMOTE_ADDR, 0, sizeof(REMOTE_ADDR));
    strcpy(REMOTE_ADDR, addr);
}
