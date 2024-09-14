/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file fal_tiger_sensor.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __FAL_TIGER_SENSOR_H__
#define __FAL_TIGER_SENSOR_H__


#include "fal_cmm.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * @internal      fal_tiger_sensor_temp_enable_set
 * @endinternal
 *
 * @brief         Description
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_sensor_temp_enable_set(yt_unit_t unit, yt_enable_t enable);


/**
 * @internal      fal_tiger_sensor_temp_enable_get
 * @endinternal
 *
 * @brief         Description
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_sensor_temp_enable_get(yt_unit_t unit, yt_enable_t *pEnable);


/**
 * @internal      fal_tiger_sensor_temp_value_get
 * @endinternal
 *
 * @brief         Description
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pIsNegative         -yt_types.h
 * @param[out]    pAbsoluteValue      -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_sensor_temp_value_get(yt_unit_t unit, yt_bool_t *pIsNegative, uint16_t *pAbsoluteValue);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
