/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file fal_tiger_port_isolation.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __FAL_TIGER_PORT_ISOLATION_H
#define __FAL_TIGER_PORT_ISOLATION_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "fal_cmm.h"


/**
 * @internal      fal_tiger_port_isolation_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     iso_portmask        -Port Bit Mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_port_isolation_set(yt_unit_t unit, yt_port_t port, yt_port_mask_t iso_portmask);


/**
 * @internal      fal_tiger_port_isolation_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pIso_portmask       -Port Bit Mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_port_isolation_get(yt_unit_t unit, yt_port_t port, yt_port_mask_t *pIso_portmask);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
