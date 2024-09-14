/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_loopdetect.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_LOOPDETECT_H
#define __YT_LOOPDETECT_H


#include "yt_cmm.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define YT_REMOTE_NUM_MAX           CAL_REMOTE_NUM_MAX

typedef enum yt_generate_way_e {
    LOOP_DETECT_GENERATE_WAY_HW,
    LOOP_DETECT_GENERATE_WAY_SW,
}yt_generate_way_t;

typedef struct yt_remote_id_e
{
    uint8_t remoteID[YT_REMOTE_NUM_MAX];
}yt_remote_id_t;

/**
 * @internal      yt_loop_detect_enable_set
 * @endinternal
 *
 * @brief         Set loopdetect state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_loop_detect_enable_set(yt_unit_t unit, yt_enable_t enable);


/**
 * @internal      yt_loop_detect_enable_get
 * @endinternal
 *
 * @brief         Get loopdetect state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_loop_detect_enable_get(yt_unit_t unit, yt_enable_t *pEnable);


/**
 * @internal      yt_loop_detect_tpid_set
 * @endinternal
 *
 * @brief         Set loopdetect tpid
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     tpid                -tpid value
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_loop_detect_tpid_set(yt_unit_t unit, yt_tpid_t tpid);


/**
 * @internal      yt_loop_detect_tpid_get
 * @endinternal
 *
 * @brief         Get loopdetect tpid
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pTpid               -tpid value
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_loop_detect_tpid_get(yt_unit_t unit, yt_tpid_t *pTpid);


/**
 * @internal      yt_loop_detect_generate_way_set
 * @endinternal
 *
 * @brief         Set loopdetect generate way (LOOP_DETECT_GENERATE_WAY_HW or LOOP_DETECT_GENERATE_WAY_SW)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     way                 -When a loop occurs, should it be handed over to HW or SW for processing
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_loop_detect_generate_way_set(yt_unit_t unit, yt_generate_way_t way);


/**
 * @internal      yt_loop_detect_generate_way_get
 * @endinternal
 *
 * @brief         Get loopdetect generate way (LOOP_DETECT_GENERATE_WAY_HW or LOOP_DETECT_GENERATE_WAY_SW)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pWay                -When a loop occurs, should it be handed over to HW or SW for processing
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_loop_detect_generate_way_get(yt_unit_t unit, yt_generate_way_t *pWay);


/**
 * @internal      yt_loop_detect_unitID_set
 * @endinternal
 *
 * @brief         Set loopdetect unitid
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     localID             -local unit id
 * @param[in]     remoteID            -remote unit id, support two remote unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_loop_detect_unitID_set(yt_unit_t unit, yt_local_id_t localID, yt_remote_id_t remoteID);


/**
 * @internal      yt_loop_detect_unitID_get
 * @endinternal
 *
 * @brief         Get loopdetect unitid
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pLocalID            -local unit id
 * @param[out]    pRemoteID           -remote unit id, support two remote unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_loop_detect_unitID_get(yt_unit_t unit, yt_local_id_t *pLocalID, yt_remote_id_t *pRemoteID);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
