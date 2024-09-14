/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_l2.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_L2_H
#define __YT_L2_H

#include "yt_cmm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum yt_l2_learn_mode_e
{
    YT_L2_LEARN_MODE_AUTO,
    YT_L2_LEARN_MODE_AUTO_AND_COPY,
    YT_L2_LEARN_MODE_CPU_CONTROL,
} yt_l2_learn_mode_t;

typedef enum yt_l2_fdb_type_e
{
    FDB_TYPE_INVALID,
    FDB_TYPE_DYNAMIC,
    FDB_TYPE_STATIC,
    FDB_TYPE_PENDING,
}yt_l2_fdb_type_t;

typedef struct l2_ucastMacAddr_info_s
{
    yt_port_t port;
    yt_vlan_t vid;
    yt_mac_addr_t macaddr;
    yt_l2_fdb_type_t type; /*invalid, static, pending, dynamic*/
}l2_ucastMacAddr_info_t;

/**
 * @internal      yt_l2_init
 * @endinternal
 *
 * @brief         l2 module initialization
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_l2_init(yt_unit_t unit);


/**
 * @internal      yt_l2_mcast_addr_add
 * @endinternal
 *
 * @brief         add static multicast mac address
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     mac_addr            -mac address
 * @param[in]     port_mask           -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 * @retval        CMM_ERR_PORTLIST        -input port mask error
 */
extern yt_ret_t  yt_l2_mcast_addr_add(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_port_mask_t port_mask);


/**
 * @internal      yt_l2_fdb_ucast_addr_add
 * @endinternal
 *
 * @brief         add static unicast mac address on physical port or lag port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     mac_addr            -mac address
 * @param[in]     port                -physical port num or lag port num
 * @param[in]     isLag               -indicate lag port or not
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -on wrong lag port num etc.
 * @retval        CMM_ERR_NOT_INIT        -on no member in lag etc.
 */
extern yt_ret_t  yt_l2_fdb_ucast_addr_add(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_port_t port, yt_bool_t isLag);


/**
 * @internal      yt_l2_fdb_ucast_addr_del
 * @endinternal
 *
 * @brief         delete specific unicast mac address
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     mac_addr            -mac address
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_l2_fdb_ucast_addr_del(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr);


/**
 * @internal      yt_l2_mcast_addr_del
 * @endinternal
 *
 * @brief         delete specific multicast mac address
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     mac_addr            -mac address
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_l2_mcast_addr_del(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr);


/**
 * @internal      yt_l2_fdb_linkdownFlush_en_set
 * @endinternal
 *
 * @brief         enable or disable fdb hw flush when port linkdown
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_l2_fdb_linkdownFlush_en_set(yt_unit_t unit, yt_enable_t enable);


/**
 * @internal      yt_l2_fdb_linkdownFlush_en_get
 * @endinternal
 *
 * @brief         get status of fdb hw flush when port linkdown
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_l2_fdb_linkdownFlush_en_get(yt_unit_t unit, yt_enable_t *pEnable);


/**
 * @internal      yt_l2_fdb_all_ucast_flush
 * @endinternal
 *
 * @brief         flush all dynamic unicast mac address
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t yt_l2_fdb_all_ucast_flush(yt_unit_t unit);


/**
 * @internal      yt_l2_fdb_port_ucast_flush
 * @endinternal
 *
 * @brief         flush all dynamic unicast mac address on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 * @retval        CMM_ERR_PORT        -input port error
 */
extern yt_ret_t yt_l2_fdb_port_ucast_flush(yt_unit_t unit, yt_port_t port);


/**
 * @internal      yt_l2_fdb_vlan_ucast_flush
 * @endinternal
 *
 * @brief         Description
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t yt_l2_fdb_vlan_ucast_flush(yt_unit_t unit, yt_vlan_t vid);


/**
 * @internal      yt_l2_all_mcast_flush
 * @endinternal
 *
 * @brief         flush all dynamic multicast mac address
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t yt_l2_all_mcast_flush(yt_unit_t unit);


/**
 * @internal      yt_l2_port_mcast_flush
 * @endinternal
 *
 * @brief         flush all dynamic multicast mac address on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 * @retval        CMM_ERR_PORT        -input port error
 */
extern yt_ret_t yt_l2_port_mcast_flush(yt_unit_t unit, yt_port_t port);


/**
 * @internal      yt_l2_vlan_mcast_flush
 * @endinternal
 *
 * @brief         flush all dynamic multicast mac address by vlan
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t yt_l2_vlan_mcast_flush(yt_unit_t unit, yt_vlan_t vid);


/**
 * @internal      yt_l2_fdb_type_get
 * @endinternal
 *
 * @brief         get mac address type
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     mac_addr            -mac address
 * @param[out]    ptype               -FDB type,dynamic or static
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 * @retval        CMM_ERR_ENTRY_NOT_FOUND        -mac addr not exist
 */
extern yt_ret_t  yt_l2_fdb_type_get(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_l2_fdb_type_t *ptype);


/**
 * @internal      yt_l2_port_learnlimit_en_set
 * @endinternal
 *
 * @brief         enable learn limitation on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 * @retval        CMM_ERR_PORT        -input port error
 */
extern yt_ret_t  yt_l2_port_learnlimit_en_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable);


/**
 * @internal      yt_l2_port_learnlimit_en_get
 * @endinternal
 *
 * @brief         get enable state of learn limitation on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 * @retval        CMM_ERR_PORT        -input port error
 */
extern yt_ret_t  yt_l2_port_learnlimit_en_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable);


/**
 * @internal      yt_l2_port_learnlimit_cnt_set
 * @endinternal
 *
 * @brief         set limitation count on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     maxcnt              -mac count
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 * @retval        CMM_ERR_PORT        -input port error
 */
extern yt_ret_t  yt_l2_port_learnlimit_cnt_set(yt_unit_t unit, yt_port_t port, uint32_t maxcnt);


/**
 * @internal      yt_l2_port_learnlimit_cnt_get
 * @endinternal
 *
 * @brief         get limitation count on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pmaxcnt             -max count
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 * @retval        CMM_ERR_PORT        -input port error
 */
extern yt_ret_t  yt_l2_port_learnlimit_cnt_get(yt_unit_t unit, yt_port_t port, uint32_t *pmaxcnt);


/**
 * @internal      yt_l2_port_learnlimit_exceed_drop_set
 * @endinternal
 *
 * @brief         enable drop action when learnt count exceed limitation
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 * @retval        CMM_ERR_PORT        -input port error
 */
extern yt_ret_t  yt_l2_port_learnlimit_exceed_drop_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable);


/**
 * @internal      yt_l2_port_learnlimit_exceed_drop_get
 * @endinternal
 *
 * @brief         get the enable state of drop action when learnt count exceed port limitation
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 * @retval        CMM_ERR_PORT        -input port error
 */
extern yt_ret_t  yt_l2_port_learnlimit_exceed_drop_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable);


/**
 * @internal      yt_l2_system_learnlimit_en_set
 * @endinternal
 *
 * @brief         enable system global learn limitation
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_system_learnlimit_en_set(yt_unit_t unit, yt_enable_t enable);


/**
 * @internal      yt_l2_system_learnlimit_en_get
 * @endinternal
 *
 * @brief         get enable state of system global learn port limitation
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_system_learnlimit_en_get(yt_unit_t unit, yt_enable_t *pEnable);


/**
 * @internal      yt_l2_system_learnlimit_cnt_set
 * @endinternal
 *
 * @brief         set system global limitation count
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     maxcnt              -max count
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_system_learnlimit_cnt_set(yt_unit_t unit, uint32_t maxcnt);


/**
 * @internal      yt_l2_system_learnlimit_cnt_get
 * @endinternal
 *
 * @brief         get system global limitation count
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pmaxcnt             -max count
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_system_learnlimit_cnt_get(yt_unit_t unit, uint32_t *pmaxcnt);


/**
 * @internal      yt_l2_system_learnlimit_exceed_drop_set
 * @endinternal
 *
 * @brief         enable drop action when learnt count exceed system limitation
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_system_learnlimit_exceed_drop_set(yt_unit_t unit, yt_enable_t enable);


/**
 * @internal      yt_l2_system_learnlimit_exceed_drop_get
 * @endinternal
 *
 * @brief         get enable state of drop action when learnt count exceed system limitation
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_system_learnlimit_exceed_drop_get(yt_unit_t unit, yt_enable_t *pEnable);


/**
 * @internal      yt_l2_fdb_drop_sa_set
 * @endinternal
 *
 * @brief         set drop source mac address.mac address should exist.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     mac_addr            -mac address
 * @param[in]     enable                  -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_fdb_drop_sa_set(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_enable_t enable);


/**
 * @internal      yt_l2_fdb_drop_sa_get
 * @endinternal
 *
 * @brief         get the drop state of source mac address
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     mac_addr            -mac address
 * @param[out]    pEnable                 -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_fdb_drop_sa_get(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr,  yt_enable_t *pEnable);


/**
 * @internal      yt_l2_fdb_drop_da_set
 * @endinternal
 *
 * @brief         set drop dest mac address.mac address should exist.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     mac_addr            -mac address
 * @param[in]     enable                  -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_fdb_drop_da_set(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_enable_t enable);


/**
 * @internal      yt_l2_fdb_drop_da_get
 * @endinternal
 *
 * @brief         get the drop state of dest mac address
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     mac_addr            -mac address
 * @param[out]    pEnable                 -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_fdb_drop_da_get(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr,  yt_enable_t *pEnable);


/**
 * @internal      yt_l2_fdb_copy2cpu_set
 * @endinternal
 *
 * @brief         set copy to cpu mac address.mac address should exist.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     mac_addr            -mac address
 * @param[in]     enable                  -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_fdb_copy2cpu_set(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_enable_t enable);


/**
 * @internal      yt_l2_fdb_copy2cpu_get
 * @endinternal
 *
 * @brief         get the copy to cpu state of mac address
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     mac_addr            -mac address
 * @param[out]    pEnable                 -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_fdb_copy2cpu_get(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, yt_enable_t *pEnable);


/**
 * @internal      yt_l2_filter_mcast_set
 * @endinternal
 *
 * @brief         enable multicast filter on ports
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port_mask           -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_PORTLIST        -input port mask error
 */
extern yt_ret_t  yt_l2_filter_mcast_set(yt_unit_t unit, yt_port_mask_t port_mask);


/**
 * @internal      yt_l2_filter_mcast_get
 * @endinternal
 *
 * @brief         get the ports that enable multicast filter
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pport_mask          -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_filter_mcast_get(yt_unit_t unit, yt_port_mask_t *pport_mask);

/**
 * @internal      yt_l2_filter_bcast_set
 * @endinternal
 *
 * @brief         enable boardcast filter on ports
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port_mask           -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_PORTLIST        -input port mask error
 */
extern yt_ret_t  yt_l2_filter_bcast_set(yt_unit_t unit, yt_port_mask_t port_mask);


/**
 * @internal      yt_l2_filter_bcast_get
 * @endinternal
 *
 * @brief         get the ports that enable boardcast filter
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pport_mask          -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_filter_bcast_get(yt_unit_t unit, yt_port_mask_t *pport_mask);


/**
 * @internal      yt_l2_filter_unknown_ucast_set
 * @endinternal
 *
 * @brief         enable unknown unicast filter on ports
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port_mask           -yt_types.h
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_l2_filter_unknown_ucast_set(yt_unit_t unit, yt_port_mask_t port_mask);


/**
 * @internal      yt_l2_filter_unknown_ucast_get
 * @endinternal
 *
 * @brief         get the ports that enable unknown unicast filter
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pport_mask          -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_l2_filter_unknown_ucast_get(yt_unit_t unit, yt_port_mask_t *pport_mask);

/**
 * @internal      yt_l2_filter_unknown_mcast_set
 * @endinternal
 *
 * @brief         enable unknown multicast filter on ports
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port_mask           -yt_types.h
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_l2_filter_unknown_mcast_set(yt_unit_t unit, yt_port_mask_t port_mask);


/**
 * @internal      yt_l2_filter_unknown_mcast_get
 * @endinternal
 *
 * @brief         get the ports that enable unknown multicast filter
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pport_mask          -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_l2_filter_unknown_mcast_get(yt_unit_t unit, yt_port_mask_t *pport_mask);



/**
 * @internal      yt_l2_rma_bypass_unknown_mcast_filter_set
 * @endinternal
 *
 * @brief         enable revert multicast address passthrough unknown multicast filter
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_rma_bypass_unknown_mcast_filter_set(yt_unit_t unit, yt_enable_t enable);


/**
 * @internal      yt_l2_rma_bypass_unknown_mcast_filter_get
 * @endinternal
 *
 * @brief         get enable state of revert multicast address passthrough unknown multicast filter
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_rma_bypass_unknown_mcast_filter_get(yt_unit_t unit, yt_enable_t *pEnable);


/**
 * @internal      yt_l2_fdb_port_uc_cnt_get
 * @endinternal
 *
 * @brief         get dynamic unicast mac address count on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pcnt                -count
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 * @retval        CMM_ERR_PORT        -input port error
 */
extern yt_ret_t  yt_l2_fdb_port_uc_cnt_get(yt_unit_t unit, yt_port_t port, uint32 *pcnt);


/**
 * @internal      yt_l2_fdb_lag_uc_cnt_get
 * @endinternal
 *
 * @brief         get dynamic unicast mac address count on lag
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     groupid             -lag group id
 * @param[out]    pcnt                -count
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_fdb_lag_uc_cnt_get(yt_unit_t unit, uint8_t groupid, uint32_t *pcnt);


/**
 * @internal      yt_l2_lag_learnlimit_en_set
 * @endinternal
 *
 * @brief         enable learn limitation on lag port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     groupid             -lag group id
 * @param[in]     enable                  -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_lag_learnlimit_en_set(yt_unit_t unit, uint8_t groupid,  yt_enable_t enable);


/**
 * @internal      yt_l2_lag_learnlimit_en_get
 * @endinternal
 *
 * @brief         get enable state of learn limitation on lag port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     groupid             -lag group id
 * @param[out]    pEnable                 -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_lag_learnlimit_en_get(yt_unit_t unit, uint8_t groupid,  yt_enable_t *pEnable);


/**
 * @internal      yt_l2_lag_learnlimit_cnt_set
 * @endinternal
 *
 * @brief         set limitaion count on lag port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     groupid             -lag group id
 * @param[in]     maxcnt              -max count
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_lag_learnlimit_cnt_set(yt_unit_t unit, uint8_t groupid,  uint32_t maxcnt);


/**
 * @internal      yt_l2_lag_learnlimit_cnt_get
 * @endinternal
 *
 * @brief         get limitaion count on lag port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     groupid             -lag group id
 * @param[out]    pmaxcnt             -max count
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_lag_learnlimit_cnt_get(yt_unit_t unit, uint8_t groupid,  uint32_t *pmaxcnt);


/**
 * @internal      yt_l2_lag_learnlimit_exceed_drop_set
 * @endinternal
 *
 * @brief         enable drop action when learnt count exceed limitation on lag
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     groupid             -lag group id
 * @param[in]     enable                  -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_lag_learnlimit_exceed_drop_set(yt_unit_t unit, uint8_t groupid, yt_enable_t enable);


/**
 * @internal      yt_l2_lag_learnlimit_exceed_drop_get
 * @endinternal
 *
 * @brief         get enable state of drop action when learnt count exceed limitation on lag
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     groupid             -lag group id
 * @param[out]    pEnable                 -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_lag_learnlimit_exceed_drop_get(yt_unit_t unit, uint8_t groupid, yt_enable_t *pEnable);


/**
 * @internal      yt_l2_fdb_uc_cnt_get
 * @endinternal
 *
 * @brief         get system dynamic unicast mac address count
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pcnt                -count
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_fdb_uc_cnt_get(yt_unit_t unit,  uint32 *pcnt);


/**
 * @internal      yt_l2_mc_cnt_get
 * @endinternal
 *
 * @brief         get system dynamic multicast mac address count
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pcnt                -count
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_mc_cnt_get(yt_unit_t unit,  uint32 *pcnt);


/**
 * @internal      yt_l2_fdb_aging_port_en_set
 * @endinternal
 *
 * @brief         enable fdb aging on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 * @retval        CMM_ERR_PORTLIST        -input port mask error
 */
extern yt_ret_t  yt_l2_fdb_aging_port_en_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable);


/**
 * @internal      yt_l2_fdb_aging_port_en_get
 * @endinternal
 *
 * @brief         get enable state of fdb aging on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 * @retval        CMM_ERR_PORT        -input port error
 */
extern yt_ret_t  yt_l2_fdb_aging_port_en_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable);


/**
 * @internal      yt_l2_fdb_aging_time_set
 * @endinternal
 *
 * @brief         set fdb aging time,time range is 1~5*65535 seconds with 5 seconds deviation.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     sec                 -seconds (1~5*65535)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_fdb_aging_time_set(yt_unit_t unit,  uint32_t sec);


/**
 * @internal      yt_l2_fdb_aging_time_get
 * @endinternal
 *
 * @brief         get fdb aging time
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    psec                -second
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_l2_fdb_aging_time_get(yt_unit_t unit,  uint32_t *psec);


/**
 * @internal      yt_l2_fdb_uc_withindex_get
 * @endinternal
 *
 * @brief         get ucast mac address by index
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     index               -index (0~MAX_FDB_NUM-1)
 * @param[out]    pUcastMac           -ucast mac info,refer to l2_ucastMacAddr_info_t
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_EXCEED_RANGE-index is over range
 * @retval        CMM_ERR_ENTRY_NOT_FOUND -no valid mac on this index
 */
extern yt_ret_t yt_l2_fdb_uc_withindex_get(yt_unit_t unit, uint16_t index, l2_ucastMacAddr_info_t *pUcastMac);

/**
 * @internal      yt_l2_fdb_uc_withMacAndVid_get
 * @endinternal
 *
 * @brief         get ucast info by addr and vlan
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     mac_addr            -mac address
 * @param[out]    pUcastMac           -ucast mac info,refer to l2_ucastMacAddr_info_t
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_ENTRY_NOT_FOUND -no valid mac on this index
 */
extern yt_ret_t yt_l2_fdb_uc_withMacAndVid_get(yt_unit_t unit, yt_vlan_t vid, yt_mac_addr_t mac_addr, l2_ucastMacAddr_info_t *pUcastMac);

/**
 * @internal      yt_l2_fdb_uc_withindex_getnext
 * @endinternal
 *
 * @brief         get next vaild ucast info by current index
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     index                 -current index
 * @param[out]    pNext_index            -the index of next vaild entry
 * @param[out]    pUcastMac           -ucast mac info,refer to l2_ucastMacAddr_info_t
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_ENTRY_NOT_FOUND -no valid mac entry on this index next
 */
extern yt_ret_t  yt_l2_fdb_uc_withindex_getnext(yt_unit_t unit, uint16_t index, uint16_t *pNext_index, l2_ucastMacAddr_info_t *pUcastMac);

/**
 * @internal      yt_l2_port_learn_en_set
 * @endinternal
 *
 * @brief         set l2 port learn status 
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_l2_port_learn_en_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable);

/**
 * @internal      yt_l2_port_learn_en_get
 * @endinternal
 *
 * @brief         get l2 port learn status
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_l2_port_learn_en_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
