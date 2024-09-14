/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_mirror.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_MIRROR_H
#define __YT_MIRROR_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "yt_cmm.h"


/**
 * @internal      yt_mirror_port_set
 * @endinternal
 *
 * @brief         set mirror target port,source rx port and source tx port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     target_port         -port num
 * @param[in]     rx_portmask         -port bit mask
 * @param[in]     tx_portmask         -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_mirror_port_set(yt_unit_t unit, yt_port_t target_port, yt_port_mask_t rx_portmask, yt_port_mask_t tx_portmask);


/**
 * @internal      yt_mirror_port_get
 * @endinternal
 *
 * @brief         get mirror target port,source rx port and source tx port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    p_target_port       -port num
 * @param[out]    p_rx_portmask       -port bit mask
 * @param[out]    p_tx_portmask       -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_mirror_port_get(yt_unit_t unit, yt_port_t *p_target_port, yt_port_mask_t *p_rx_portmask, yt_port_mask_t *p_tx_portmask);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
