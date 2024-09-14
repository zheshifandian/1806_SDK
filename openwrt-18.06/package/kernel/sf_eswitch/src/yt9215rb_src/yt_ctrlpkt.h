/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_ctrlpkt.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_CTRLPKT_H
#define __YT_CTRLPKT_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "yt_cmm.h"

typedef enum yt_ctrlpkt_l2_action_e {
    L2_ACTION_FWD = 0,
    L2_ACTION_TRAP,
    L2_ACTION_DROP,
    L2_ACTION_COPY,
    L2_ACTION_END
}yt_ctrlpkt_l2_action_t;


/**
 * @internal      yt_ctrlpkt_unknown_ucast_act_set
 * @endinternal
 *
 * @brief         set unknown unicast packet action on port,forward by default
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     act_ctrl            -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_ctrlpkt_unknown_ucast_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl);


/**
 * @internal      yt_ctrlpkt_unknown_ucast_act_get
 * @endinternal
 *
 * @brief         get unknown unicast packet action on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pAct_ctrl           -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_ctrlpkt_unknown_ucast_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl);


/**
 * @internal      yt_ctrlpkt_unknown_mcast_act_set
 * @endinternal
 *
 * @brief         set unknown multicast packet action on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     act_ctrl            -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_ctrlpkt_unknown_mcast_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl);


/**
 * @internal      yt_ctrlpkt_unknown_mcast_act_get
 * @endinternal
 *
 * @brief         get unknown multicast packet action on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pAct_ctrl           -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_ctrlpkt_unknown_mcast_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl);


/**
 * @internal      yt_ctrlpkt_arp_act_set
 * @endinternal
 *
 * @brief         set arp packet behaviour
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     act_ctrl            -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_ctrlpkt_arp_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl);


/**
 * @internal      yt_ctrlpkt_arp_act_get
 * @endinternal
 *
 * @brief         get arp packet behaviour
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pAct_ctrl           -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_ctrlpkt_arp_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl);


/**
 * @internal      yt_ctrlpkt_nd_act_set
 * @endinternal
 *
 * @brief         set nd packet behaviour
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     act_ctrl            -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_ctrlpkt_nd_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl);


/**
 * @internal      yt_ctrlpkt_nd_act_get
 * @endinternal
 *
 * @brief         get nd packet behaviour
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pAct_ctrl           -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_ctrlpkt_nd_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl);


/**
 * @internal      yt_ctrlpkt_lldp_eee_act_set
 * @endinternal
 *
 * @brief         set lldp eee packet behaviour
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     act_ctrl            -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_ctrlpkt_lldp_eee_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl);


/**
 * @internal      yt_ctrlpkt_lldp_eee_act_get
 * @endinternal
 *
 * @brief         get lldp eee packet behaviour
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pAct_ctrl           -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_ctrlpkt_lldp_eee_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl);


/**
 * @internal      yt_ctrlpkt_lldp_act_set
 * @endinternal
 *
 * @brief         set lldp packet behaviour
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     act_ctrl            -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_ctrlpkt_lldp_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl);


/**
 * @internal      yt_ctrlpkt_lldp_act_get
 * @endinternal
 *
 * @brief         get lldp packet behaviour
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pAct_ctrl           -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_ctrlpkt_lldp_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
