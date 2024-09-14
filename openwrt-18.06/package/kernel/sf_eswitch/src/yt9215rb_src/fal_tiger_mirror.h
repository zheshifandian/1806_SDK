/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file fal_tiger_mirror.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __FAL_TIGER_MIRROR_H
#define __FAL_TIGER_MIRROR_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "fal_cmm.h"


/**
 * @internal      fal_tiger_mirror_port_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     target_port         -port num
 * @param[in]     rx_portmask         -port bit mask
 * @param[in]     tx_portmask         -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_mirror_port_set(yt_unit_t unit, yt_port_t target_port, yt_port_mask_t rx_portmask, yt_port_mask_t tx_portmask);


/**
 * @internal      fal_tiger_mirror_port_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[out]    p_target_port       -port num
 * @param[out]    p_rx_portmask       -port bit mask
 * @param[out]    p_tx_portmask       -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_mirror_port_get(yt_unit_t unit, yt_port_t *p_target_port, yt_port_mask_t *p_rx_portmask, yt_port_mask_t *p_tx_portmask);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
