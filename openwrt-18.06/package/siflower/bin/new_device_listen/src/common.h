/*
 * =====================================================================================
 *
 *       Filename:  common.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2015年11月18日 14时22分01秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  kevin.feng (), kevin.feng@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */


#ifndef COMMON_H
#define COMMON_H


#define ID_BUF_S 256
#define MANAGERFILE "/etc/config/simanager"

struct HttpResonseData{
    size_t size;
    void *data;
    long code;
};

#endif
