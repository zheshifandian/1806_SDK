/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file fal_tiger_interrupt.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __FAL_TIGER_INTERRUPT_H
#define __FAL_TIGER_INTERRUPT_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "fal_cmm.h"


/**
 * @internal      fal_tiger_int_polarity_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -high level low level
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_int_polarity_set(yt_unit_t unit, yt_int_polarity_t type);


/**
 * @internal      fal_tiger_int_polarity_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[out]    pType               -high level low level
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_int_polarity_get(yt_unit_t unit, yt_int_polarity_t *pType);


/**
 * @internal      fal_tiger_int_control_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -interrupt type
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_int_control_set(yt_unit_t unit, yt_int_type_t type, yt_enable_t enable);


/**
 * @internal      fal_tiger_int_control_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -interrupt type
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_int_control_get(yt_unit_t unit, yt_int_type_t type, yt_enable_t *pEnable);


/**
 * @internal      fal_tiger_int_status_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[out]    pIntStatus          -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_int_status_get(yt_unit_t unit, yt_intr_status_t *pIntStatus);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
