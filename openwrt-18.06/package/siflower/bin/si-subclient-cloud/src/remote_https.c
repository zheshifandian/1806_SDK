/*
 * =====================================================================================
 *
 *       Filename:  remote_https.c
 *
 *    Description:  implement for https methord
 *
 *        Version:  1.0
 *        Created:  2015年09月07日 14时53分12秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert , robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */

#include "local_http.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <string.h>
#include "cloud_common.h"
#include "remote_https.h"


size_t OnHttpReponseData(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    if(size * nmemb > 0){
        char *str = realloc(((struct HttpResonseData*)lpVoid)->data, ((struct HttpResonseData*)lpVoid)->size+size*nmemb);
        memcpy(str+((struct HttpResonseData*)lpVoid)->size,buffer,size*nmemb);
        LOG("size=%d,nmemb=%d========size*nmemb=%d", size, nmemb, size * nmemb);
        if(lpVoid){
            ((struct HttpResonseData*)lpVoid)->size += size * nmemb;
            ((struct HttpResonseData*)lpVoid)->data = str;
        }else{
            free(str);
        }
    }
    return size*nmemb;
}

int32_t postHttps(char *url,void *data,void *response, int usetoken)
{
    struct HttpResonseData *response_tmp = (struct HttpResonseData *)response;
	int32_t res;
	int8_t i = 0;
    long responseCode;
	int32_t errcode;
	char token_tmp[288];
    response_tmp->data = NULL;
    response_tmp->size = 0;
    CURL* curl;

    long time1 = time(NULL);
    struct curl_slist *slist = NULL;
    char header[100] = "";

try_again:
	curl = curl_easy_init();
    if(NULL == curl) return CURLE_FAILED_INIT;
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4 );
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnHttpReponseData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_tmp);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    // just to start the cookie engine
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, COOKIE_PATH);
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, COOKIE_PATH);

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 6);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

    slist = curl_slist_append(slist, "Content-Type: application/json");
	if(usetoken == 1){
		sprintf(token_tmp,"Authorization:Bearer %s",token);
		slist = curl_slist_append(slist, token_tmp);
	}
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);

    res = curl_easy_perform(curl);

    response_tmp->data = realloc(response_tmp->data, response_tmp->size+1);
    ((char *)(response_tmp->data))[response_tmp->size] = '\0';

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    if((responseCode % 200) > 100 || responseCode >= 400){
        LOG("postHttps http check response code fail %d \n",(int)responseCode);
		if(usetoken == 1 && responseCode == 401){
			if(parseIntFromData("code", response_tmp->data, &errcode) == 0){
				if(errcode == 405){
					if( i > 2 || GetToken() < 0){
						goto err;
					}
					i++;
					curl_easy_cleanup(curl);
					curl_slist_free_all(slist);
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
    LOG("post http resonse code %ld cost %ld s url: %s\n",responseCode,time(NULL) - time1,url);
    curl_easy_cleanup(curl);
    curl_slist_free_all(slist);
    return res;
}
