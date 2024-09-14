/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file fal_tiger_rma.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __FAL_TIGER_RMA_H
#define __FAL_TIGER_RMA_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "fal_cmm.h"


/**
 * @internal      fal_tiger_rma_action_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     macda               -x
 * @param[in]     action              -yt_rma.h
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_rma_action_set(yt_unit_t unit, yt_rma_da_t  macda, yt_rma_action_t action);


/**
 * @internal      fal_tiger_rma_action_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     macda               -x
 * @param[out]    pAction             -yt_rma.h
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_rma_action_get(yt_unit_t unit, yt_rma_da_t  macda, yt_rma_action_t *pAction);


/**
 * @internal      fal_tiger_rma_bypass_port_isolation_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     macda               -x
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rma_bypass_port_isolation_set(yt_unit_t unit, yt_rma_da_t  macda, yt_enable_t enable);


/**
 * @internal      fal_tiger_rma_bypass_port_isolation_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     macda               -x
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rma_bypass_port_isolation_get(yt_unit_t unit, yt_rma_da_t  macda, yt_enable_t *pEnable);


/**
 * @internal      fal_tiger_rma_bypass_vlan_filter_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     macda               -x
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_rma_bypass_vlan_filter_set(yt_unit_t unit, yt_rma_da_t macda, yt_enable_t enable);


/**
 * @internal      fal_tiger_rma_bypass_vlan_filter_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     macda               -x
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_rma_bypass_vlan_filter_get(yt_unit_t unit, yt_rma_da_t macda, yt_enable_t *pEnable);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
