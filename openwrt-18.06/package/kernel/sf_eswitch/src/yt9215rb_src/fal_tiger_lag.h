/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file fal_tiger_lag.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __FAL_TIGER_LAG_H
#define __FAL_TIGER_LAG_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "fal_cmm.h"

#define LAG_MEM_NUM_PERGRP      4

/**
 * @internal      fal_tiger_lag_hash_sel_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     hash_mask           -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_lag_hash_sel_set(yt_unit_t unit, uint8_t hash_mask);


/**
 * @internal      fal_tiger_lag_hash_sel_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[out]    p_hash_mask         -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_lag_hash_sel_get(yt_unit_t unit, uint8_t *p_hash_mask);


/**
 * @internal      fal_tiger_lag_group_port_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     groupId             -x
 * @param[in]     member_portmask     -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_lag_group_port_set(yt_unit_t unit, uint8_t groupId, yt_port_mask_t member_portmask);


/**
 * @internal      fal_tiger_lag_group_info_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     groupId             -x
 * @param[out]    p_laginfo           -link aggregation group config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_lag_group_info_get(yt_unit_t unit, uint8_t groupId, yt_link_agg_group_t *p_laginfo);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
