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
enum sf_factory_read_action {
	READ_MAC_ADDRESS,
	READ_SN,
	READ_SN_FLAG,
	READ_WAN_MAC_ADDRESS,
	READ_WIFI_LB_MAC_ADDRESS,
	READ_WIFI_HB_MAC_ADDRESS,
	READ_FIRST_MAC_ADDRESS
};

int sf_factory_read_operation(enum sf_factory_read_action action, char* data, int len);
#endif
