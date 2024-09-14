/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file fal_tiger_rate.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __FAL_TIGER_RATE_H
#define __FAL_TIGER_RATE_H

#include "fal_cmm.h"
#include "yt_qos.h"
#include "fal_tiger_qos.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define FAL_TIGER_DEFAULT_TOKEN_UNIT                       0x3
#define FAL_TIGER_DEFAULT_METER_TIME_SLOT                  0x50
#define FAL_TIGER_DEFAULT_PSHAP_TIME_SLOT                  0x50
#define FAL_TIGER_DEFAULT_QSHAP_TIME_SLOT                  0x84
#define FAL_TIGER_DEFAULT_BURST_SIZE                       0x3e8
#define FAL_SYS_CLK_REG                                    0xE0040
#define FAL_CHIP_DEVICE_REG                                0x80008

extern uint8_t g_cycle_time[YT_UNIT_NUM];
#define FAL_CHIP_CYCLE_TIME(unit) g_cycle_time[unit]

/**
 * @internal      fal_tiger_rate_init
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_init(yt_unit_t unit);


/**
 * @internal      fal_tiger_rate_igrBandwidthCtrlEnable_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_igrBandwidthCtrlEnable_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable);


/**
 * @internal      fal_tiger_rate_igrBandwidthCtrlEnable_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_igrBandwidthCtrlEnable_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable);


/**
 * @internal      fal_tiger_rate_igrBandwidthCtrlMode_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     port_rate_mode      -config RATE_MODE(byte or packet) and BYTE_RATE_MODE(excGAP or incGAP)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_igrBandwidthCtrlMode_set(yt_unit_t unit, yt_port_t port, yt_port_rate_mode_t port_rate_mode);


/**
 * @internal      fal_tiger_rate_igrBandwidthCtrlMode_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pPort_rate_mode     -config RATE_MODE(byte or packet) and BYTE_RATE_MODE(excGAP or incGAP)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_igrBandwidthCtrlMode_get(yt_unit_t unit, yt_port_t port, yt_port_rate_mode_t *pPort_rate_mode);


/**
 * @internal      fal_tiger_rate_igrBandwidthCtrlRate_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     rate                -CIR config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_igrBandwidthCtrlRate_set(yt_unit_t unit, yt_port_t port, uint32_t rate);


/**
 * @internal      fal_tiger_rate_igrBandwidthCtrlRate_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pRate               -CIR config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_igrBandwidthCtrlRate_get(yt_unit_t unit, yt_port_t port, uint32_t *pRate);


/**
 * @internal      fal_tiger_rate_meter_vlan_enable_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     meter_id            -meter id, can access meter tbl by it
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_meter_vlan_enable_set(yt_unit_t unit, yt_vlan_t vid, yt_meterid_t meter_id, yt_enable_t enable);


/**
 * @internal      fal_tiger_rate_meter_vlan_enable_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[out]    pMeter_id           -meter id, can access meter tbl by it
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_meter_vlan_enable_get(yt_unit_t unit, yt_vlan_t vid, yt_meterid_t *pMeter_id, yt_enable_t *pEnable);


/**
 * @internal      fal_tiger_rate_meter_enable_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     meter_id            -meter id, can access meter tbl by it
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_meter_enable_set(yt_unit_t unit, yt_meterid_t meter_id, yt_enable_t enable);


/**
 * @internal      fal_tiger_rate_meter_enable_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     meter_id            -meter id, can access meter tbl by it
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_meter_enable_get(yt_unit_t unit, yt_meterid_t meter_id, yt_enable_t *pEnable);


/**
 * @internal      fal_tiger_rate_meter_mode_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     meter_id            -meter id, can access meter tbl by it
 * @param[in]     mode                - meter mode config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_meter_mode_set(yt_unit_t unit, yt_meterid_t meter_id, yt_rate_meter_mode_t mode);


/**
 * @internal      fal_tiger_rate_meter_mode_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     meter_id            -meter id, can access meter tbl by it
 * @param[out]    pMode               - meter mode config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_meter_mode_get(yt_unit_t unit, yt_meterid_t meter_id, yt_rate_meter_mode_t *pMode);


/**
 * @internal      fal_tiger_rate_meter_rate_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     meter_id            -x
 * @param[in]     rate                -CIR, EIR, CBS, EBS config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_meter_rate_set(yt_unit_t unit, yt_meterid_t meter_id, yt_qos_two_rate_t rate);


/**
 * @internal      fal_tiger_rate_meter_rate_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     meter_id            -meter id, can access meter tbl by it
 * @param[out]    pRate               -CIR, EIR, CBS, EBS config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_meter_rate_get(yt_unit_t unit, yt_meterid_t meter_id, yt_qos_two_rate_t *pRate);


/**
 * @internal      fal_tiger_rate_shaping_port_enable_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_shaping_port_enable_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable);


/**
 * @internal      fal_tiger_rate_shaping_port_enable_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_shaping_port_enable_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable);


/**
 * @internal      fal_tiger_rate_shaping_port_mode_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     shaping_mode        -shaping mode config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_shaping_port_mode_set(yt_unit_t unit, yt_port_t port, yt_shaping_mode_t shaping_mode);


/**
 * @internal      fal_tiger_rate_shaping_port_mode_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pShaping_mode       -shaping mode config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_shaping_port_mode_get(yt_unit_t unit, yt_port_t port, yt_shaping_mode_t *pShaping_mode);


/**
 * @internal      fal_tiger_rate_shaping_port_rate_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     rate                -CIR config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_shaping_port_rate_set(yt_unit_t unit, yt_port_t port, uint32_t rate);


/**
 * @internal      fal_tiger_rate_shaping_port_rate_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pRate               -CIR config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_shaping_port_rate_get(yt_unit_t unit, yt_port_t port, uint32_t *pRate);


/**
 * @internal      fal_tiger_rate_shaping_queue_enable_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[in]     cshap_en            -enable or disable
 * @param[in]     eshap_en            -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_shaping_queue_enable_set(yt_unit_t unit, yt_qid_t qinfo, yt_enable_t cshap_en, yt_enable_t eshap_en);


/**
 * @internal      fal_tiger_rate_shaping_queue_enable_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[out]    pCshap_en           -enable or disable
 * @param[out]    pEshap_en           -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_shaping_queue_enable_get(yt_unit_t unit, yt_qid_t qinfo, yt_enable_t *pCshap_en, yt_enable_t *pEshap_en);


/**
 * @internal      fal_tiger_rate_shaping_queue_mode_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[in]     shaping_mode        -shaping mode config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_shaping_queue_mode_set(yt_unit_t unit, yt_qid_t qinfo, yt_shaping_mode_t shaping_mode);


/**
 * @internal      fal_tiger_rate_shaping_queue_mode_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[out]    pShaping_mode       -shaping mode config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_shaping_queue_mode_get(yt_unit_t unit, yt_qid_t qinfo, yt_shaping_mode_t *pShaping_mode);


/**
 * @internal      fal_tiger_rate_shaping_queue_rate_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[in]     rate                -CIR, EIR, CBS, EBS config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_shaping_queue_rate_set(yt_unit_t unit, yt_qid_t qinfo, yt_qos_two_rate_t rate);


/**
 * @internal      fal_tiger_rate_shaping_queue_rate_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[out]    pRate               -CIR, EIR, CBS, EBS config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_rate_shaping_queue_rate_get(yt_unit_t unit, yt_qid_t qinfo, yt_qos_two_rate_t *pRate);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
