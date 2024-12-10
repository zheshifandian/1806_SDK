/*
 * =====================================================================================
 *
 *       Filename:  sf_factory_read.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2018年11月14日 17时52分09秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  star (), star.jiang@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */
#ifndef __SF_FACTORY_READ_H__
#define __SF_FACTORY_READ_H__
#include <syslog.h>
#include "utils.h"

enum sf_factory_read_action {
	READ_MAC_ADDRESS,
	READ_SN,
	READ_SN_FLAG,
	READ_WAN_MAC_ADDRESS,
	READ_WIFI_LB_MAC_ADDRESS,
	READ_WIFI_HB_MAC_ADDRESS,
	READ_FIRST_MAC_ADDRESS,
	READ_VENDER,
	READ_VENDER_FLAG,
	READ_PRODUCT_KEY,
	READ_PRODUCT_KEY_FLAG,
};

int sf_factory_read_operation(enum sf_factory_read_action action, char* data, int len);
#endif
