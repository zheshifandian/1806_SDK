/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_mirror.c
*
* @brief x
*
********************************************************************************
*/
#include "yt_mirror.h"
#include "fal_dispatch.h"


/**
 * @internal      yt_mirror_port_set
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
yt_ret_t yt_mirror_port_set(yt_unit_t unit, yt_port_t target_port, yt_port_mask_t rx_portmask, yt_port_mask_t tx_portmask)
{
    yt_bool_t isEnable = FALSE;

    CMM_PARAM_CHK((YT_UNIT_NUM <= unit), CMM_ERR_INPUT);
    CMM_PARAM_CHK(NULL == YT_DISPATCH(unit), CMM_ERR_NOT_INIT);
    CMM_PARAM_CHK((!(YT_DISPATCH(unit)->is_inited)), CMM_ERR_NOT_INIT);
    CMM_PARAM_CHK((!(CMM_PORT_VALID(unit,target_port))), CMM_ERR_PORT);
    CMM_PARAM_CHK((!(CMM_PLIST_VALID(unit,rx_portmask))), CMM_ERR_PORTLIST);
    CMM_PARAM_CHK((!(CMM_PLIST_VALID(unit,tx_portmask))), CMM_ERR_PORTLIST);

    /* rx/tx port shouldn't contain target port itself */
    CMM_IS_MEMBER_PORT(rx_portmask, target_port, isEnable);
    CMM_PARAM_CHK((TRUE == isEnable), CMM_ERR_PORTLIST);
    CMM_IS_MEMBER_PORT(tx_portmask, target_port, isEnable);
    CMM_PARAM_CHK((TRUE == isEnable), CMM_ERR_PORTLIST);

    return YT_DISPATCH(unit)->mirror_port_set(unit, target_port, rx_portmask, tx_portmask);
}

/**
 * @internal      yt_mirror_port_get
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
yt_ret_t yt_mirror_port_get(yt_unit_t unit, yt_port_t *p_target_port, yt_port_mask_t *p_rx_portmask, yt_port_mask_t *p_tx_portmask)
{
    CMM_PARAM_CHK((YT_UNIT_NUM <= unit), CMM_ERR_INPUT);
    CMM_PARAM_CHK(NULL == YT_DISPATCH(unit), CMM_ERR_NOT_INIT);
    CMM_PARAM_CHK((!(YT_DISPATCH(unit)->is_inited)), CMM_ERR_NOT_INIT);
    CMM_PARAM_CHK((NULL == p_target_port), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == p_rx_portmask), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == p_tx_portmask), CMM_ERR_NULL_POINT);

    return YT_DISPATCH(unit)->mirror_port_get(unit, p_target_port, p_rx_portmask, p_tx_portmask);
}
