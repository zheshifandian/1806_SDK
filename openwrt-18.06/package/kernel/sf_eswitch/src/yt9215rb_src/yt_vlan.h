/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file yt_vlan.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __YT_VLAN_H
#define __YT_VLAN_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "yt_cmm.h"

#define YT_VLAN_ID_MIN                  0
#define YT_VLAN_ID_MAX                  4095
#define YT_DSCP_VALUE_MIN               0
#define YT_DSCP_VALUE_MAX               63
#define YT_DOT1P_PRIORITY_MAX       7

#define YT_TPID_PROFILE_NUM             CAL_VLAN_TPID_PROFILE_NUM
#define YT_PROTOCOL_BASED_VLAN_NUM(unit)      CAL_PROTOCOL_BASED_VLAN_NUM(unit)

typedef struct yt_tpid_profiles_s
{
	uint16_t tpid[YT_TPID_PROFILE_NUM];
}yt_tpid_profiles_t;

typedef enum yt_vlan_type_e
{
    VLAN_TYPE_CVLAN,
    VLAN_TYPE_SVLAN,
}yt_vlan_type_t;

typedef enum yt_vlan_aft_e
{
    VLAN_AFT_ALL,
    VLAN_AFT_TAGGED,
    VLAN_AFT_UNTAGGED,
}yt_vlan_aft_t;

typedef enum yt_egr_tag_mode_e
{
    VLAN_TAG_MODE_UNTAGGED,
    VLAN_TAG_MODE_TAGGED,
    VLAN_TAG_MODE_TAGGED_EXPECT_PVID,
    VLAN_TAG_MODE_PRIO_TAGGED,
    VLAN_TAG_MODE_KEEP_ALL,
    VLAN_TAG_MODE_KEEP_TAGGED_MODE,
    VLAN_TAG_MODE_ENTRY_BASED
} yt_egr_tag_mode_t;

/**
 * @internal      yt_vlan_init
 * @endinternal
 *
 * @brief         vlan init api
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_init(yt_unit_t unit);


/**
 * @internal      yt_vlan_port_set
 * @endinternal
 *
 * @brief         Set vlan member port and untag port.untag port works in VLAN_TAG_MODE_ENTRY_BASED egress tag mode
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     member_portmask       -member port bit mask
 * @param[in]     untag_portmask           -untag member port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_port_set(yt_unit_t unit,  yt_vlan_t vid,  yt_port_mask_t  member_portmask, yt_port_mask_t  untag_portmask);


/**
 * @internal      yt_vlan_port_get
 * @endinternal
 *
 * @brief         Get vlan member port and untag port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[out]    pMember_portmask        -member port bit mask
 * @param[out]    pUntag_portmask           -untag member port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_port_get(yt_unit_t unit,  yt_vlan_t vid,  yt_port_mask_t  *pMember_portmask, yt_port_mask_t  *pUntag_portmask);


/**
 * @internal      yt_vlan_svlMode_enable_set
 * @endinternal
 *
 * @brief         Set vlan svl mode.It will look up fid after enable svl.
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     enable                -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_svlMode_enable_set(yt_unit_t unit, yt_vlan_t vid,  yt_enable_t enable);


/**
 * @internal      yt_vlan_svlMode_enable_get
 * @endinternal
 *
 * @brief         Get vlan svl mode state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[out]    pEnable               -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_svlMode_enable_get(yt_unit_t unit, yt_vlan_t vid,  yt_enable_t *pEnable);


/**
 * @internal      yt_vlan_fid_set
 * @endinternal
 *
 * @brief         Set vlan fid
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     fid                 -vlan filtering id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_fid_set(yt_unit_t unit, yt_vlan_t vid,  yt_fid_t fid);


/**
 * @internal      yt_vlan_fid_get
 * @endinternal
 *
 * @brief         Get vlan fid
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[out]    pFid                -vlan filtering id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_fid_get(yt_unit_t unit, yt_vlan_t vid,  yt_fid_t *pFid);


/**
 * @internal      yt_vlan_igrTpid_set
 * @endinternal
 *
 * @brief         Set vlan ingress tpid profiles,max 4 profiles
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     tpid                -tpid profile array
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_igrTpid_set(yt_unit_t unit,  yt_tpid_profiles_t tpid);


/**
 * @internal      yt_vlan_igrTpid_get
 * @endinternal
 *
 * @brief         Get vlan ingress tpid profiles array
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pTpid               -tpid profile array
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_igrTpid_get(yt_unit_t unit,  yt_tpid_profiles_t *pTpid);


/**
 * @internal      yt_vlan_port_igrTpidSel_set
 * @endinternal
 *
 * @brief         Select vlan ingress tpid profile id depend on vlan type (CVLAN or SVLAN)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[in]     tpidIdxMask         -tpid profile id(0~3) mask,4 profile id bitmap(0x1~0xF)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_port_igrTpidSel_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_tpidprofile_id_mask_t tpidIdxMask);


/**
 * @internal      yt_vlan_port_igrTpidSel_get
 * @endinternal
 *
 * @brief         Get vlan ingress selected tpid profile id mask depend on vlan type (CVLAN or SVLAN)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[out]    pTpidIdxMask        -tpid profile id(0~3) mask,4 profile id bitmap(0x1~0xF)
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_port_igrTpidSel_get(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_tpidprofile_id_mask_t *pTpidIdxMask);

/**
 * @internal      yt_vlan_port_igrPvid_set
 * @endinternal
 *
 * @brief         Set port ingress default vid depend on vlan type (CVLAN or SVLAN)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[in]     vid                 -vlan id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_port_igrPvid_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_vlan_t vid);


/**
 * @internal      yt_vlan_port_igrPvid_get
 * @endinternal
 *
 * @brief         Get port ingress default vid depend on vlan type (CVLAN or SVLAN)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[out]    pVid                -vlan id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_port_igrPvid_get(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_vlan_t *pVid);


/**
 * @internal      yt_vlan_port_igrFilter_enable_set
 * @endinternal
 *
 * @brief         Set port vlan ingress filter state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_port_igrFilter_enable_set(yt_unit_t unit, yt_port_t  port, yt_enable_t enable);


/**
 * @internal      yt_vlan_port_igrFilter_enable_get
 * @endinternal
 *
 * @brief         Get port vlan ingress filter state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable            -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_port_igrFilter_enable_get(yt_unit_t unit, yt_port_t  port, yt_enable_t *pEnable);

/**
 * @internal      yt_vlan_igrTransparent_set
 * @endinternal
 *
 * @brief         Set ingress vlan transparent,packet from port will bypass egress vlan filter on the ports specified by port_mask
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -ignress port num
 * @param[in]     port_mask           -egress port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_igrTransparent_set(yt_unit_t unit, yt_port_t  port, yt_port_mask_t port_mask);


/**
 * @internal      yt_vlan_igrTransparent_get
 * @endinternal
 *
 * @brief         Get ingress vlan transparent setting on ingress port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -ignress port num
 * @param[out]    pPort_mask          -egress port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_igrTransparent_get(yt_unit_t unit, yt_port_t  port, yt_port_mask_t *pPort_mask);



/**
 * @internal      yt_vlan_port_aft_set
 * @endinternal
 *
 * @brief         Set vlan accept frame type depend on vlan type (CVLAN or SVLAN)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[in]     aft              -vlan accept frame type,tag,untag or both.drop all if set NONE
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_port_aft_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t port, yt_vlan_aft_t aft);


/**
 * @internal      yt_vlan_port_aft_get
 * @endinternal
 *
 * @brief         Get vlan accept frame type depend on vlan type (CVLAN or SVLAN)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[out]    pAft             -vlan accept frame type,tag,untag or both
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_port_aft_get(yt_unit_t unit, yt_vlan_type_t type, yt_port_t port, yt_vlan_aft_t *pAft);


/**
 * @internal      yt_vlan_port_egrTagMode_set
 * @endinternal
 *
 * @brief         Set port vlan egress tag mode depend on vlan type (CVLAN or SVLAN)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[in]     tagMode             -egress vlan tag mode,refer to yt_egr_tag_mode_t
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_vlan_port_egrTagMode_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port,  yt_egr_tag_mode_t tagMode);


/**
 * @internal      yt_vlan_port_egrTagMode_get
 * @endinternal
 *
 * @brief         Get port vlan egress tag mode depend on vlan type (CVLAN or SVLAN)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[out]    pTagMode            -egress vlan tag mode,refer to yt_egr_tag_mode_t
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_vlan_port_egrTagMode_get(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port,  yt_egr_tag_mode_t *pTagMode);


/**
 * @internal      yt_vlan_port_egrDefVid_set
 * @endinternal
 *
 * @brief         Set port vlan egress default vid depend on vlan type (CVLAN or SVLAN)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[in]     default_vid         -vlan id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_vlan_port_egrDefVid_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_vlan_t default_vid);


/**
 * @internal      yt_vlan_port_egrDefVid_get
 * @endinternal
 *
 * @brief         Get port vlan egress default vid depend on vlan type (CVLAN or SVLAN)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[out]    pDefault_vid        -vlan id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_vlan_port_egrDefVid_get(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_vlan_t *pDefault_vid);



/**
 * @internal      yt_vlan_egrTpid_set
 * @endinternal
 *
 * @brief         Set vlan egress tpids array
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     tpids               -yt_vlan.h
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_vlan_egrTpid_set(yt_unit_t unit,  yt_tpid_profiles_t tpids);


/**
 * @internal      yt_vlan_egrTpid_get
 * @endinternal
 *
 * @brief         Get vlan egress tpids array
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[out]    pTpids              -yt_vlan.h
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_vlan_egrTpid_get(yt_unit_t unit,  yt_tpid_profiles_t *pTpids);


/**
 * @internal      yt_vlan_port_egrTpidSel_set
 * @endinternal
 *
 * @brief         Set vlan egress tpid index to choose tpid
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[in]     tpidIdx             -index of tpid tpids array
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_vlan_port_egrTpidSel_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t port, uint8_t tpidIdx);


/**
 * @internal      yt_vlan_port_egrTpidSel_get
 * @endinternal
 *
 * @brief         Get vlan egress tpid selected index
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[out]    pTpidIdx            -index of tpid tpids array
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_vlan_port_egrTpidSel_get(yt_unit_t unit, yt_vlan_type_t type, yt_port_t port, uint8_t *pTpidIdx);


/**
 * @internal      yt_vlan_port_egrTransparent_set
 * @endinternal
 *
 * @brief         Set vlan egress transparent,packets from the ports specified by port_mask will bypass tag mode on egress port
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -egress port num
 * @param[in]     enable              -enable or disable
 * @param[in]     port_mask           -ignress port bit mask to transparent
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_vlan_port_egrTransparent_set(yt_unit_t unit, yt_vlan_type_t type,  yt_port_t port,  yt_enable_t enable, yt_port_mask_t port_mask);


/**
 * @internal      yt_vlan_port_egrTransparent_get
 * @endinternal
 *
 * @brief         Get vlan egress transparent state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -egress port num
 * @param[out]    pEnable             -enable or disable
 * @param[out]    pPort_mask          -ignress port bit mask to transparent
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t yt_vlan_port_egrTransparent_get(yt_unit_t unit, yt_vlan_type_t type,  yt_port_t port,  yt_enable_t *pEnable, yt_port_mask_t *pPort_mask);


/**
 * @internal      yt_vlan_port_egrFilter_enable_set
 * @endinternal
 *
 * @brief         Set vlan egress filter state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enable             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_port_egrFilter_enable_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable);


/**
 * @internal      yt_vlan_port_egrFilter_enable_get
 * @endinternal
 *
 * @brief         Get vlan egress filter state
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnable            -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_port_egrFilter_enable_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable);


/**
 * @internal      yt_vlan_port_vidTypeSel_set
 * @endinternal
 *
 * @brief         Set port vid type select (CVLAN or SVLAN)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_port_vidTypeSel_set(yt_unit_t unit, yt_port_t port, yt_vlan_type_t type);


/**
 * @internal      yt_vlan_port_vidTypeSel_get
 * @endinternal
 *
 * @brief         Get port vid select type (CVLAN or SVLAN)
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pType               -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  yt_vlan_port_vidTypeSel_get(yt_unit_t unit, yt_port_t port, yt_vlan_type_t *pType);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
