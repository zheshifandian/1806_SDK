/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file fal_tiger_ctrlpkt.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __FAL_TIGER_CTRLPKT_H
#define __FAL_TIGER_CTRLPKT_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "fal_cmm.h"


/**
 * @internal      fal_tiger_ctrlpkt_unknown_ucast_act_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     act_ctrl            -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_ctrlpkt_unknown_ucast_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl);


/**
 * @internal      fal_tiger_ctrlpkt_unknown_ucast_act_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pAct_ctrl           -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_ctrlpkt_unknown_ucast_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl);


/**
 * @internal      fal_tiger_ctrlpkt_unknown_mcast_act_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     act_ctrl            -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_ctrlpkt_unknown_mcast_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl);


/**
 * @internal      fal_tiger_ctrlpkt_unknown_mcast_act_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pAct_ctrl           -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_ctrlpkt_unknown_mcast_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl);


/**
 * @internal      fal_tiger_ctrlpkt_arp_act_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     act_ctrl            -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_ctrlpkt_arp_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl);


/**
 * @internal      fal_tiger_ctrlpkt_arp_act_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pAct_ctrl           -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_ctrlpkt_arp_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl);


/**
 * @internal      fal_tiger_ctrlpkt_nd_act_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     act_ctrl            -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_ctrlpkt_nd_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl);


/**
 * @internal      fal_tiger_ctrlpkt_nd_act_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pAct_ctrl           -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_ctrlpkt_nd_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl);


/**
 * @internal      fal_tiger_ctrlpkt_lldp_eee_act_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     act_ctrl            -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_ctrlpkt_lldp_eee_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl);


/**
 * @internal      fal_tiger_ctrlpkt_lldp_eee_act_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pAct_ctrl           -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_ctrlpkt_lldp_eee_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl);


/**
 * @internal      fal_tiger_ctrlpkt_lldp_act_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     act_ctrl            -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_ctrlpkt_lldp_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl);


/**
 * @internal      fal_tiger_ctrlpkt_lldp_act_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pAct_ctrl           -l2 packet action,FWD,TRAP,DROP,COPY
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_ctrlpkt_lldp_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
