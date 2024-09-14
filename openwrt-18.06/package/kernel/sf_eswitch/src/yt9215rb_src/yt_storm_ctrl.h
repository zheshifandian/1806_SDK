/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_storm_ctrl.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_STORM_CTRL_H
#define __YT_STORM_CTRL_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "yt_cmm.h"

typedef enum yt_storm_type_e {
    STORM_TYPE_BCAST,
    STORM_TYPE_MCAST,
    STORM_TYPE_UNKNOWN_MCAST,
    STORM_TYPE_UNKNOWN_UCAST
}yt_storm_type_t;

typedef enum yt_storm_rate_mode_e {
    STORM_RATE_MODE_BYTE,
    STORM_RATE_MODE_PACKET
}yt_storm_rate_mode_t;

typedef enum yt_storm_rate_gap_e {
    STORM_RATE_GAP_EXCLUDE,
    STORM_RATE_GAP_INCLUDE
}yt_storm_rate_gap_t;

/**
 * @internal      yt_storm_ctrl_init
 * @endinternal
 *
 * @brief         init storm control module
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_storm_ctrl_init(yt_unit_t unit);


/**
 * @internal      yt_storm_ctrl_enable_set
 * @endinternal
 *
 * @brief         enable specific storm control on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_storm_ctrl_enable_set(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_enable_t enable);


/**
 * @internal      yt_storm_ctrl_enable_get
 * @endinternal
 *
 * @brief         get the storm control enable state on port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_storm_ctrl_enable_get(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_enable_t *pEnable);


/**
 * @internal      yt_storm_ctrl_rate_mode_set
 * @endinternal
 *
 * @brief         set the storm control rate mode
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[in]     rate_mode           -storm rate mode,byte or packet
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_storm_ctrl_rate_mode_set(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_storm_rate_mode_t rate_mode);


/**
 * @internal      yt_storm_ctrl_rate_mode_get
 * @endinternal
 *
 * @brief         get the storm control rate mode
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[out]    pRate_mode         -storm rate mode,byte or packet
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_storm_ctrl_rate_mode_get(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_storm_rate_mode_t *pRate_mode);


/**
 * @internal      yt_storm_ctrl_rate_include_gap_set
 * @endinternal
 *
 * @brief         set include or exclude packet gap for storm rate.the length of gap is 20 bytes by default.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[in]     inc_gap             -storm rate include or exclude packet gap
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_storm_ctrl_rate_include_gap_set(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_storm_rate_gap_t inc_gap);


/**
 * @internal      yt_storm_ctrl_rate_include_gap_get
 * @endinternal
 *
 * @brief         get include or exclude packet gap state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[out]    pInc_gap           -storm rate include or exclude packet gap
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_storm_ctrl_rate_include_gap_get(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, yt_storm_rate_gap_t *pInc_gap);


/**
 * @internal      yt_storm_ctrl_rate_set
 * @endinternal
 *
 * @brief         set storm control rate value.The min unit is 8kbps(1k=1000) or 1pps.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[in]     rate                -storm rate.the range is (8kbps~2^22kbps) or (1~2^9kpps),1k=1000,maximum exclude.
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_storm_ctrl_rate_set(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, uint32_t rate);


/**
 * @internal      yt_storm_ctrl_rate_get
 * @endinternal
 *
 * @brief         get storm control rate value
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     storm_type          -storm type,refer to yt_storm_type_t
 * @param[out]    pRate               -storm rate
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_storm_ctrl_rate_get(yt_unit_t unit, yt_port_t port, yt_storm_type_t storm_type, uint32_t *pRate);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
