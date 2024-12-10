#include "local_stub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <shadow.h>
#include <regex.h>
#include <uci.h>
#include "cJSON.h"
#include "cloud_common.h"
#include "local_http.h"
#include "local_storage.h"
#include "remote_https.h"
#include "mtd.h"

#define MANAGERFILE "/etc/config/simanager"
#define ID_LEN 18
#define ID_BUF_S 256

void checkBindId(void){
	uint8_t bindid[32];
	uint8_t managerid[ID_BUF_S];
	uint8_t idtmp[ID_LEN + 1];
	uint16_t i=0;
	int8_t ret;
	int8_t *json_str;
	FILE *mfile;

	ret = getRouterValueFromUci("binder",bindid);
	if (ret < 0){
		LOG("get binder fail\n");
		goto fail;
	}

	mfile = fopen(MANAGERFILE,"r");
	if (mfile){
		fgets(managerid, ID_BUF_S -1, mfile);
		fclose(mfile);
		LOG("open managerid file success\n");
	}else{
		LOG("open managerid file fail\n");
		managerid[0] = '\0';
	}

	cJSON *object = cJSON_CreateObject();
	if(!object)
		goto fail;
    cJSON_AddStringToObject(object,"sn",routersn);
    cJSON_AddStringToObject(object,"bindid",bindid);
	cJSON *midlist = cJSON_CreateArray();
	if(managerid[0] != '\0'){
		idtmp[ID_LEN] = '\0';
		while(managerid[i] != '\0' && managerid[i] != '\n' && i < ID_BUF_S - ID_LEN){
			strncpy(idtmp, managerid + i, ID_LEN);
			cJSON_AddItemToArray(midlist, idtmp);
			i = i + ID_LEN;
		}
	}
	LOG("prepare data ok\n");
	cJSON_AddItemToObject(object,"mgrlist",midlist);
	json_str = (int8_t *)cJSON_Print(object);

    struct HttpResonseData response;
	struct cJSON *result_object = NULL;
	struct cJSON *array_object = NULL;
	struct cJSON *obj;
    int8_t url[256] = "";
	int16_t resp_ret;
	uint8_t j = 0;
    void *data = malloc(strlen(json_str) + 256);

    sprintf((char*)data,"%s",json_str);
    if(object) cJSON_Delete(object);
    if(json_str) free(json_str);
    sprintf(url,"%s%s",XCLOUD_REMOTE_ADDR,CLOUD_REMOTE_FUNCTION_CHECK_BINDMGR);
    response.size = 0;
    LOG("post check bind--data=%s\n",(char*)data);
    ret = postHttps(url,data,&response,0);
    LOG("post check bind---ret= %d --response=%s\n",ret,(response.size > 0) ? (char*)response.data : "");
    free(data);

    if(ret != CURLE_OK || response.size == 0){
        LOG("post check bind fail ret= %d\n",ret);
        ret = -1;
    }else{
		if(parseIntFromData("code", response.data, &resp_ret) == 0){
			if(resp_ret == 409){
				result_object = cJSON_Parse(response.data);
				if(!result_object){
					LOG("get data from return code 409 fail\n");
					goto fail2;
				}
				array_object = cJSON_GetObjectItem(result_object, "data");
				if(!array_object){
					LOG("no managerid\n");
				}else{
					LOG("id is %d\n",cJSON_GetArraySize(array_object));
					for(;j < cJSON_GetArraySize(array_object); j++) {
						obj = cJSON_GetArrayItem(array_object, j);
						strncpy(managerid + j*ID_LEN, (char*)(obj->valuestring), ID_LEN);
					}
				}
				managerid[j*ID_LEN] = '\0';
				if(result_object) cJSON_Delete(result_object);
				LOG("write data to id file\n");
				mfile = fopen(MANAGERFILE,"w");
				if (mfile){
					fputs(managerid, mfile);
					fputc('\0', mfile);
					fclose(mfile);
				}else{
					ret = -1;
					goto fail2;
				}
			}else if(resp_ret == 407){
				LOG("start unbind router\n");
				ret = setRouterValueToUci("cloudrouter","bind","0");
				if(ret){
					LOG("CheckBindMgr: unbind fail\n");
				}else{
					setRouterValueToUci("cloudrouter","binder","");
					system("/etc/init.d/subservice stop");
				}
			}else{
				ret = 0;
			}
		}
	}
fail2:
    if(response.size > 0) free(response.data);
fail:
	return ret;
}
