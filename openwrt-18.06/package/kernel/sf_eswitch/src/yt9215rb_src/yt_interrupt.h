/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_interrupt.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_INTERRUPT_H
#define __YT_INTERRUPT_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "yt_cmm.h"

typedef enum yt_int_polarity_e
{
    INT_POLAR_LOW = 0,
    INT_POLAR_HIGH,
    INT_POLAR_END
} yt_int_polarity_t;

typedef enum yt_int_type_e
{
    INT_TYPE_HW_EXCEPTION = 0,
    INT_TYPE_PHY_INT0,
    INT_TYPE_PHY_INT1,
    INT_TYPE_PHY_INT2,
    INT_TYPE_PHY_INT3,
    INT_TYPE_PHY_INT4,
    INT_TYPE_PHY_INT5,
    INT_TYPE_PHY_INT6,
    INT_TYPE_PHY_INT7,
    INT_TYPE_SG_INT0,
    INT_TYPE_SG_INT1,
    INT_TYPE_EEPROM_LOAD_DONE,
    INT_TYPE_LOOP_DETECT,
    INT_TYPE_BRIDGE_CONGESTION,
    INT_TYPE_ACL_ACT,
    INT_TYPE_CPU_OP_DONE,
    INT_TYPE_FDB_LEARN_FULL,
    INT_TYPE_FDB_DELETE,
    INT_TYPE_FDB_LEARN_MOVE,
    INT_TYPE_FDB_LEARN_CREATE,
    INT_TYPE_IGMP_DELETE,
    INT_TYPE_IGMP_LEARN_FULL,
    INT_TYPE_HW_INI_DONE,
    INT_TYPE_QM_FLUSH_DONE,
    INT_TYPE_WOL_INT,
    INT_TYPE_DYING_GASP,
    INT_TYPE_ACL_HIT,
    INT_TYPE_MDIO_TRANS_DONE,
    INTR_TYPE_OVER_TEMP,
    INT_TYPE_END
}yt_int_type_t;

/**
 * @internal      yt_int_polarity_set
 * @endinternal
 *
 * @brief         Set interrupt polarity type
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -high level low level
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_int_polarity_set(yt_unit_t unit, yt_int_polarity_t type);


/**
 * @internal      yt_int_polarity_get
 * @endinternal
 *
 * @brief         Get interrupt polarity type
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pType               -high level low level
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_int_polarity_get(yt_unit_t unit, yt_int_polarity_t *pType);


/**
 * @internal      yt_int_control_set
 * @endinternal
 *
 * @brief         Set interrupt state depend on interrupt type
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -interrupt type
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_int_control_set(yt_unit_t unit, yt_int_type_t type, yt_enable_t enable);


/**
 * @internal      yt_int_control_get
 * @endinternal
 *
 * @brief         Get interrupt state depend on interrupt type
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -interrupt type
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_int_control_get(yt_unit_t unit, yt_int_type_t type, yt_enable_t *pEnable);


/**
 * @internal      yt_int_status_get
 * @endinternal
 *
 * @brief         Get interrupt state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pIntStatus          -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_int_status_get(yt_unit_t unit, yt_intr_status_t *pIntStatus);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
