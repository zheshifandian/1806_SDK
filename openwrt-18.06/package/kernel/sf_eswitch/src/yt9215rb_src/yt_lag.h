/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_lag.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_LAG_H
#define __YT_LAG_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "yt_cmm.h"

typedef enum yt_link_agg_hash_sel_e {
    AGG_HASH_SEL_SRC_PORT,
    AGG_HASH_SEL_MAC_DA,
    AGG_HASH_SEL_MAC_SA,
    AGG_HASH_SEL_IP_DEST,
    AGG_HASH_SEL_IP_SRC,
    AGG_HASH_SEL_IP_PROTO,
    AGG_HASH_SEL_L4_DPORT,
    AGG_HASH_SEL_L4_SPORT,
    AGG_HASH_SEL_MAX
}yt_link_agg_hash_sel_t;

typedef struct yt_link_agg_group_s {
    uint32_t    member_num;
    uint32_t    member_portmask;
} yt_link_agg_group_t;


/**
 * @internal      yt_lag_hash_sel_set
 * @endinternal
 *
 * @brief         set link aggregation hash algorithm bitmask
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     hash_mask           -link aggregation hash algorithm bitmask,refer to yt_link_agg_hash_sel_t
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_lag_hash_sel_set(yt_unit_t unit, uint16_t hash_mask);


/**
 * @internal      yt_lag_hash_sel_get
 * @endinternal
 *
 * @brief         get link aggregation hash algorithm bitmask
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    p_hash_mask         -link aggregation hash algorithm bitmask,refer to yt_link_agg_hash_sel_t
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_lag_hash_sel_get(yt_unit_t unit, uint16_t *p_hash_mask);


/**
 * @internal      yt_lag_group_port_set
 * @endinternal
 *
 * @brief         set lag group member ports
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     groupId             -lag group index
 * @param[in]     member_portmask     -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_SAMEENTRY_EXIST   - member port conflict with other lag
 */
extern yt_ret_t yt_lag_group_port_set(yt_unit_t unit, uint8_t groupId, yt_port_mask_t member_portmask);


/**
 * @internal      yt_lag_group_info_get
 * @endinternal
 *
 * @brief         get lag group member ports num and portmask
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     groupId             -lag group index
 * @param[out]    p_laginfo           -link aggregation group config
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_lag_group_info_get(yt_unit_t unit, uint8_t groupId, yt_link_agg_group_t *p_laginfo);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
