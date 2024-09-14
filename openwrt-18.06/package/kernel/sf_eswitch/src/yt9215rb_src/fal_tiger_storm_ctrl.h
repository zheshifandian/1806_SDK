/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file fal_tiger_storm_ctrl.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __FAL_TIGER_STORM_CTRL_H
#define __FAL_TIGER_STORM_CTRL_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "fal_cmm.h"

#define STORM_DEFAULT_TIMESLOT  100
#define STORM_DEFAULT_CBS_BYTE  1000
#define STORM_DEFAULT_CBS_PACKET  0x1E8

/**
 * @internal      fal_tiger_storm_ctrl_init
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_storm_ctrl_init(yt_unit_t unit);


/**
 * @internal      fal_tiger_storm_ctrl_enable_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_storm_ctrl_enable_set(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_enable_t enable);


/**
 * @internal      fal_tiger_storm_ctrl_enable_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[out]    penable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_storm_ctrl_enable_get(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_enable_t *penable);


/**
 * @internal      fal_tiger_storm_ctrl_rate_mode_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[in]     rate_mode           -storm rate mode,byte or packet
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_storm_ctrl_rate_mode_set(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_storm_rate_mode_t rate_mode);


/**
 * @internal      fal_tiger_storm_ctrl_rate_mode_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[out]    p_rate_mode         -storm rate mode,byte or packet
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_storm_ctrl_rate_mode_get(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_storm_rate_mode_t *p_rate_mode);


/**
 * @internal      fal_tiger_storm_ctrl_rate_include_gap_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[in]     inc_gap             -storm rate include or exclude packet gap
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_storm_ctrl_rate_include_gap_set(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_storm_rate_gap_t inc_gap);


/**
 * @internal      fal_tiger_storm_ctrl_rate_include_gap_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[out]    p_inc_gap           -storm rate include or exclude packet gap
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_storm_ctrl_rate_include_gap_get(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_storm_rate_gap_t *p_inc_gap);


/**
 * @internal      fal_tiger_storm_ctrl_rate_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[in]     rate                -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_storm_ctrl_rate_set(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, uint32_t rate);


/**
 * @internal      fal_tiger_storm_ctrl_rate_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[out]    prate               -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_storm_ctrl_rate_get(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, uint32_t *prate);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
