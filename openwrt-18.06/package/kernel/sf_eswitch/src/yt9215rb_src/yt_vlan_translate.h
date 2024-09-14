/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_vlan_translate.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_VLAN_TRANSLATE_H__
#define __YT_VLAN_TRANSLATE_H__


#include "yt_cmm.h"
#include "yt_vlan.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef enum yt_vlan_trans_action_e
{
    VLAN_TRANS_ACTION_INVALID,/*no action*/
    VLAN_TRANS_ACTION_ADD,/*add assign vid if untag*/
    VLAN_TRANS_ACTION_REPLACE,/*replace original vid with assgin vid*/
}yt_vlan_trans_action_t;

typedef enum yt_prio_trans_action_e
{
    PRIO_TRANS_ACTION_INVALID,/*no action*/
    PRIO_TRANS_ACTION_REPLACE,/*replace original priority with assgin priority*/
}yt_prio_trans_action_t;

typedef enum yt_vlan_range_trans_mode_e
{
    VLAN_RANGE_TRANS_MODE_CVLAN,
    VLAN_RANGE_TRANS_MODE_SVLAN,
}yt_vlan_range_trans_mode_t;

typedef enum yt_vlan_format_e
{
    VLAN_FMT_UNTAGGED,
    VLAN_FMT_PRIO_TAGGED,
    VLAN_FMT_TAGGED,
}yt_vlan_format_t;

typedef struct yt_vlan_range_group_s
{
    uint16_t vid_range0_min;
    uint16_t vid_range0_max;
    uint16_t vid_range1_min;
    uint16_t vid_range1_max;
    uint16_t vid_range2_min;
    uint16_t vid_range2_max;
    uint16_t vid_range3_min;
    uint16_t vid_range3_max;
}yt_vlan_range_group_t;

typedef struct yt_vlan_trans_tbl_s
{
    yt_bool_t   svid_valid;
    uint16_t    svid;
    yt_bool_t   stag_format_valid;
    yt_vlan_format_t     stag_format;
    yt_bool_t   cvid_valid;
    uint16_t    cvid;
    yt_bool_t   ctag_format_valid;
    yt_vlan_format_t    ctag_format;
    uint16_t    valid_port_mask;
}yt_vlan_trans_tbl_t;

typedef struct yt_vlan_trans_action_tbl_s
{
    yt_vlan_trans_action_t     svid_action;
    uint16_t    assign_svid;
    yt_vlan_trans_action_t     cvid_action;
    uint16_t    assign_cvid;
}yt_vlan_trans_action_tbl_t;

typedef struct yt_egr_vlan_trans_tbl_s
{
    yt_bool_t   svid_valid;
    yt_bool_t   cvid_valid;
    yt_bool_t   mvr_valid;/*if check multicast vlan hit or not*/
    uint16_t    valid_port_mask;
    yt_vlan_range_trans_mode_t  vid_range_mode;/*select range check works on CVLAN or SVLAN*/
    uint16_t    vid_range_min;
    uint16_t    vid_range_max;
    uint16_t    vid;/*for one of another svlan and cvlan if range mode works one*/
    yt_bool_t   original_ctag_format_valid;/*if check original cvlan is tagged*/
    yt_bool_t   original_stag_format_valid;/*if check original svlan is tagged*/
} yt_egr_vlan_trans_tbl_t;

typedef struct yt_egr_vlan_trans_data_s
{
    yt_bool_t     svid_enable;
    yt_bool_t     cvid_enable;
    uint16_t    svid;
    uint16_t    cvid;
} yt_egr_vlan_trans_action_tbl_t;


/**
 * @internal      yt_vlan_trans_untagPvidIgnore_set
 * @endinternal
 *
 * @brief         Set if check pvid for untag packet,use pvid by default.if enable,use 0 instead of pvid
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[in]     enable              -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_vlan_trans_untagPvidIgnore_set(yt_unit_t unit, yt_vlan_type_t type,  yt_port_t  port,  yt_enable_t enable);


/**
 * @internal      yt_vlan_trans_untagPvidIgnore_get
 * @endinternal
 *
 * @brief         Get pvid ignore state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[out]    pEnable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_vlan_trans_untagPvidIgnore_get(yt_unit_t unit, yt_vlan_type_t  type,  yt_port_t  port,  yt_enable_t *pEnable);


/**
 * @internal      yt_vlan_trans_rangeProfile_add
 * @endinternal
 *
 * @brief         Add vlan translate range profile
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vlan_range          -ingress vlan translate profile group info, one of them should bind to the port
 * @param[out]     pProfile_id         -ingress vlan range profile index
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_trans_rangeProfile_add(yt_unit_t unit, yt_vlan_range_group_t vlan_range, yt_profile_id_t *pProfile_id);


/**
 * @internal      yt_vlan_trans_rangeProfile_get
 * @endinternal
 *
 * @brief         Description
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     profile_id          -ingress vlan range profile index
 * @param[out]    pVlan_range         -ingress vlan translate profile group info, one of them should bind to the port
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_trans_rangeProfile_get(yt_unit_t unit, yt_profile_id_t profile_id, yt_vlan_range_group_t *pVlan_range);


/**
 * @internal      yt_vlan_trans_rangeProfile_del
 * @endinternal
 *
 * @brief         Delete vlan translate range profile by profile id
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     profile_id          -ingress vlan range profile index
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_trans_rangeProfile_del(yt_unit_t unit, yt_profile_id_t profile_id);


/**
 * @internal      yt_vlan_trans_port_rangeProfileSel_set
 * @endinternal
 *
 * @brief         Select port vlan translate range profile info
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     profile_id          -ingress vlan range profile index
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_trans_port_rangeProfileSel_set(yt_unit_t unit, yt_port_t  port, yt_profile_id_t profile_id);


/**
 * @internal      yt_vlan_trans_port_rangeProfileSel_get
 * @endinternal
 *
 * @brief         Get port vlan translate selected range profile id
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pProfile_id         -ingress vlan profile index
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_trans_port_rangeProfileSel_get(yt_unit_t unit, yt_port_t  port, yt_profile_id_t *pProfile_id);


/**
 * @internal      yt_vlan_trans_mode_set
 * @endinternal
 *
 * @brief         Set vlan translate mode (CVLAN or SVLAN)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     rangeMode           -select the vlan type on which ingress vlan translate depend
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_trans_mode_set(yt_unit_t unit, yt_port_t  port, yt_vlan_range_trans_mode_t rangeMode);


/**
 * @internal      yt_vlan_trans_mode_get
 * @endinternal
 *
 * @brief         Get vlan translate mode (CVLAN or SVLAN)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pRangeMode          -the vlan type on which ingress vlan translate depend
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_trans_mode_get(yt_unit_t unit, yt_port_t  port, yt_vlan_range_trans_mode_t *pRangeMode);


/**
 * @internal      yt_vlan_igr_trans_table_add
 * @endinternal
 *
 * @brief         Add ingress vlan translate table and action
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     pRuleTbl            -ingress vlan translate table info
 * @param[in]     pAction             -ingress vlan translate action
 * @param[out]     pEntry_id           -vlan translate table index
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_TABLE_FULL        -no free table entry
 */
extern yt_ret_t  yt_vlan_igr_trans_table_add(yt_unit_t unit, yt_vlan_trans_tbl_t *pRuleTbl,  yt_vlan_trans_action_tbl_t *pAction, yt_trans_tbl_id_t *pEntry_id);


/**
 * @internal      yt_vlan_igr_trans_table_get
 * @endinternal
 *
 * @brief         Get ingress vlan translate table and action
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     entry_id            -vlan translate table index
 * @param[out]    pRuleTbl            -ingress vlan translate table info
 * @param[out]    pAction             -ingress vlan translate action
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_igr_trans_table_get(yt_unit_t unit,  yt_trans_tbl_id_t entry_id, yt_vlan_trans_tbl_t *pRuleTbl,  yt_vlan_trans_action_tbl_t *pAction);


/**
 * @internal      yt_vlan_igr_trans_table_del
 * @endinternal
 *
 * @brief         Delete ingress vlan translate table
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     entry_id            -vlan translate table index
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_igr_trans_table_del(yt_unit_t unit, yt_trans_tbl_id_t entry_id);


/**
 * @internal      yt_vlan_egr_trans_table_add
 * @endinternal
 *
 * @brief         Add engress vlan translate table and action
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     pRuleTbl            -egress vlan translate table info
 * @param[in]     pAction             -egress vlan translate action
 * @param[out]     pEntry_id           -vlan translate table index
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 * @retval        CMM_ERR_TABLE_FULL        -no free table entry
 */
extern yt_ret_t yt_vlan_egr_trans_table_add(yt_unit_t unit, yt_egr_vlan_trans_tbl_t  *pRuleTbl, yt_egr_vlan_trans_action_tbl_t *pAction, yt_trans_tbl_id_t *pEntry_id);


/**
 * @internal      yt_vlan_egr_trans_table_get
 * @endinternal
 *
 * @brief         Get engress vlan translate table and action
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     entry_idx           -vlan translate table index
 * @param[out]    pRuleTbl            -egress vlan translate table info
 * @param[out]    pAction             -egress vlan translate action
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_vlan_egr_trans_table_get(yt_unit_t unit, yt_trans_tbl_id_t entry_idx, yt_egr_vlan_trans_tbl_t  *pRuleTbl, yt_egr_vlan_trans_action_tbl_t *pAction);


/**
 * @internal      yt_vlan_egr_trans_table_del
 * @endinternal
 *
 * @brief         Delete engress vlan translate table
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     entry_idx           -vlan translate table index
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_egr_trans_table_del(yt_unit_t unit, yt_trans_tbl_id_t entry_idx);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
