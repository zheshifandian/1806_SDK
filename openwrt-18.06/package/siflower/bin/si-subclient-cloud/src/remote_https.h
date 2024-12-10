/*
 * =====================================================================================
 *
 *       Filename:  remote_https.h
 *
 *    Description:  common http util for https
 *
 *        Version:  1.0
 *        Created:  2015年09月07日 14时31分39秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  robert (), robert.chang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */
#ifndef REMOTE_HTTPS_H
#define REMOTE_HTTPS_H

#include <stdlib.h>

#define COOKIE_PATH "/tmp/subCloudCookie"
int32_t postHttps(char *url,void *data,void *response, int usetoken);

size_t OnHttpReponseData(void* buffer, size_t size, size_t nmemb, void* lpVoid);
#endif
