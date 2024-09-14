/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_rate.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_RATE_H
#define __YT_RATE_H

#include "yt_cmm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum yt_byte_rate_gap_e
{
    BYTE_RATE_GAP_EXCLUDE,
    BYTE_RATE_GAP_INCLUDE,
}yt_byte_rate_gap_t;

typedef struct yt_port_rate_mode_s
{
    yt_rate_mode_t    rate_mode;  //byte or packet
    yt_byte_rate_gap_t    inc_gap;//excGAP or incGAP
}yt_port_rate_mode_t;

typedef struct yt_shaping_mode_s
{
    yt_rate_mode_t shp_mode;
    yt_byte_rate_gap_t sch_mode;
}yt_shaping_mode_t;

typedef struct {
    uint32_t    eir; //bps or pps
    uint32_t    cir; //bps or pps
}yt_qos_two_rate_t;

typedef enum yt_meter_mode_e
{
    METER_MODE_RFC4115,
    METER_MODE_RFC2698
} yt_meter_mode_t;

typedef enum yt_color_mode_e
{
    COLOR_AWARE,
    COLOR_BLIND
} yt_color_mode_t;

typedef enum yt_cf_mode_e
{
    CF_MODE_NONE,
    CF_MODE_LEAKY
}yt_cf_mode_t;

typedef enum yt_drop_color_e
{
    DROP_COLOR_GYR = 0,     /* drop color is GREEN YELLOW RED */
    DROP_COLOR_YR,      /* drop color is YELLOW RED */
    DROP_COLOR_R,       /* drop color is RED */
    DROP_COLOR_NONE,    /* no drop color */
}yt_drop_color_t;

typedef struct yt_rate_meter_mode_s
{
    yt_meter_mode_t    meter_mode; //RFC4115 or RFC2698
    yt_rate_mode_t    rate_mode;  //byte or packet
    yt_color_mode_t    color_mode; //aware or blind 
    yt_drop_color_t    drop_color; // drop color
    yt_byte_rate_gap_t    inc_gap;//excGAP or incGAP
    yt_cf_mode_t    cf_mode; //carry overflow Green buckets to Yellow when meter mode is RFC4115
}yt_rate_meter_mode_t;

/**
 * @internal      yt_rate_init
 * @endinternal
 *
 * @brief         rate init api
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_init(yt_unit_t unit);


/**
 * @internal      yt_rate_igrBandwidthCtrlEnable_set
 * @endinternal
 *
 * @brief         Config port rate state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_igrBandwidthCtrlEnable_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable);


/**
 * @internal      yt_rate_igrBandwidthCtrlEnable_get
 * @endinternal
 *
 * @brief         Get port rate state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_igrBandwidthCtrlEnable_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable);


/**
 * @internal      yt_rate_igrBandwidthCtrlMode_set
 * @endinternal
 *
 * @brief         Config port rate mode
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     port_rate_mode      -config RATE_MODE(byte or packet) and BYTE_RATE_MODE(excGAP or incGAP)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_igrBandwidthCtrlMode_set(yt_unit_t unit, yt_port_t port, yt_port_rate_mode_t port_rate_mode);


/**
 * @internal      yt_rate_igrBandwidthCtrlMode_get
 * @endinternal
 *
 * @brief         Get port rate mode
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pPort_rate_mode     -config RATE_MODE(byte or packet) and BYTE_RATE_MODE(excGAP or incGAP)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_igrBandwidthCtrlMode_get(yt_unit_t unit, yt_port_t port, yt_port_rate_mode_t *pPort_rate_mode);


/**
 * @internal      yt_rate_igrBandwidthCtrlRate_set
 * @endinternal
 *
 * @brief         Config port rate value. The min unit is 9kbps(1k=1000) or 8pps.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     rate                -rate setting.
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_igrBandwidthCtrlRate_set(yt_unit_t unit, yt_port_t port, uint32_t rate);


/**
 * @internal      yt_rate_igrBandwidthCtrlRate_get
 * @endinternal
 *
 * @brief         Get port rate value
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pRate               -rate setting
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_igrBandwidthCtrlRate_get(yt_unit_t unit, yt_port_t port, uint32_t *pRate);


/**
 * @internal      yt_rate_meter_vlan_enable_set
 * @endinternal
 *
 * @brief         Config vlan rate state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     meter_id            -meter id, can access meter tbl by it. The range is 0~31.
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_meter_vlan_enable_set(yt_unit_t unit, yt_vlan_t vid, yt_meterid_t meter_id, yt_enable_t enable);


/**
 * @internal      yt_rate_meter_vlan_enable_get
 * @endinternal
 *
 * @brief         Get vlanbased rate state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[out]    pMeter_id           -meter id, can access meter tbl by it
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_meter_vlan_enable_get(yt_unit_t unit, yt_vlan_t vid, yt_meterid_t *pMeter_id, yt_enable_t *pEnable);


/**
 * @internal      yt_rate_meter_enable_set
 * @endinternal
 *
 * @brief         Config meter entry state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     meter_id            -meter id, can access meter tbl by it
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_meter_enable_set(yt_unit_t unit, yt_meterid_t meter_id, yt_enable_t enable);


/**
 * @internal      yt_rate_meter_enable_get
 * @endinternal
 *
 * @brief         Get meter entry state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     meter_id            -meter id, can access meter tbl by it
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_meter_enable_get(yt_unit_t unit, yt_meterid_t meter_id, yt_enable_t *pEnable);


/**
 * @internal      yt_rate_meter_mode_set
 * @endinternal
 *
 * @brief         Config meter entry mode
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     meter_id            -meter id, can access meter tbl by it
 * @param[in]     mode                - meter mode config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_meter_mode_set(yt_unit_t unit, yt_meterid_t meter_id, yt_rate_meter_mode_t mode);


/**
 * @internal      yt_rate_meter_mode_get
 * @endinternal
 *
 * @brief         Get meter entry mode
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     meter_id            -meter id, can access meter tbl by it
 * @param[out]    pMode               - meter mode config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_meter_mode_get(yt_unit_t unit, yt_meterid_t meter_id, yt_rate_meter_mode_t *pMode);


/**
 * @internal      yt_rate_meter_rate_set
 * @endinternal
 *
 * @brief         Config meter entry rate value. The min unit is 9kbps(1k=1000) or 8pps.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     meter_id            -meter id
 * @param[in]     rate                -CIR, EIR config.
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_meter_rate_set(yt_unit_t unit, yt_meterid_t meter_id, yt_qos_two_rate_t rate);


/**
 * @internal      yt_rate_meter_rate_get
 * @endinternal
 *
 * @brief         Get meter entry rate value
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     meter_id            -meter id, can access meter tbl by it
 * @param[out]    pRate               -CIR, EIR config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_meter_rate_get(yt_unit_t unit, yt_meterid_t meter_id, yt_qos_two_rate_t *pRate);


/**
 * @internal      yt_rate_shaping_port_enable_set
 * @endinternal
 *
 * @brief         Config egress port rate state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_shaping_port_enable_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable);


/**
 * @internal      yt_rate_shaping_port_enable_get
 * @endinternal
 *
 * @brief         Get egress port rate state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_shaping_port_enable_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable);


/**
 * @internal      yt_rate_shaping_port_mode_set
 * @endinternal
 *
 * @brief         Config egress port rate mode
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     shaping_mode        -shaping mode config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_shaping_port_mode_set(yt_unit_t unit, yt_port_t port, yt_shaping_mode_t shaping_mode);


/**
 * @internal      yt_rate_shaping_port_mode_get
 * @endinternal
 *
 * @brief         Get egress port rate mode
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pShaping_mode       -shaping mode config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_shaping_port_mode_get(yt_unit_t unit, yt_port_t port, yt_shaping_mode_t *pShaping_mode);


/**
 * @internal      yt_rate_shaping_port_rate_set
 * @endinternal
 *
 * @brief         Config egress port rate value. The min unit is 9kbps(1k=1000) or 8pps.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     rate                -rate setting.
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_shaping_port_rate_set(yt_unit_t unit, yt_port_t port, uint32_t rate);


/**
 * @internal      yt_rate_shaping_port_rate_get
 * @endinternal
 *
 * @brief         Get egress port rate value
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pRate               -rate setting
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_shaping_port_rate_get(yt_unit_t unit, yt_port_t port, uint32_t *pRate);


/**
 * @internal      yt_rate_shaping_queue_enable_set
 * @endinternal
 *
 * @brief         Set status of egress bandwidth control on specified queue.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[in]     cshap_en            -enable or disable
 * @param[in]     eshap_en            -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_shaping_queue_enable_set(yt_unit_t unit, yt_qid_t qinfo, yt_enable_t cshap_en, yt_enable_t eshap_en);


/**
 * @internal      yt_rate_shaping_queue_enable_get
 * @endinternal
 *
 * @brief         Get state of egress bandwidth control on specified queue.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[out]    pCshap_en           -enable or disable
 * @param[out]    pEshap_en           -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_shaping_queue_enable_get(yt_unit_t unit, yt_qid_t qinfo, yt_enable_t *pCshap_en, yt_enable_t *pEshap_en);


/**
 * @internal      yt_rate_shaping_queue_mode_set
 * @endinternal
 *
 * @brief         Set mode of egress bandwidth control on specified queue.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[in]     shaping_mode        -shaping mode config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_shaping_queue_mode_set(yt_unit_t unit, yt_qid_t qinfo, yt_shaping_mode_t shaping_mode);


/**
 * @internal      yt_rate_shaping_queue_mode_get
 * @endinternal
 *
 * @brief         Get mode of egress bandwidth control on specified queue.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[out]    pShaping_mode       -shaping mode config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_shaping_queue_mode_get(yt_unit_t unit, yt_qid_t qinfo, yt_shaping_mode_t *pShaping_mode);


/**
 * @internal      yt_rate_shaping_queue_rate_set
 * @endinternal
 *
 * @brief         Set rate value of egress bandwidth control rate on specified queue. The min unit is 9kbps(1k=1000) or 8pps.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[in]     rate                -CIR, EIR config.
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_shaping_queue_rate_set(yt_unit_t unit, yt_qid_t qinfo, yt_qos_two_rate_t rate);


/**
 * @internal      yt_rate_shaping_queue_rate_get
 * @endinternal
 *
 * @brief         Get rate of egress bandwidth control on specified queue.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     qinfo               -queue id, port, and ucast or mcast type config
 * @param[out]    pRate               -CIR, EIR config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_rate_shaping_queue_rate_get(yt_unit_t unit, yt_qid_t qinfo, yt_qos_two_rate_t *pRate);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
