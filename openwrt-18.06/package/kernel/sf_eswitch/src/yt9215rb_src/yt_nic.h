/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_nic.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_NIC_H
#define __YT_NIC_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "yt_cmm.h"

typedef enum yt_cpuport_mode_e {
    CPUPORT_MODE_INTERNAL,
    CPUPORT_MODE_EXTERNAL,
}yt_cpuport_mode_t;


/**
 * @internal      yt_nic_init
 * @endinternal
 *
 * @brief         init nic module
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_nic_init(yt_unit_t unit);



/**
 * @internal      yt_nic_cpuport_mode_set
 * @endinternal
 *
 * @brief         Select internal or external cpu port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     mode                -internal or external cpu port
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_nic_cpuport_mode_set(yt_unit_t unit, yt_cpuport_mode_t mode);


/**
 * @internal      yt_nic_cpuport_mode_get
 * @endinternal
 *
 * @brief         Get cpu port mode setting
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pMode               -internal or external cpu port
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_INPUT        -input parameter error
 */
extern yt_ret_t  yt_nic_cpuport_mode_get(yt_unit_t unit, yt_cpuport_mode_t *pMode);


/**
 * @internal      yt_nic_ext_cpuport_en_set
 * @endinternal
 *
 * @brief         enable or disable extend cpu port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_nic_ext_cpuport_en_set(yt_unit_t unit,  yt_enable_t enable);


/**
 * @internal      yt_nic_ext_cpuport_en_get
 * @endinternal
 *
 * @brief         get the state of extend cpu port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_nic_ext_cpuport_en_get(yt_unit_t unit,  yt_enable_t *pEnable);


/**
 * @internal      yt_nic_ext_cpuport_port_set
 * @endinternal
 *
 * @brief         set the extend cpu port num
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_nic_ext_cpuport_port_set(yt_unit_t unit,  yt_port_t port);


/**
 * @internal      yt_nic_ext_cpuport_port_get
 * @endinternal
 *
 * @brief         get the extend cpu port num
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pPort               -port num
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_nic_ext_cpuport_port_get(yt_unit_t unit,  yt_port_t *pPort);


/**
 * @internal      yt_nic_cpuport_tagtpid_set
 * @endinternal
 *
 * @brief         set vlan tag tpid for cpu port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     tpid                -tag tpid value
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_nic_cpuport_tagtpid_set(yt_unit_t unit, uint16_t tpid);


/**
 * @internal      yt_nic_cpuport_tagtpid_get
 * @endinternal
 *
 * @brief         get vlan tag tpid for cpu port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pTpid               -tag tpid value
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_nic_cpuport_tagtpid_get(yt_unit_t unit, uint16_t *pTpid);

/**
 * @internal      yt_nic_ext_cputag_en_set
 * @endinternal
 *
 * @brief         enable or disable extend cpu tag
 * @note          APPLICABLE DEVICES  -Tiger 
 * @param[in]     unit                -unit id
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_nic_ext_cputag_en_set(yt_unit_t unit,  yt_enable_t enable);

/**
 * @internal      yt_nic_ext_cputag_en_get
 * @endinternal
 *
 * @brief         get the state of extend cpu tag
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_nic_ext_cputag_en_get(yt_unit_t unit,  yt_enable_t *pEnable);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
