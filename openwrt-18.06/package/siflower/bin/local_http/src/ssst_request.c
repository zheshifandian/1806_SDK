/*
 * =====================================================================================
 *
 *       Filename:  ssst_request.c
 *
 *    Description:  implement common utils for ssst request
 *
 *        Version:  1.0
 *        Created:  2015年09月01日 09时36分25秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert (), robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#include "ssst_request.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "token.h"

int32_t postHttpCommon(char *url,void *data,void *response,int https,int usetoken)
{
    CURL* curl;
	int32_t errcode;
    long responseCode;
	struct HttpResonseData *response_tmp = (struct HttpResonseData *)response;
	int32_t res;
    struct timeval time1;
    gettimeofday(&time1,NULL);
    struct curl_slist *slist = NULL;
    char header[100] = "";
    struct timeval time2;
	char token_tmp[288];
	int8_t i = 0;

try_again:
	curl = curl_easy_init();
    if(NULL == curl) return CURLE_FAILED_INIT;
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4 );
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnHttpReponseData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

	if(https){
	    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
	}

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 6);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

    slist = curl_slist_append(slist, "Content-Type: application/json");
	if(usetoken == 1){
		sprintf(token_tmp,"Authorization:Bearer %s",token);
		slist = curl_slist_append(slist, token_tmp);
	}
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    if((responseCode % 200) > 100 || responseCode >= 400){
        LOG("postHttps http check response code fail %d \n",(int)responseCode);
		if(usetoken == 1 && responseCode == 401){
			LOG("401 response is %s\n",response_tmp->data);
			if(parseIntFromData("code", response_tmp->data, &errcode) == 0){
				if(errcode == 405){
					if( i > 2 || GetToken() < 0){
						goto err;
					}
					i++;
					curl_easy_cleanup(curl);
					LOG("free curl\n");
					curl_slist_free_all(slist);
					LOG("free slist\n");
					slist = NULL;
					free(response_tmp->data);
					response_tmp->data = NULL;
					response_tmp->size = 0;
					goto try_again;
				}
			}
		}
        res = -1;
    }
err:
    gettimeofday(&time2,NULL);
    LOG("post http resonse code %ld cost %ld ms --url: %s\n",responseCode,(time2.tv_sec - time1.tv_sec) * 1000 + (time2.tv_usec - time1.tv_usec) / 1000,url);
    curl_easy_cleanup(curl);
    curl_slist_free_all(slist);
    return res;
}

int32_t postHttp(char *url,void *data,void *response)
{
	return postHttpCommon(url,data,response,0,1);
}

int32_t postHttps(char *url,void *data,void *response)
{
	return postHttpCommon(url,data,response,1,1);
}

int postHttpFile(char *url,const char *filename, const char *sn, const char *text, const char *module, const char *code, const char *timestamp, void *response)
{
    CURL *curl;
    int32_t rcode;
    CURLFORMcode formCode;
    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;
    struct curl_slist *headers=NULL;

    rcode = curl_global_init(CURL_GLOBAL_NOTHING);
    if (CURLE_OK != rcode)
        goto err1;

    curl = curl_easy_init();
    if (NULL == curl)
        goto err2;
    formCode = curl_formadd(&formpost,&lastptr,CURLFORM_COPYNAME, "file",CURLFORM_FILE, filename, CURLFORM_END);
    if(formCode != CURL_FORMADD_OK){
        LOG("curl_formadd file [%d] error\n", formCode);
        //goto err1;
    }
    formCode = curl_formadd(&formpost,&lastptr,CURLFORM_COPYNAME, "sn",CURLFORM_COPYCONTENTS, sn, CURLFORM_END);
    if(formCode != CURL_FORMADD_OK){
        LOG("curl_formadd sn [%d] error\n", formCode);
        //goto err1;
    }
    formCode = curl_formadd(&formpost,&lastptr,CURLFORM_COPYNAME, "text",CURLFORM_COPYCONTENTS, text, CURLFORM_END);
    if(formCode != CURL_FORMADD_OK){
        LOG("curl_formadd text [%d] error\n", formCode);
        //goto err1;
    }
    formCode = curl_formadd(&formpost,&lastptr,CURLFORM_COPYNAME, "module",CURLFORM_COPYCONTENTS, module, CURLFORM_END);
    if(formCode != CURL_FORMADD_OK){
        LOG("curl_formadd module [%d] error\n", formCode);
        //goto err1;
    }
    formCode = curl_formadd(&formpost,&lastptr,CURLFORM_COPYNAME, "code",CURLFORM_COPYCONTENTS, code, CURLFORM_END);
    if(formCode != CURL_FORMADD_OK){
        LOG("curl_formadd code [%d] error\n", formCode);
        //goto err1;
    }
    formCode = curl_formadd(&formpost,&lastptr,CURLFORM_COPYNAME, "timestamp",CURLFORM_COPYCONTENTS, timestamp, CURLFORM_END);
    if(formCode != CURL_FORMADD_OK){
        LOG("curl_formadd timestamp [%d] error\n", formCode);
        //goto err1;
    }
    headers = curl_slist_append(headers, "Content-Type:multipart/form-data");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnHttpReponseData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

    rcode = curl_easy_perform(curl);
    curl_formfree(formpost);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    curl_global_cleanup();
    return 0;
err2:
    curl_global_cleanup();
err1:
        return -1;
}
