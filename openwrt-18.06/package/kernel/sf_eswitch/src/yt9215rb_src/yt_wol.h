/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_wol.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_WOL_H
#define __YT_WOL_H


#include "yt_cmm.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * @internal      yt_wol_ctrl_set
 * @endinternal
 *
 * @brief         Set wol ctrl state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_wol_ctrl_set(yt_unit_t unit, yt_enable_t enable);


/**
 * @internal      yt_wol_ctrl_get
 * @endinternal
 *
 * @brief         Get wol ctrl state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_wol_ctrl_get(yt_unit_t unit, yt_enable_t *pEnable);


/**
 * @internal      yt_wol_ethertype_set
 * @endinternal
 *
 * @brief         Set wol ethertype
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     eth_type            -tpid value
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_wol_ethertype_set(yt_unit_t unit, yt_tpid_t eth_type);


/**
 * @internal      yt_wol_ethertype_get
 * @endinternal
 *
 * @brief         Get wol ethertype
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pEth_type           -tpid value
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_wol_ethertype_get(yt_unit_t unit, yt_tpid_t *pEth_type);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
