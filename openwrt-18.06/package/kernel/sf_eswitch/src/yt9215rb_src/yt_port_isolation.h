/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_port_isolation.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_PORT_ISOLATION_H
#define __YT_PORT_ISOLATION_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "yt_cmm.h"


/**
 * @internal      yt_port_isolation_set
 * @endinternal
 *
 * @brief         set allowed dest port mask of the port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     iso_portmask        -the allowed dest portmask of specific port
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_port_isolation_set(yt_unit_t unit, yt_port_t port, yt_port_mask_t iso_portmask);


/**
 * @internal      yt_port_isolation_get
 * @endinternal
 *
 * @brief         get allowed dest port mask of the port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pIso_portmask       -the allowed dest portmask of specific port
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_port_isolation_get(yt_unit_t unit, yt_port_t port, yt_port_mask_t *pIso_portmask);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
