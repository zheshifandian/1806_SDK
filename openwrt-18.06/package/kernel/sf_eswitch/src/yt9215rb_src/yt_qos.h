/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_qos.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_QOS_H
#define __YT_QOS_H

#include "yt_cmm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum yt_pri_index_e
{
    CPRI_MAP_INDEX,
    SPRI_MAP_INDEX
}yt_pri_index_t;

typedef enum yt_remark_index_e
{
    CPRI_REMARK_INDEX,
    SPRI_REMARK_INDEX
}yt_remark_index_t;

typedef enum yt_dp_e
{
    DROP_PRIO_GREEN = 0,
    DROP_PRIO_YELLOW,
    DROP_PRIO_RED
}yt_dp_t;

typedef struct yt_qos_intPri_map_weight_s
{
    uint8_t    mac_sa_pri;
    uint8_t    mac_da_pri;
    uint8_t    vlan_pri;
    uint8_t    acl_pri;
    uint8_t    dscp_pri;
    uint8_t    cpri;
    uint8_t    spri;
    uint8_t    port_pri;
}yt_qos_intPri_map_weight_t;

typedef struct yt_qos_pmap_tci_s
{
    uint8_t    dei;
    uint8_t    prio;
}yt_qos_pmap_tci_t;

typedef struct yt_qos_qmap_s
{
    uint8_t    pri0_qid;
    uint8_t    pri1_qid;
    uint8_t    pri2_qid;
    uint8_t    pri3_qid;
    uint8_t    pri4_qid;
    uint8_t    pri5_qid;
    uint8_t    pri6_qid;
    uint8_t    pri7_qid;
}yt_qos_qmap_t;

typedef struct yt_qos_remark_en_s
{
    uint8_t    spri_en;
    uint8_t    sdei_en;
    uint8_t    cpri_en;
    uint8_t    ccfi_en;
    uint8_t    dscp_en;
}yt_qos_remark_en_t;

typedef struct yt_qos_remark_info_s
{
    yt_dp_t     dp;
    yt_pri_t    prio;
}yt_qos_remark_info_t;


/**
 * @internal      yt_qos_intPri_map_weight_set
 * @endinternal
 *
 * @brief         Config the priority between different priority mechanism.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     pri_tbl             -internal priority select, The higher the value, the higher the priority
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intPri_map_weight_set(yt_unit_t unit, yt_port_t port, yt_qos_intPri_map_weight_t pri_tbl);


/**
 * @internal      yt_qos_intPri_map_weight_get
 * @endinternal
 *
 * @brief         Get the priority configuration between different priority mechanism.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pri_tbl             -internal priority select, The higher the value, the higher the priority
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intPri_map_weight_get(yt_unit_t unit, yt_port_t port, yt_qos_intPri_map_weight_t *pri_tbl);

/**
 * @internal      yt_qos_intPri_portDefPri_set  
 * @endinternal
 *
 * @brief         Map port to internal priority.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable              -enable or disable
 * @param[in]     pri                 -internal priority or vlan tag priority (0 - MAX_PRIORITY)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intPri_portDefPri_set (yt_unit_t unit, yt_port_t port, yt_enable_t enable, yt_pri_t pri);


/**
 * @internal      yt_qos_intPri_portDefPri_get 
 * @endinternal
 *
 * @brief         Get port to internal priority.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable             -enable or disable
 * @param[out]    pPri                -internal priority or vlan tag priority (0 - MAX_PRIORITY)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_qos_intPri_portDefPri_get (yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable, yt_pri_t *pPri);


/**
 * @internal      yt_qos_intPri_cpri_map_set
 * @endinternal
 *
 * @brief         Map 802.1p priority value to internal priority.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     cpri                -tag control information exclude vid
 * @param[in]     pri                 -internal priority or vlan tag priority (0 - MAX_PRIORITY)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intPri_cpri_map_set(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t cpri, yt_pri_t pri);

/**
 * @internal      yt_qos_intPri_cpri_map_get
 * @endinternal
 *
 * @brief         Get 802.1p priority value to internal priority.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     cpri                -tag control information exclude vid
 * @param[out]    pPri                -internal priority or vlan tag priority (0 - MAX_PRIORITY)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intPri_cpri_map_get(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t cpri, yt_pri_t *pPri);


/**
 * @internal      yt_qos_intPri_spri_map_set
 * @endinternal
 *
 * @brief         Map 802.1ad priority value to internal priority.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     spri                -tag control information exclude vid
 * @param[in]     pri                 -internal priority or vlan tag priority (0 - MAX_PRIORITY)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intPri_spri_map_set(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t spri, yt_pri_t pri);


/**
 * @internal      yt_qos_intPri_spri_map_get
 * @endinternal
 *
 * @brief         Get 802.1ad priority value to internal priority.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     spri                -tag control information exclude vid
 * @param[out]    pPri                -internal priority or vlan tag priority (0 - MAX_PRIORITY)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intPri_spri_map_get(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t spri, yt_pri_t *pPri);

/**
 * @internal      yt_qos_intPri_dscp_map_set
 * @endinternal
 *
 * @brief         Map dscp value to internal priority.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     dscp                -the dscp value (0 - MAX_DSCP_VALUE)
 * @param[in]     pri                 -internal priority or vlan tag priority (0 - MAX_PRIORITY)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intPri_dscp_map_set(yt_unit_t unit, yt_dscp_t dscp, yt_pri_t pri);

/**
 * @internal      yt_qos_intPri_dscp_map_get
 * @endinternal
 *
 * @brief         Get dscp value to internal priority.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     dscp                -the dscp value (0 - MAX_DSCP_VALUE)
 * @param[out]    pPri                -internal priority or vlan tag priority (0 - MAX_PRIORITY)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intPri_dscp_map_get(yt_unit_t unit, yt_dscp_t dscp, yt_pri_t *pPri);

/**
 * @internal      yt_qos_intPri_vlan_map_set
 * @endinternal
 *
 * @brief         Set vlan mapping internal priority and state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     enable              -enable or disable
 * @param[in]     pri                 -internal priority or vlan tag priority (0 - MAX_PRIORITY)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_qos_intPri_vlan_map_set(yt_unit_t unit, yt_vlan_t vid, yt_enable_t enable, yt_pri_t pri);

/**
 * @internal      yt_qos_intPri_vlan_map_get
 * @endinternal
 *
 * @brief         Get vlan mapping internal priority and state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[out]    pEnable             -enable or disable
 * @param[out]    pPri                -internal priority or vlan tag priority (0 - MAX_PRIORITY)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_qos_intPri_vlan_map_get(yt_unit_t unit, yt_vlan_t vid, yt_enable_t *pEnable, yt_pri_t *pPri);

/**
 * @internal      yt_qos_intPri_map_igrMirror_set
 * @endinternal
 *
 * @brief         Config ingress mirror to internal priority and state.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     enable              -enable or disable
 * @param[in]     pri                 -internal priority or vlan tag priority (0 - MAX_PRIORITY)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intPri_map_igrMirror_set(yt_unit_t unit, yt_enable_t enable, yt_pri_t pri);


/**
 * @internal      yt_qos_intPri_map_igrMirror_get
 * @endinternal
 *
 * @brief         Get ingress mirror to internal priority and state.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pEnable             -enable or disable
 * @param[out]    pPri                -internal priority or vlan tag priority (0 - MAX_PRIORITY)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intPri_map_igrMirror_get(yt_unit_t unit, yt_enable_t *pEnable, yt_pri_t *pPri);


/**
 * @internal      yt_qos_intPri_map_egrMirror_set
 * @endinternal
 *
 * @brief         Config engress mirror to internal priority and state.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     enable              -enable or disable
 * @param[in]     pri                 -internal priority or vlan tag priority (0 - MAX_PRIORITY)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intPri_map_egrMirror_set(yt_unit_t unit, yt_enable_t enable, yt_pri_t pri);


/**
 * @internal      yt_qos_intPri_map_egrMirror_get
 * @endinternal
 *
 * @brief         Get engress mirror to internal priority and state.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pEnable             -enable or disable
 * @param[out]    pPri                -internal priority or vlan tag priority (0 - MAX_PRIORITY)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intPri_map_egrMirror_get(yt_unit_t unit, yt_enable_t *pEnable, yt_pri_t *pPri);


/**
 * @internal      yt_qos_intDP_cpri_map_set
 * @endinternal
 *
 * @brief         Map 802.1p priority value to internal drop precedent
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     cpri                -tag control information exclude vid
 * @param[in]     dp                  -drop priority
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intDP_cpri_map_set(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t cpri, yt_dp_t dp);

/**
 * @internal      yt_qos_intDP_cpri_map_get
 * @endinternal
 *
 * @brief         Get 802.1p priority value to internal drop precedent
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     cpri                -as a key value that maps to int_pri and int_dp
 * @param[out]    pDp                 -drop priority
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intDP_cpri_map_get(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t cpri, yt_dp_t *pDp);

/**
 * @internal      yt_qos_intDP_spri_map_set
 * @endinternal
 *
 * @brief         Map 802.1ad priority value to internal drop precedent.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     spri                -tag control information exclude vid
 * @param[in]     dp                  -drop priority
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intDP_spri_map_set(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t spri, yt_dp_t dp);

/**
 * @internal      yt_qos_intDP_spri_map_get
 * @endinternal
 *
 * @brief         Get 802.1ad priority value to internal drop precedent.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     spri                -tag control information exclude vid
 * @param[out]    pDp                 -drop priority
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intDP_spri_map_get(yt_unit_t unit, yt_port_t port, yt_qos_pmap_tci_t spri, yt_dp_t *pDp);

/**
 * @internal      yt_qos_intDP_dscp_map_set
 * @endinternal
 *
 * @brief         Map dscp value to internal drop precedent.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     dscp                -the dscp value (0 - MAX_DSCP_VALUE)
 * @param[in]     dp                  -drop priority
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intDP_dscp_map_set(yt_unit_t unit, yt_dscp_t dscp, yt_dp_t dp);


/**
 * @internal      yt_qos_intDP_dscp_map_get
 * @endinternal
 *
 * @brief         Get dscp value to internal drop precedent.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     dscp                -the dscp value (0 - MAX_DSCP_VALUE)
 * @param[out]    pDp                 -drop priority
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_intDP_dscp_map_get(yt_unit_t unit, yt_dscp_t dscp, yt_dp_t *pDp);

/**
 * @internal      yt_qos_que_map_ucast_set
 * @endinternal
 *
 * @brief         Internal priority to ucast queue mapping about port.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     qmap_tbl            -internal priority to queue mapping,qid range(0~7)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_que_map_ucast_set(yt_unit_t unit, yt_port_t port, yt_qos_qmap_t qmap_tbl);


/**
 * @internal      yt_qos_que_map_ucast_get
 * @endinternal
 *
 * @brief         Get internal priority to ucast queue mapping about port.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    qmap_tbl            -internal priority to queue mapping
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_que_map_ucast_get(yt_unit_t unit, yt_port_t port, yt_qos_qmap_t *qmap_tbl);


/**
 * @internal      yt_qos_que_map_mcast_set
 * @endinternal
 *
 * @brief         Internal priority to mcast queue mapping about port.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     qmap_tbl            -internal priority to queue mapping,qid range(0~3)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_que_map_mcast_set(yt_unit_t unit, yt_port_t port, yt_qos_qmap_t qmap_tbl);


/**
 * @internal      yt_qos_que_map_mcast_get
 * @endinternal
 *
 * @brief         Get internal priority to mcast queue mapping about port.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    qmap_tbl            -internal priority to queue mapping
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_que_map_mcast_get(yt_unit_t unit, yt_port_t port, yt_qos_qmap_t *qmap_tbl);

/**
 * @internal      yt_qos_que_forceDrop_enable_set
 * @endinternal
 *
 * @brief         Config force drop for port.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_que_forceDrop_enable_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable);


/**
 * @internal      yt_qos_que_forceDrop_enable_get
 * @endinternal
 *
 * @brief         Get force drop for port.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_que_forceDrop_enable_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable);

/**
 * @internal      yt_qos_remark_port_set
 * @endinternal
 *
 * @brief         Config port remark state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     rmark_en            -remark states of different type
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_remark_port_set(yt_unit_t unit, yt_port_t port, yt_qos_remark_en_t rmark_en);


/**
 * @internal      yt_qos_remark_port_get
 * @endinternal
 *
 * @brief         Get port remark state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pRmark_en           -remark states of different type
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_remark_port_get(yt_unit_t unit, yt_port_t port, yt_qos_remark_en_t *pRmark_en);


/**
 * @internal      yt_qos_remark_dscp_set
 * @endinternal
 *
 * @brief         Config internal priority and drop precedent to dscp mapping
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     srcInfo             -as a key value that remark to priority and dscp
 * @param[in]     new_dscp            -the dscp value (0 - MAX_DSCP_VALUE)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_remark_dscp_set(yt_unit_t unit, yt_qos_remark_info_t srcInfo, yt_dscp_t new_dscp);


/**
 * @internal      yt_qos_remark_dscp_get
 * @endinternal
 *
 * @brief         Get internal priority and drop precedent to dscp mapping
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     srcInfo             -as a key value that remark to priority and dscp
 * @param[out]    pNew_dscp           -the dscp value (0 - MAX_DSCP_VALUE)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_remark_dscp_get(yt_unit_t unit, yt_qos_remark_info_t srcInfo, yt_dscp_t *pNew_dscp);


/**
 * @internal      yt_qos_remark_cpri_set
 * @endinternal
 *
 * @brief         Config cvlan internal priority and drop precedent to priority and cfi mapping
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     srcInfo             -as a key value that remark to priority and dscp
 * @param[in]     dstInfo             -tag control information exclude vid
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_remark_cpri_set(yt_unit_t unit, yt_port_t port, yt_qos_remark_info_t srcInfo, yt_qos_pmap_tci_t dstInfo);


/**
 * @internal      yt_qos_remark_cpri_get
 * @endinternal
 *
 * @brief         Get cvlan internal priority and drop precedent to priority and cfi mapping
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     srcInfo             -as a key value that remark to priority and dscp
 * @param[out]    pDstInfo            -tag control information exclude vid
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_remark_cpri_get(yt_unit_t unit, yt_port_t port, yt_qos_remark_info_t srcInfo, yt_qos_pmap_tci_t *pDstInfo);

/**
 * @internal      yt_qos_remark_spri_set
 * @endinternal
 *
 * @brief         Config svlan internal priority and drop precedent to priority and cfi mapping
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     srcInfo             -as a key value that remark to priority and dscp
 * @param[in]     dstInfo             -tag control information exclude vid
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_remark_spri_set(yt_unit_t unit, yt_port_t port, yt_qos_remark_info_t srcInfo, yt_qos_pmap_tci_t dstInfo);


/**
 * @internal      yt_qos_remark_spri_get
 * @endinternal
 *
 * @brief         Get svlan internal priority and drop precedent to priority and cfi mapping
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     srcInfo             -as a key value that remark to priority and dscp
 * @param[out]    pDstInfo            -tag control information exclude vid
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_remark_spri_get(yt_unit_t unit, yt_port_t port, yt_qos_remark_info_t srcInfo, yt_qos_pmap_tci_t *pDstInfo);


/**
 * @internal      yt_qos_schedule_sp_set
 * @endinternal
 *
 * @brief         Config queue schedule sp priority
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[in]     qpri                -queue priority (0 - MAX_QUEUE_ID)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_schedule_sp_set(yt_unit_t unit, yt_qid_t qinfo, yt_queue_pri_t qpri);


/**
 * @internal      yt_qos_schedule_sp_get
 * @endinternal
 *
 * @brief         Get queue schedule sp priority
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[out]    pQpri               -queue priority (0 - MAX_QUEUE_ID)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_schedule_sp_get(yt_unit_t unit, yt_qid_t qinfo, yt_queue_pri_t *pQpri);


/**
 * @internal      yt_qos_schedule_dwrr_mode_set
 * @endinternal
 *
 * @brief         Config queue schedule dwrr mode(bps or pps)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[in]     dwrr_cmode          -bbs and pps
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_schedule_dwrr_mode_set(yt_unit_t unit, yt_qid_t qinfo, yt_rate_mode_t dwrr_cmode);


/**
 * @internal      yt_qos_schedule_dwrr_mode_get
 * @endinternal
 *
 * @brief         Get queue schedule dwrr mode weight(bps or pps)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[out]    pDwrr_cmode         -bbs and pps
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_schedule_dwrr_mode_get(yt_unit_t unit, yt_qid_t qinfo, yt_rate_mode_t *pDwrr_cmode);


/**
 * @internal      yt_qos_schedule_dwrr_set
 * @endinternal
 *
 * @brief         Config queue schedule dwrr mode weight
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[in]     qweight             -queue weight in dwrr mode,(1~0x3FFF)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_schedule_dwrr_set(yt_unit_t unit, yt_qid_t qinfo, yt_queue_weight_t qweight);


/**
 * @internal      yt_qos_schedule_dwrr_get
 * @endinternal
 *
 * @brief         Get queue schedule dwrr mode weight
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[out]    pQweight            -queue weight in dwrr mode
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_qos_schedule_dwrr_get(yt_unit_t unit, yt_qid_t qinfo, yt_queue_weight_t *pQweight);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
