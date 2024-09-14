/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_multicast.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_MULTICAST_H
#define __YT_MULTICAST_H

#include "yt_cmm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define YT_MAX_MULTICAST_ROUTER_PORT    2
#define YT_MAX_MULTICAST_VLAN_NUM       16

typedef enum yt_multi_op_mode_e
{
    YT_MULTI_OP_MODE_LEARN,
    YT_MULTI_OP_MODE_FLOOD,
    YT_MULTI_OP_MODE_TRAP,
    YT_MULTI_OP_MODE_LEARN_AND_COPY_TO_CPU,
} yt_multi_op_mode_t;

typedef struct yt_multi_learn_bypass_range_s
{
    uint8_t    bypass_239_255_255_x_en;
    uint8_t    bypass_224_0_1_x_en;
    uint8_t    bypass_224_0_0_x_en;
    uint8_t    bypass_ipv6_00xx_en;
} yt_multi_learn_bypass_range_t;

typedef struct yt_multi_router_port_s
{
    uint8_t    valid[YT_MAX_MULTICAST_ROUTER_PORT];
    yt_port_t  port[YT_MAX_MULTICAST_ROUTER_PORT];
} yt_multi_router_port_t;

typedef struct yt_multi_vlan_s
{
    uint16_t    vlanid[YT_MAX_MULTICAST_VLAN_NUM];
} yt_multi_vlan_t;

/**
 * @internal      yt_multicast_igmp_opmode_set
 * @endinternal
 *
 * @brief         set operaction mode for igmp packet
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     mode                -IGMP/MLD operation mode
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_igmp_opmode_set(yt_unit_t unit,  yt_multi_op_mode_t mode);


/**
 * @internal      yt_multicast_igmp_opmode_get
 * @endinternal
 *
 * @brief         get operaction mode for igmp packet
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pmode               -IGMP/MLD operation mode
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_igmp_opmode_get(yt_unit_t unit,  yt_multi_op_mode_t *pmode);


/**
 * @internal      yt_multicast_mld_opmode_set
 * @endinternal
 *
 * @brief         set operaction mode for mld packet
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     mode                -IGMP/MLD operation mode
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_mld_opmode_set(yt_unit_t unit,  yt_multi_op_mode_t mode);


/**
 * @internal      yt_multicast_mld_opmode_get
 * @endinternal
 *
 * @brief         get operaction mode for mld packet
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pmode               -IGMP/MLD operation mode
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_mld_opmode_get(yt_unit_t unit,  yt_multi_op_mode_t *pmode);


/**
 * @internal      yt_multicast_port_report_allow_set
 * @endinternal
 *
 * @brief         enable igmp/mld report on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     en                  -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_port_report_allow_set(yt_unit_t unit, yt_port_t port, yt_enable_t en);


/**
 * @internal      yt_multicast_port_report_allow_get
 * @endinternal
 *
 * @brief         get enable state of igmp/mld report on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pen                 -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_port_report_allow_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pen);


/**
 * @internal      yt_multicast_port_leave_allow_set
 * @endinternal
 *
 * @brief         enable igmp/mld leave on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     en                  -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_port_leave_allow_set(yt_unit_t unit, yt_port_t port, yt_enable_t en);


/**
 * @internal      yt_multicast_port_leave_allow_get
 * @endinternal
 *
 * @brief         get enable state of igmp/mld leave on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pen                 -enable or not
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_port_leave_allow_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pen);


/**
 * @internal      yt_multicast_port_query_allow_set
 * @endinternal
 *
 * @brief         enable igmp/mld query on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     en                  -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_port_query_allow_set(yt_unit_t unit, yt_port_t port, yt_enable_t en);


/**
 * @internal      yt_multicast_port_query_allow_get
 * @endinternal
 *
 * @brief         get enable state of igmp/mld query on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pen                 -enable or not
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_port_query_allow_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pen);


/**
 * @internal      yt_multicast_fastleave_set
 * @endinternal
 *
 * @brief         enable fastleave
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     en                  -enable or not
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_fastleave_set(yt_unit_t unit, yt_enable_t en);


/**
 * @internal      yt_multicast_fastleave_get
 * @endinternal
 *
 * @brief         get enable state of fastleave
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pen                 -enable or not
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_fastleave_get(yt_unit_t unit, yt_enable_t *pen);


/**
 * @internal      yt_multicast_learnlimit_en_set
 * @endinternal
 *
 * @brief         enable multicast group limit
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     en                  -enable or not
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_learnlimit_en_set(yt_unit_t unit, yt_enable_t en);


/**
 * @internal      yt_multicast_learnlimit_en_get
 * @endinternal
 *
 * @brief         get enable state of multicast group limit
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pen                 -enable or not
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_learnlimit_en_get(yt_unit_t unit, yt_enable_t *pen);


/**
 * @internal      yt_multicast_learnlimit_maxgroup_set
 * @endinternal
 *
 * @brief         set max multicast group number
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     maxgroup            -max group number
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_learnlimit_maxgroup_set(yt_unit_t unit, uint32_t  maxgroup);


/**
 * @internal      yt_multicast_learnlimit_maxgroup_get
 * @endinternal
 *
 * @brief         get max multicast group number
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pmaxgroup           -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_learnlimit_maxgroup_get(yt_unit_t unit, uint32_t  *pmaxgroup);


/**
 * @internal      yt_multicast_fwd_routerport_only_set
 * @endinternal
 *
 * @brief         enable forward igmp/mld packet to router port only
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     en                  -enable or not
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_fwd_routerport_only_set(yt_unit_t unit, yt_enable_t en);


/**
 * @internal      yt_multicast_fwd_routerport_only_get
 * @endinternal
 *
 * @brief         get enable state of forward igmp/mld packet to router port only
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pen                 -enable or not
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_fwd_routerport_only_get(yt_unit_t unit, yt_enable_t *pen);


/**
 * @internal      yt_multicast_fwd_routerport_primary_set
 * @endinternal
 *
 * @brief         enable forward igmp/mld packet to router port only if router port exist,otherwire flood
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     en                  -enable or not
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_fwd_routerport_primary_set(yt_unit_t unit, yt_enable_t en);


/**
 * @internal      yt_multicast_fwd_routerport_primary_get
 * @endinternal
 *
 * @brief         get enable state of forward igmp/mld packet to router port only if router port exist
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pen                 -enable or not
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_fwd_routerport_primary_get(yt_unit_t unit, yt_enable_t *pen);


/**
 * @internal      yt_multicast_bypass_grouprange_set
 * @endinternal
 *
 * @brief         set ip range that won't be learnt to multicast group
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     bypass              -Multicast IP range that bypass learn
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_multicast_bypass_grouprange_set(yt_unit_t unit, yt_multi_learn_bypass_range_t bypass);


/**
 * @internal      yt_multicast_bypass_grouprange_get
 * @endinternal
 *
 * @brief         get ip range that won't be learnt to multicast group
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pbypass             -Multicast IP range that bypass learn
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_multicast_bypass_grouprange_get(yt_unit_t unit, yt_multi_learn_bypass_range_t *pbypass);


/**
 * @internal      yt_multicast_dynamic_routerport_get
 * @endinternal
 *
 * @brief         get dynamic multicast router ports
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    prouter_port        -Multicast router port info
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_dynamic_routerport_get(yt_unit_t unit, yt_multi_router_port_t *prouter_port);


/**
 * @internal      yt_multicast_dynamic_routerport_allow_set
 * @endinternal
 *
 * @brief         enable dynamic router port learning on ports
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     en                  -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_multicast_dynamic_routerport_allow_set(yt_unit_t unit, yt_port_t port, yt_enable_t en);


/**
 * @internal      yt_multicast_dynamic_routerport_allow_get
 * @endinternal
 *
 * @brief         get the enable state of dynamic router port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pen                 -enable or not
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_multicast_dynamic_routerport_allow_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pen);


/**
 * @internal      yt_multicast_dynamic_routerport_agingtime_set
 * @endinternal
 *
 * @brief         set dynamic router port aging time
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     sec                 -second
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_multicast_dynamic_routerport_agingtime_set(yt_unit_t unit, uint32_t sec);


/**
 * @internal      yt_multicast_dynamic_routerport_agingtime_get
 * @endinternal
 *
 * @brief         get dynamic router port aging time
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    psec                -second
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_multicast_dynamic_routerport_agingtime_get(yt_unit_t unit, uint32_t *psec);


/**
 * @internal      yt_multicast_vlan_add
 * @endinternal
 *
 * @brief         add multicast vlan
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_SAMEENTRY_EXIST    -already exist same entry
 * @retval        CMM_ERR_ENTRY_FULL -multicast vlan table full
 */
extern yt_ret_t yt_multicast_vlan_add(yt_unit_t unit, yt_vlan_t vid);


/**
 * @internal      yt_multicast_vlan_get
 * @endinternal
 *
 * @brief         get multicast vlan table info
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pvlanarray          -multicast vlan info
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_ENTRY_NOT_FOUND   -no entry
 */
extern yt_ret_t yt_multicast_vlan_get(yt_unit_t unit, yt_multi_vlan_t *pvlanarray);


/**
 * @internal      yt_multicast_vlan_del
 * @endinternal
 *
 * @brief         delete 
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_ENTRY_NOT_FOUND   -no entry
 */
extern yt_ret_t yt_multicast_vlan_del(yt_unit_t unit, yt_vlan_t vid);


/**
 * @internal      yt_multicast_static_routerport_set
 * @endinternal
 *
 * @brief         add static multicast router ports
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port_mask           -yt_types.h
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_static_routerport_set(yt_unit_t unit, yt_port_mask_t port_mask);


/**
 * @internal      yt_multicast_static_routerport_get
 * @endinternal
 *
 * @brief         get static multicast router ports
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pport_mask          -yt_types.h
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_static_routerport_get(yt_unit_t unit, yt_port_mask_t *pport_mask);

/**
 * @internal      yt_multicast_igmp_bypass_port_isolation_set
 * @endinternal
 *
 * @brief         enable igmp passthrough port isolation
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     en                  -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_multicast_igmp_bypass_port_isolation_set(yt_unit_t unit, yt_port_t port, yt_enable_t en);


/**
 * @internal      yt_multicast_igmp_bypass_port_isolation_get
 * @endinternal
 *
 * @brief         get enable state of igmp passthrough port isolation
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pen                 -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_multicast_igmp_bypass_port_isolation_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pen);


/**
 * @internal      yt_multicast_ipmc_bypass_port_isolation_set
 * @endinternal
 *
 * @brief         enable ip multicast passthrough port isolation
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     en                  -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_multicast_ipmc_bypass_port_isolation_set(yt_unit_t unit, yt_enable_t en);


/**
 * @internal      yt_multicast_ipmc_bypass_port_isolation_get
 * @endinternal
 *
 * @brief         get the enable state of ip multicast passthrough port isolation
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pen                 -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_multicast_ipmc_bypass_port_isolation_get(yt_unit_t unit, yt_enable_t *pen);

/**
 * @internal      yt_multicast_port_igmp_bypass_ingrfilter_en_set
 * @endinternal
 *
 * @brief         Set port vlan igmp bypass ingress filter state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enabled             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_port_igmp_bypass_ingrfilter_en_set(yt_unit_t unit, yt_port_t port, yt_enable_t enabled);


/**
 * @internal      yt_multicast_port_igmp_bypass_ingrfilter_en_get
 * @endinternal
 *
 * @brief         Get port vlan igmp bypass ingress filter state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnabled            -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_multicast_port_igmp_bypass_ingrfilter_en_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnabled);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
