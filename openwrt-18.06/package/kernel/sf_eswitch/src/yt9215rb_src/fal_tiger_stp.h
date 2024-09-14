/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file fal_tiger_stp.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __FAL_TIGER_STP_H
#define __FAL_TIGER_STP_H


#include "fal_cmm.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * @internal      fal_tiger_stp_state_set
 * @endinternal
 *
 * @brief         Description
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     stp_id              -stp instance id
 * @param[in]     port                -port num
 * @param[in]     state               -stp state, STP_STATE_LEARN, STP_STATE_DISCARD, STP_STATE_FORWARD
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_stp_state_set(yt_unit_t unit, yt_stp_id_t stp_id, yt_port_t port, yt_stp_state_t state);


/**
 * @internal      fal_tiger_stp_state_get
 * @endinternal
 *
 * @brief         Description
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     stp_id              -stp instance id
 * @param[in]     port                -port num
 * @param[out]    pState              -stp state, STP_STATE_LEARN, STP_STATE_DISCARD, STP_STATE_FORWARD
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_stp_state_get(yt_unit_t unit, yt_stp_id_t stp_id, yt_port_t port, yt_stp_state_t *pState);


/**
 * @internal      fal_tiger_stp_instance_map_set
 * @endinternal
 *
 * @brief         Description
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     stp_id              -stp instance id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_stp_instance_map_set(yt_unit_t unit, yt_vlan_t vid, yt_stp_id_t stp_id );


/**
 * @internal      fal_tiger_stp_instance_map_get
 * @endinternal
 *
 * @brief         Description
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[out]    pStp_id             -stp instance id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_stp_instance_map_get(yt_unit_t unit, yt_vlan_t vid, yt_stp_id_t *pStp_id );




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
