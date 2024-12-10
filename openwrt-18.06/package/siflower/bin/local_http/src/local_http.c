/*
 * =====================================================================================
 *
 *       Filename:  local_http.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2015年08月12日 15时54分19秒
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
#include <curl/curl.h>
#include <curl/easy.h>
#include <shadow.h>
#include <regex.h>
#include <sys/time.h>
#include "local_http.h"
//#include "utils.h"

#define USER_COMMON "common"

//time out for communicate with uhttpd
#define TIMEOUT_UHTTPD_S 20

#define UHTTPD_REDIRECT_URL_CODE 302

struct LuciUserCookie{
    char userId[32];
    char cookieSession[128];
    char cookiePath[256];
    struct LuciUserCookie *next;
};

struct LuciUserCookie *cookieCacheList = NULL;

void getSubString(char *stbuf,const char*str, unsigned start, unsigned end)
{
    unsigned n = end - start;
    memcpy(stbuf, str + start, n);
    *(stbuf + n) = 0;
}

//join HttpResonseData link list to one data
struct HttpResonseData *JoinHttpResponseData(struct HttpResonseData *response)
{
	//link all resposne data to one
	//get length
	int32_t total_len = 0;
	struct HttpResonseData *child = response;
	while(child){
		total_len += child->size;
		child = child->next;
	}

	//re malloc data
	struct HttpResonseData *total = (struct HttpResonseData *)malloc(sizeof(struct HttpResonseData));
	total->size = total_len + 1;
	total->data = malloc(total->size);
	*((char*)(total->data) + total_len) = '\0';

	//copy data
	child = response;
	int32_t offset = 0;
	while(child){
		memcpy((char*)(total->data) + offset,(char*)(child->data),child->size);
		offset += child->size;
		child = child->next;
	}
	//free response link list
	child = response;
	struct HttpResonseData *current = NULL;
	while(child){
		current = child;
		child = child->next;
		LOG("current %p current-data %p \n",current,current->data);
		if(current->data) free(current->data);
		free(current);
	}
	return total;
}


size_t OnLocalHttpReponseData(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    if(size * nmemb > 0){
        char *str = realloc(((struct HttpResonseData*)lpVoid)->data, ((struct HttpResonseData*)lpVoid)->size+size*nmemb);
        memcpy(str+((struct HttpResonseData*)lpVoid)->size,buffer,size*nmemb);
#if KEVIN_TEST
        int32_t tmp_count = 0;
        int32_t tmp_num = size*nmemb/50+1;
        char tmp_buf[51]="";
        for(tmp_count=0; tmp_count<tmp_num; tmp_count++){
            memcpy(tmp_buf, str+((struct HttpResonseData*)lpVoid)->size+tmp_count*50, 50);
            tmp_buf[50] = '\0';
            usleep(100);
        }
#endif
        if(lpVoid){
            ((struct HttpResonseData*)lpVoid)->size += size * nmemb;
            ((struct HttpResonseData*)lpVoid)->data = str;
        }else{
            free(str);
        }
    }
    return size*nmemb;
}

size_t OnLocalHttpHeaderData(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    if(size * nmemb > 0){
        char *str = malloc(size*nmemb + 1);
        memcpy(str,buffer,size*nmemb);
        *(str + size*nmemb) = '\0';
        //LOG("get result=%s\n",str);
        if(lpVoid && strstr(str,"Set-Cookie")){
            ((struct HttpCookieData*)lpVoid)->size = size * nmemb;
            ((struct HttpCookieData*)lpVoid)->data = str;
        }else{
            free(str);
        }
    }
    return size*nmemb;
}

void getCookieFromCache(char *userId,char *cookie_path,char *cookie_session)
{
    struct LuciUserCookie *cookie = cookieCacheList;
    while(cookie){
        if(!strcmp(userId,cookie->userId)){
            //find it
            strcpy(cookie_path,cookie->cookiePath);
            strcpy(cookie_session,cookie->cookieSession);
            break;
        }
        cookie = cookie->next;
    }
}

void storeCookie(char *userId,char *cookie_session,char *cookie_path)
{
    struct LuciUserCookie *cookie = cookieCacheList;
    struct LuciUserCookie *pre = cookieCacheList;
    while(cookie){
        if(!strcmp(userId,cookie->userId)){
            //find it
            break;
        }
        pre = cookie;
        cookie = cookie->next;
    }
    if(!cookie){
        cookie = malloc(sizeof(struct LuciUserCookie));
        strcpy(cookie->userId,userId);
        cookie->next = NULL;
        if(pre) pre->next = cookie;
        if(!cookieCacheList) cookieCacheList = cookie;
    }
    strcpy(cookie->cookiePath,cookie_path);
    strcpy(cookie->cookieSession,cookie_session);
}

void cleanCookieCache()
{
    struct LuciUserCookie *cookie = cookieCacheList;
    struct LuciUserCookie *pre = cookieCacheList;
    while(cookie){
        pre = cookie->next;
        free(cookie);
        cookie = pre;
    }
    cookieCacheList = NULL;
}

int32_t parseCookie(char *cookieData,char *sysauth,char *path)
{
    static const char *STR_SSST_PATTERN = "([^\f\n\r\t\v]*)(sysauth=[a-z0-9A-F]+)([^\n\r\t\v]*)path=([^\040\n\r\t\v]*;stok=[a-z0-9A-F]+)([^\n\r\t\v]*)?";
    regex_t regex;
    int32_t errcode = regcomp(&regex, STR_SSST_PATTERN , REG_EXTENDED);
    if(errcode != 0){
        LOG("regcomp pattern error %d\n",errcode);
        return -1;
    }
    regmatch_t value[6];
    errcode = regexec(&regex, (char *)cookieData,6, value, 0);
    if(errcode != 0){
        LOG("regexec does't match error %d\n",errcode);
        return -1;
    }
    if(value[2].rm_so != -1){
        getSubString(sysauth,cookieData,value[2].rm_so,value[2].rm_eo);
    }
    if(value[4].rm_so != -1){
        getSubString(path,cookieData,value[4].rm_so,value[4].rm_eo);
    }
    regfree(&regex);
    return 0;
}

#define LOCAL_HTTPD_PORT 1688

int postDataToHttpd(char *command,char *postData,void *response,char *userId,int32_t timeout, int32_t count)
{
    struct HttpResonseData *response_tmp = (struct HttpResonseData *)response;
    response_tmp->data = NULL;
    response_tmp->size = 0;
    struct timeval time1;
    gettimeofday(&time1,NULL);
    CURLcode ret;
    CURL* curl = curl_easy_init();
    if(NULL == curl) return CURLE_FAILED_INIT;
    char url[256] = "";
	sprintf(url,"http://127.0.0.1/cgi-bin/luci%s",command);
    //2 prepare auth
    //get local passwd info to pass session authentication if token and cookie verify fail
    struct spwd *sp;
    sp = getspnam("sfroot");
    char auth[256] = "";
    sprintf(auth,"Authorization: %s",(sp != NULL) ? sp->sp_pwdp : "");
    if(!sp) LOG("get sp root shadow fail\n");

    //LOG("postDataToHttpd--url=%s postData=%s\n",url, postData);
    //set headers
    struct curl_slist *slist = NULL;
    //use json format encode
    slist = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");
    slist = curl_slist_append(slist, auth);
    char cookie_str[256] = "";
    //always pass userid to avoid if session is up to date and need refresh
	sprintf(cookie_str,"userid: %s",userId);
    slist = curl_slist_append(slist, cookie_str);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_PORT, LOCAL_HTTPD_PORT);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnLocalHttpReponseData);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,OnLocalHttpHeaderData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_tmp);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    // just to start the cookie engine
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, COOKIE_PATH);
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, COOKIE_PATH);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (timeout < 0) ? TIMEOUT_UHTTPD_S : timeout);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (timeout < 0) ? TIMEOUT_UHTTPD_S : timeout);
	ret = curl_easy_perform(curl);

	response_tmp->data = realloc(response_tmp->data, response_tmp->size+1);
    ((char *)(response_tmp->data))[response_tmp->size] = '\0';
    //LOG("response_tmp->data===%p,response_tmp->data===%s, response_tmp->size===%d", response_tmp->data, (char *)response_tmp->data, response_tmp->size);
	response_tmp->size += 1;
    long responseCode;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    struct timeval time2;
    gettimeofday(&time2,NULL);
    //LOG("perform2 curl_easy_perform------2----responsecode %ld cost time %ld ms\n",responseCode,(time2.tv_sec - time1.tv_sec) * 1000 + (time2.tv_usec - time1.tv_usec) / 1000);
    curl_easy_cleanup(curl);
    curl_slist_free_all(slist);
    //check if uhttpd server is ok
    if(ret == CURLE_COULDNT_CONNECT){
        //restart it
        system("/etc/init.d/uhttpd restart");
        LOG("fuck restart uhttpd now\n");
    }
	//add by edward: if get the redirect url response, post the command to httpd once again.
	if((responseCode == UHTTPD_REDIRECT_URL_CODE) && count){
        LOG("CURLINFO_REDIRECT_URL, post the command to httpd once again!\n");
		if(response_tmp->data != NULL){
			free(response_tmp->data);
			response_tmp->data = NULL;
		}
		return postDataToHttpd(command,postData,response,userId,timeout,count - 1);
	}
    return ret;
}

int postDataToHttpdCommon(char *command,char *postData,void *response)
{
    return postDataToHttpd(command,postData,response,USER_COMMON,-1,1);
}
