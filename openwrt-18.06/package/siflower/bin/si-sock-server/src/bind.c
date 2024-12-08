/*
 * =====================================================================================
 *
 *       Filename:  bind.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  07/21/2015 08:59:48 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  franklin , franklin.wang@siflower.com.cn
 *        Company:  Siflower Communication Tenology Co.,Ltd
          TODO:
          1)unbind workflow is not defined
          2)get the actual value from uci config files
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uci.h>
#include <curl/curl.h>
#include "utils.h"
#include "bind.h"
#include "cJSON.h"
#include "http.h"
#include "token.h"
#include "manager.h"
#include "ssst_request.h"
#include "local_storage.h"
#include "status_sync.h"


extern char slversion[64];

int32_t clean_subservice(char *data)
{
    LOG( "[server] %s\n", __func__);
    cJSON *root = cJSON_Parse(data);
    if(!root){
        LOG( "[server] can not parser the data : %s\n", data);
        return -1;
    }
    int32_t remote = -1;
    cJSON *caller = cJSON_GetObjectItem(root,"caller");
    if(!caller){
        LOG( "[server] the 'caller' key do not include in the unbind data\n");
        cJSON_Delete(root);
        return -1;
    }
    if(cJSON_GetValueInt(caller, &remote) < 0 ||
            remote < 0){
        LOG( "[server] can not get the json value , %d\n", __LINE__);
        cJSON_Delete(root);
        return -1;
    }
    LOG( "[server] unbind cmd come from %s\n", remote ? "remote":"local");
    if(!remote)
        system("/etc/init.d/subservice stop");
    return 0;
}

int32_t do_unbind_internal(char *router_object,char *result,int32_t notify_binder)
{
    LOG( "[server] %s\n", __func__);
    int32_t ret = 0;
    //prepare msg
    char routerSubId[32] = "";
    getRouterValueUci("subid",routerSubId);
    cJSON *root=cJSON_CreateObject();
    cJSON_AddStringToObject(root, "routerid", router_object);
    cJSON_AddNumberToObject(root, "msgid", 0);
    cJSON_AddStringToObject(root, "subid", routerSubId);
    cJSON_AddStringToObject(root, "userid", "0");
    cJSON_AddNumberToObject(root, "type", 1); //type == 1 means unbind
    char tmp[256];
    sprintf(tmp, "{\"routerid\":\"%s\"}", router_object);
    cJSON_AddStringToObject(root, "data", tmp);

    cJSON *json=cJSON_CreateObject();
    char *router_message = cJSON_Print(root);
    cJSON_AddStringToObject(json,"message",router_message);
    cJSON_Delete(root);
    cJSON_AddStringToObject(json,"routerid",router_object);
    cJSON_AddStringToObject(json, "version", slversion);
    char *json_string = cJSON_Print(json);
    LOG( "[server]json_string : %s\n", json_string);

    //send unbind message with remote function
    struct HttpResonseData response;
    response.size = 0;
    char url[256] = "";

    sprintf(url,"%s/%s",XCLOUD_REMOTE_FUNCTION_ADDR,XCLOUD_REMOTE_FUNCTION_UNBIND);
    int32_t ret3 = postHttps(url,json_string,&response);
	LOG("unbind with new function\n");

    LOG("post unbind message---ret= %d --response=%s\n",ret3,(response.size > 0) ? (char*)response.data : "");
    if(ret3 != CURLE_OK || response.size == 0){
        LOG("post unbind message fail ret= %d\n",ret3);
        if(result) sprintf(result, "post unbind message fail ret= %d\n",ret3);
        ret = -1;
        goto CLEAN;
    }else{
        cJSON *response_json = cJSON_Parse((char *)response.data);
		if(response_json){
			cJSON *result_code_obj = cJSON_GetObjectItem(response_json, "code");
			if(result_code_obj){
				int32_t result_code = -1;
				cJSON_GetValueInt(result_code_obj, &result_code);
				if(result_code != 0){
					ret = -1;
					LOG( "[ssst]===function:%s===resule_code=%d", __func__, result_code);
					goto CLEAN;
				}
			}else{
				ret = -1;
				LOG( "[ssst]===function:%s===has no code item in result!", __func__);
				goto CLEAN;
            }
			cJSON_Delete(response_json);
        }else{
            ret = -1;
            LOG( "[ssst]===function:%s===parse response.data failed!", __func__);
            goto CLEAN;
        }
    }
    //delete local bind information
    delete_bind(0);
CLEAN:
    if(response.size > 0) free(response.data);
    if(router_message) free(router_message);
    if(json_string) free(json_string);
    cJSON_Delete(json);
    return ret;
}

void do_unbind(char *data, char **callback)
{
    LOG( "[server]%s, %s\n", __func__, data ? data : "NULL");
    if(!data){
        LOG("[server] args is NULL!\n");
        return;
    }
    char tmp[256];
    int32_t ret = -1;
    char router_object[32] = "";
    /*1,Notify all Managers that the router will be unbinded:
        1)find the all Managers from router table,
        2)use the Manager table's object id to search the UserSub table,
        3)create a Router unbind Message , send the Router Message to the UserSub table
     */
    char pending[20] = "";
    memset(pending, 0, 20);
    getRouterValueUci("pending",pending);
    int32_t ispending = 0;
    if(strcmp(pending, "1") == 0){
        ispending = 1;
    }else{
        ispending = 0;
    }

    if(ispending == 0 && has_binded() < 0){
        LOG( "[server] has been unbinded!\n");
        strcpy(tmp, "has been unbinded already!");
        goto DONE;
    }

    if(ispending){
        LOG( "[server]===ispending==%d==the sync_process have not finished===", ispending);
        strcpy(tmp, "the sync_process have not finished");
        goto DONE;
    }

    if(get_table(router_object, TABLE_ROUTER, -1) < 0){
        LOG( "[server]can not get router table objectid!\n");
        strcpy(tmp, "can not find router object!");
        goto DONE;
    }
    //2 clear binder and managers and notify them with router message
    ret = do_unbind_internal(router_object,tmp,callback ? 1: 0);
    if(ret != 0){
        sprintf(tmp, " do_unbind_internal-ret = %d", ret);
        goto DONE;
    }
    //if clean up subservice failed, do not care
    if(clean_subservice(data) < 0){
        LOG( "[server] clean subservice failed!\n");
    }
    setRouterValueToUci("cloudrouter", "pending", "0");
DONE:
    if(callback){
        cJSON *root =cJSON_CreateObject();
        cJSON_AddStringToObject(root, "ret", ret < 0 ? "fail" : "success");
        if(ret < 0){
            cJSON_AddStringToObject(root, "reason", tmp);
        }
        *callback = cJSON_Print(root);
        cJSON_Delete(root);
    }
}

void prepareSocketCallback(char **callback,char *ret,char *reason)
{
    if(callback){
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "ret", ret);
        cJSON_AddStringToObject(root, "reason", reason);
        *callback = cJSON_Print(root);
        cJSON_Delete(root);
    }
}

int32_t prepareBindData(char *binderid, char **bind_data, char* deviceid)
{
    LOG( "[ssst]===function:%s===binderid=%s: len %d===", __func__, binderid,strlen(binderid));
    LOG( "[ssst]===function:%s===deviceid=%s: len %d===", __func__, deviceid,strlen(deviceid));
    int32_t res = 0;
    int32_t ret_router = 0;
	/*
    int32_t ret_sub = 0;
    int32_t ret_state = 0;
    int32_t ret_wifi = 0;
    int32_t ret_device = 0;
#ifdef ZIGBEE_SUPPORT
    int32_t ret_zigbee_device_basic = 0;
    int32_t ret_zigbee_device_status = 0;
    int32_t ret_zigbee_device_record = 0;
    int32_t ret_zigbee_rule_record = 0;
    int32_t ret_zigbee_rule = 0;
#endif
*/
    cJSON *root = cJSON_CreateObject();
    if(root){
        router_table router;
        ret_router = add_router_to_json(root, 0, router);
/*
        router_sub_table routersub;
        router_state_table routerstate;
        ret_router = add_router_to_json(root, 0, router);
        ret_sub = add_router_sub_to_json(root, 0, routersub);
        ret_state = add_router_state_to_json(root, 0, routerstate);
        ret_wifi = add_wifi_info_to_json(root, 0, NULL, 0);
        ret_device = add_devices_info_to_json(root, 0, NULL, 0);
#ifdef ZIGBEE_SUPPORT
        ret_zigbee_device_basic = add_zigbee_device_basic_info_to_json(root, 0, NULL, 0);
        ret_zigbee_device_status = add_zigbee_device_status_info_to_json(root, 0, NULL, 0);
        ret_zigbee_device_record = add_zigbee_device_record_info_to_json(root, 0, NULL, 0);
        ret_zigbee_rule_record = add_zigbee_rule_record_info_to_json(root, 0, NULL, 0);
        ret_zigbee_rule = add_zigbee_rule_info_to_json(root, 0, NULL, 0);
#endif
*/
        char sn[50];
        if(getSfHardwareConfig("sn",sn) != 0){
            LOG( "[server] get sn from hardware config fail!\n");
            return -1;
        }
        cJSON_AddStringToObject(root, "sn", sn);
		if(strlen(binderid) > 0)
		  cJSON_AddStringToObject(root, "binderid", binderid);
		else
		  if(strlen(deviceid) > 0)
			cJSON_AddStringToObject(root, "binddeviceid", deviceid);

/*
        cJSON_AddStringToObject(root, "version", slversion);
*/
        *bind_data = cJSON_PrintUnformatted(root);
        if(*bind_data){
            res = 0;
        }else{
            LOG( "prepare Bind Data failed!!!");
            res = -1;
        }
        cJSON_Delete(root);
    }else{
        LOG( "[ssst]===function:%s====maybe no enough memory =====Create cJSON Object failed!!!=====", __func__);
        res = -1;
    }
	/*
#ifdef ZIGBEE_SUPPORT
    LOG( "[ssst]===function:%s======ret_router=%d,ret_sub=%d,ret_state=%d,ret_wifi=%d,ret_device=%d,ret_zigbee_device_basic=%d===", __func__, ret_router, ret_sub, ret_state, ret_wifi, ret_device, ret_zigbee_device_basic);
    return res||ret_router||ret_sub||ret_state||ret_wifi||ret_device||ret_zigbee_device_basic;
#else
    LOG( "[ssst]===function:%s======ret_router=%d,ret_sub=%d,ret_state=%d,ret_wifi=%d,ret_device=%d===", __func__, ret_router, ret_sub, ret_state, ret_wifi, ret_device);
    return res||ret_router||ret_sub||ret_state||ret_wifi||ret_device;
#endif
*/
	LOG( "[ssst]===function:%s======ret_router %d\n",__func__, res);
	return res;
}

int32_t doAddRouterUser(char *args, char **callback)
{
    int32_t res = 0;
    char tmp[1024] = "";
    LOG("------[server]%s, args : %s\n",__func__, args ? args : "NULL");
    if(!args){
        LOG("[server] args is null!\n");
        return -1;
    }

    char *data = args;
    char binderid[32];
    char device_id[32];
    memset(binderid, '\0', 32);
    memset(device_id, '\0', 32);
    cJSON *root = cJSON_Parse(data);
    if(root){
        if((get_json_value_string(root,"binder",binderid) != 0) ){
            LOG( "[server] check add router user args --can not find binder id!\n");
			if(get_json_value_string(root,"deviceid",device_id) != 0){
				LOG( "[server] check add router user fail--can not find binder or device id !\n");
				prepareSocketCallback(callback,"fail","check add router user fail--can not find binder or device id !");
				return -1;
			}
        };
        cJSON_Delete(root);
    }else{
        return -1;
    }
	char *json_str = NULL;
	char sn_str[50];
	if(getSfHardwareConfig("sn",sn_str) != 0){
		LOG( "[server] func %s get sn from hardware config fail!\n", __func__);
		res = -1;
		strcpy(tmp, " prepare add user router sn failed!");
	}
	else{
		LOG("function:%s sn=%s bind id=%s userid=%s\n", __func__, sn_str, binderid, device_id);
		cJSON *root = cJSON_CreateObject();
		if(root){
			cJSON_AddStringToObject(root, "sn", sn_str);
			if(strlen(binderid) > 0)
			  cJSON_AddStringToObject(root, "userid", binderid);
			else
			  if(strlen(device_id) > 0)
				cJSON_AddStringToObject(root, "deviceid", device_id);

			json_str = cJSON_PrintUnformatted(root);
			if(json_str){
				res = 0;
			}else{
				LOG( "prepare add router user Data failed!!!");
				res = -2;
			}
			cJSON_Delete(root);
		}else{
			LOG( "[ssst]===function:%s====maybe no enough memory =====Create cJSON Object failed!!!=====", __func__);
			res = -1;
		}
		LOG("prepare data ok\n");
		LOG("strlen(json_str) ======= %d",strlen(json_str));
		LOG("content  ======= %s",json_str);
		//use remote function to handle this message
		char url[256] = "";
		sprintf(url,"%s/%s",XCLOUD_REMOTE_FUNCTION_ADDR,XCLOUD_REMOTE_FUNCTION_ADD_ROUTER_USER);
		struct HttpResonseData response;
		response.size = 0;
		LOG("publish event--data=%s\n",(char*)data);
		int32_t ret3 = -1;
		int32_t try_count = 0;
		for(try_count = 0; try_count<3; try_count++){
			ret3 = postHttpCommon(url,json_str,&response,1,1);
			LOG("publish event---try_time= %d\n", try_count);
			LOG("publish event---ret= %d --response=%s\n",ret3,(response.size > 0) ? (char*)response.data : "");
			if(ret3 == CURLE_OK){
				break;
			}
		}
		free(json_str);

		LOG("respnse data %s \n",(char *)response.data);

		if(ret3 != CURLE_OK || response.size == 0){
			LOG("publish add router user fail ret= %d\n",ret3);
			if(response.size > 0) free(response.data);
			res = -2;
			strcpy(tmp, " add user router call server failed!");
		}
		//TODO: process response data
	}

	if(callback){
		cJSON *root =cJSON_CreateObject();
		if(root){
			cJSON_AddStringToObject(root, "ret", res < 0 ? "fail" : "success");
			if(res < 0){
				cJSON_AddStringToObject(root, "reason", tmp);
			}
			*callback = cJSON_Print(root);
			cJSON_Delete(root);
		}else{
			LOG( "[ssst]===function:%s====cJSON_CreateObject root failed!!!===", __func__);
		}
	}

	return 0;
}

void do_bind(char *args, char **callback)
{
	int32_t res = 0;
	char tmp[1024] = "";
	LOG("------[server]%s, args : %s\n",__func__, args ? args : "NULL");
	if(!args){
		LOG("[server] args is null!\n");
		return;
	}

	char *data = args;
	char binderid[32];
	char device_id[32];
	memset(binderid, '\0', 32);
	memset(device_id, '\0', 32);
	cJSON *root = cJSON_Parse(data);
	if(root){
		if((get_json_value_string(root,"binder",binderid) != 0) ){
			LOG( "[server] check binder args --can not find binder id!\n");
			if(get_json_value_string(root,"deviceid",device_id) != 0){
				LOG( "[server] check binder args fail--can not find binder or device id !\n");
				prepareSocketCallback(callback,"fail","check binder args fail--can not find binder or device id !");
				return;
			}
		};
		cJSON_Delete(root);
	}else{
		return;
	}

	router_table rt;
	router_sub_table subt;
	router_state_table rst;
	memset(&rt,0,sizeof(rt));
	memset(&subt,0,sizeof(subt));
	memset(&rst,0,sizeof(rst));

	wireless_table *wt = NULL;
	device_table *dst = NULL;
	//int32_t devnum = 0;
	//int32_t wifinum = 0;
	int32_t bind = 0;


	char pending[20] = "";
	memset(pending, 0, 20);
	getRouterValueUci("pending",pending);
	int32_t ispending = 0;
	if(strcmp(pending, "1") == 0){
		ispending = 1;
	}else{
		ispending = 0;
	}

	char *bind_data = NULL;
	if(!ispending && !has_binded()){      //pending=0 and the router has binded
		LOG( "[server] has been binded already!\n");
		prepareSocketCallback(callback,"fail","has been binded already!");
		return;
	}

	if(ispending){     //pending=1 means the sync_process have not finished
		LOG( "[server]===ispending==%d==the sync_process have not finished===", ispending);
		prepareSocketCallback(callback,"fail","the sync_process have not finished!");
		return;
	}

	if(prepareBindData(binderid, &bind_data, device_id) == 0){
		struct HttpResonseData response;
		char url[256] = "";
		int32_t ret = 0;
		sprintf(url,"%s/%s",XCLOUD_REMOTE_FUNCTION_ADDR,XCLOUD_REMOTE_FUNCTION_BIND);
		ret = postHttps(url, bind_data, &response);
		LOG("bind with new function %s \n",bind_data);

		sprintf(tmp, "ret=%d", ret);
		if(ret != CURLE_OK){
			LOG( "function:%s-------postHttps failed==ret==%d\n", __func__, ret);
			res = -1;
		}else{
			LOG( "function:%s-------post bind data===%s\n", __func__, bind_data);
			if(response.size>0){
				LOG( "=======response.data=====%s====\n", (char *)response.data);
				//response data format like this {"result":"{\"result\":\"success\",\"msg\":{\"bind\":1,\"routerid\":\"5c82f54cbc\",\"device\":[\"8571093e49\",\"fe12d38488\",\"499dc24293\"],\"devnum\":3,\"wifi\":[\"1d1f8ac202\",\"e8de8452e4\"],\"wifinum\":2,\"subid\":\"98ad6697c1\",\"stateid\":\"8cc4930691\",\"binder\":\"a81b324a27\"}}"/}
				//or if the bind_data in incorrect format the response data format like this {"result":"there is an error in callback function"}
				//replace '\"' with '"'
				char *msgBuffer = malloc(response.size);
				//str_replace(msgBuffer,response.data,"\\\"","\"");
				memcpy(msgBuffer,(char *)response.data,response.size);

				LOG( "-----get iHomeCloud response =%s\n", msgBuffer);
				cJSON *response_json = cJSON_Parse(msgBuffer);

				if(response_json){
					cJSON *result_code_json = cJSON_GetObjectItem(response_json, "code");
					if(result_code_json){
						char result_code_str[32] = "";
						memset(result_code_str, 0, 32);
						cJSON_GetValueString(result_code_json, result_code_str);
						int32_t code = atoi(result_code_str);
						if(code == 0){
							cJSON *result_data_json = cJSON_GetObjectItem(response_json, "data");
							if(result_data_json){
								cJSON *routerid_json = cJSON_GetObjectItem(result_data_json, "routerid");
								cJSON_GetValueString(routerid_json, rt.objectid);
/*
								char devnum_str[32] = "";
								cJSON *devnum_json = cJSON_GetObjectItem(result_data_json, "devnum");
								cJSON_GetValueString(devnum_json, devnum_str);
								devnum = atoi(devnum_str);

								LOG( "devnum====%d", devnum);
								cJSON *device_array_json = cJSON_GetObjectItem(result_data_json, "device");
								cJSON *device_json_tmp = NULL;
								int32_t device_count = 0;
								dst = (device_table *)malloc(devnum*sizeof(device_table));
								for(device_count=0; device_count<devnum; device_count++){
									device_json_tmp = cJSON_GetArrayItem(device_array_json, device_count);

									cJSON_GetValueString(device_json_tmp, dst[device_count].objectid);

								}
								char wifinum_str[32] = "";
								cJSON *wifinum_json = cJSON_GetObjectItem(result_data_json, "wifinum");
								cJSON_GetValueString(wifinum_json, wifinum_str);
								wifinum = atoi(wifinum_str);

								cJSON *wifi_array_json = cJSON_GetObjectItem(result_data_json, "wifi");
								cJSON *wifi_json_tmp = NULL;
								int32_t wifi_count = 0;
								LOG( "wifinum====%d", wifinum);
								wt = (wireless_table *)malloc(wifinum*sizeof(wireless_table));
								for(wifi_count=0; wifi_count<wifinum; wifi_count++){
									wifi_json_tmp = cJSON_GetArrayItem(wifi_array_json, wifi_count);
									cJSON_GetValueString(wifi_json_tmp, wt[wifi_count].objectid);
								}

*/
								cJSON *subid_json = cJSON_GetObjectItem(result_data_json, "subid");
								cJSON_GetValueString(subid_json, subt.objectid);
/*
								cJSON *stateid_json = cJSON_GetObjectItem(result_data_json, "stateid");
								cJSON_GetValueString(stateid_json, rst.objectid);
*/
								cJSON *binderid_json = cJSON_GetObjectItem(result_data_json, "binder");
								cJSON_GetValueString(binderid_json, binderid);

								//bind success if bind is not empty
								if(strcmp(binderid,"")) bind = 1;

								//if(save_bind(&rt, &subt, &rst, wt, wifinum, dst, devnum, binderid, bind) < 0){
								if(save_bind(&rt, binderid, bind) < 0){
									LOG( "[server]save_bind failed\n");
									res = -1;
									strcpy(tmp, "save_bind failed");
									//        goto DONE;
								}
								if(dst) free(dst);
								if(wt) free(wt);
							}else{
								res = -1;
								LOG( "[ssst]===function:%s----paser date json fail", __func__);
								sprintf(tmp, "paser date json fail");
							}
						}else{
							res = -1;
							LOG( "[ssst]===function:%s--result code is not success--code=%d\n", __func__, code);
							sprintf(tmp, " result code is not success--code=%d", code);
						}
					}else{
						res = -1;
						LOG( "[ssst]===function:%s--code json not exist", __func__);
						sprintf(tmp, "code json not exist");
					}
					cJSON_Delete(response_json);
				}else{
					res = -1;
					LOG( "[ssst]===function:%s----parse result_json failed!", __func__);
					sprintf(tmp, " parse response json failed");
				}
				if(msgBuffer != NULL){
					free(msgBuffer);
					msgBuffer = NULL;
				}
			}else{
				res = -1;
				LOG( "[ssst]===function:%s=====response.size=%d========", __func__, response.size);
				sprintf(tmp, " response.size=%d", response.size);
			}
		}
	}else{
		res = -1;
		LOG( "[ssst]===function:%s===prepare bind data failed !!!===", __func__);
		strcpy(tmp, " prepare bind data failed!");
	}
	if(bind_data) free(bind_data);
	if(res == 0){
		setRouterValueToUci("cloudrouter", "pending", "0");
		system("/etc/init.d/subservice start");
	}
	LOG( "[server]---------bind done--1\n");
	if(callback){
		cJSON *root =cJSON_CreateObject();
		if(root){
			cJSON_AddStringToObject(root, "ret", res < 0 ? "fail" : "success");
			if(res < 0){
				cJSON_AddStringToObject(root, "reason", tmp);
			}else{
				cJSON_AddStringToObject(root, "router", rt.objectid);
				cJSON_AddStringToObject(root, "routersub", subt.objectid);
			}
			*callback = cJSON_Print(root);
			cJSON_Delete(root);
		}else{
			LOG( "[ssst]===function:%s====cJSON_CreateObject root failed!!!===", __func__);
		}
	}

	LOG( "[server]---------bind done--2\n");
}

int32_t do_manager_op_internel(char *user_json,char *error)
{
	return handleUserManagerAction(user_json,error);
}

void do_manager_op(char *args, char **callback)
{
	LOG("[server]%s, args : %s\n",__func__, args ? args : "NULL");
	if(!args){
		LOG("[server] args is null!\n");
		return;
	}
	char tmp[256] = "";
	char *data = args;
	int32_t ret = do_manager_op_internel(data,tmp);
	if(callback){
		cJSON *root =cJSON_CreateObject();
		cJSON_AddStringToObject(root, "ret", ret < 0 ? "fail" : "success");
		if(ret < 0) cJSON_AddStringToObject(root, "reason", tmp);
		*callback = cJSON_Print(root);
		cJSON_Delete(root);
	}
}
