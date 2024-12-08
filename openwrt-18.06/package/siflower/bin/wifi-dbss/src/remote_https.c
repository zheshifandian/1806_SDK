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

#include <stdlib.h>
#ifdef siwifi_dbss_curl
#include <curl/curl.h>
#include <curl/easy.h>
#endif
#include <stdio.h>
#include <string.h>
#include <syslog.h>

struct HttpResonseData{
	size_t size;
	void *data;
	struct HttpResonseData *next;
};

size_t OnHttpReponseData(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
	if(size * nmemb > 0){
		char *str = realloc(((struct HttpResonseData*)lpVoid)->data, ((struct HttpResonseData*)lpVoid)->size+size*nmemb);
		memcpy(str+((struct HttpResonseData*)lpVoid)->size,buffer,size*nmemb);
		syslog(LOG_CRIT, "size=%d,nmemb=%d========size*nmemb=%d", size, nmemb, size * nmemb);
		if(lpVoid){
			((struct HttpResonseData*)lpVoid)->size += size * nmemb;
			((struct HttpResonseData*)lpVoid)->data = str;
		}else{
			free(str);
		}
	}
	return size*nmemb;
}

#ifdef siwifi_dbss_curl
int32_t postHttps(char *url,void *data,void *response)
{
	struct HttpResonseData *response_tmp = (struct HttpResonseData *)response;
	int32_t res;
	long responseCode;
	response_tmp->data = NULL;
	response_tmp->size = 0;
	CURL* curl;

	long time1 = time(NULL);
	struct curl_slist *slist = NULL;

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

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 6);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

	slist = curl_slist_append(slist, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);

	res = curl_easy_perform(curl);

	response_tmp->data = realloc(response_tmp->data, response_tmp->size+1);
	((char *)(response_tmp->data))[response_tmp->size] = '\0';

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
	if((responseCode % 200) > 100 || responseCode >= 400){
		syslog(LOG_CRIT, "postHttps http check response code fail %d \n",(int)responseCode);
		syslog(LOG_CRIT, "data is %s\n", (char*)response_tmp->data);
		res = -1;
	}

	syslog(LOG_CRIT, "post http resonse code %ld cost %ld s url: %s\n",responseCode,time(NULL) - time1,url);
	curl_easy_cleanup(curl);
	curl_slist_free_all(slist);
	return res;
}
#endif
