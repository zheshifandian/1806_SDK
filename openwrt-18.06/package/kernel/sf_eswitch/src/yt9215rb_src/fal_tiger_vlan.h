/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/**
********************************************************************************
* @file fal_tiger_vlan.h
*
* @brief x
*
********************************************************************************
*/
#ifndef __FAL_TIGER_VLAN_H
#define __FAL_TIGER_VLAN_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "fal_cmm.h"


/**
 * @internal      fal_tiger_vlan_init
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_init(yt_unit_t unit);

extern yt_ret_t  fal_tiger_vlan_port_set(yt_unit_t unit,  yt_vlan_t vid,  yt_port_mask_t  member_portmask, yt_port_mask_t  untag_portmask);

extern yt_ret_t  fal_tiger_vlan_port_get(yt_unit_t unit,  yt_vlan_t vid,  yt_port_mask_t  *pMember_portmask, yt_port_mask_t  *pUntag_portmask);

extern yt_ret_t  fal_tiger_vlan_svlMode_enable_set(yt_unit_t unit, yt_vlan_t vid,  yt_enable_t enable);

extern yt_ret_t  fal_tiger_vlan_svlMode_enable_get(yt_unit_t unit, yt_vlan_t vid,  yt_enable_t *pEnable);


/**
 * @internal      fal_tiger_vlan_fid_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[in]     fid                 -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_fid_set(yt_unit_t unit, yt_vlan_t vid,  yt_fid_t fid);


/**
 * @internal      fal_tiger_vlan_fid_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     vid                 -vlan id
 * @param[out]    pFid                -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_fid_get(yt_unit_t unit, yt_vlan_t vid,  yt_fid_t *pFid);


/**
 * @internal      fal_tiger_vlan_ingrTpid_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     tpid                -yt_vlan.h
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_ingrTpid_set(yt_unit_t unit,  yt_tpid_profiles_t tpid);


/**
 * @internal      fal_tiger_vlan_ingrTpid_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[out]    pTpid               -yt_vlan.h
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_ingrTpid_get(yt_unit_t unit,  yt_tpid_profiles_t *pTpid);


/**
 * @internal      fal_tiger_vlan_port_ingrTpidMask_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[in]     tpidIdxMask         -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_port_ingrTpidMask_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_tpidprofile_id_mask_t tpidIdxMask);


/**
 * @internal      fal_tiger_vlan_port_ingrTpidMask_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[out]    pTpidIdxMask        -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_port_ingrTpidMask_get(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_tpidprofile_id_mask_t *pTpidIdxMask);


/**
 * @internal      fal_tiger_vlan_port_ingrDefaultVlan_set
 * @endinternal
 *
 * @brief         Description
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[in]     vid                 -vlan id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_port_ingrDefaultVlan_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_vlan_t vid);


/**
 * @internal      fal_tiger_vlan_port_ingrDefaultVlan_get
 * @endinternal
 *
 * @brief         Description
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[out]    pVid                -vlan id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_port_ingrDefaultVlan_get(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_vlan_t *pVid);

/**
 * @internal      fal_tiger_vlan_port_ingrFilter_enable_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enabled             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_port_ingrFilter_enable_set(yt_unit_t unit, yt_port_t  port, yt_enable_t enabled);


/**
 * @internal      fal_tiger_vlan_port_ingrFilter_enable_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnabled            -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_port_ingrFilter_enable_get(yt_unit_t unit, yt_port_t  port, yt_enable_t *pEnabled);

/**
 * @internal      fal_tiger_vlan_ingrTransparent_set
 * @endinternal
 *
 * @brief         Description
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     port_mask           -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_ingrTransparent_set(yt_unit_t unit, yt_port_t  port, yt_port_mask_t port_mask);


/**
 * @internal      fal_tiger_vlan_ingrTransparent_get
 * @endinternal
 *
 * @brief         Description
 * @note          APPLICABLE DEVICES  -Tiger
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pPort_mask          -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_ingrTransparent_get(yt_unit_t unit, yt_port_t  port, yt_port_mask_t *pPort_mask);


/**
 * @internal      fal_tiger_vlan_port_aft_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[in]     tagAFT              -yt_vlan.h
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_port_aft_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t port, yt_vlan_aft_t tagAFT);


/**
 * @internal      fal_tiger_vlan_port_aft_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[out]    pTagAFT             -yt_vlan.h
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_port_aft_get(yt_unit_t unit, yt_vlan_type_t type, yt_port_t port, yt_vlan_aft_t *pTagAFT);


/**
 * @internal      fal_tiger_vlan_port_egrTagMode_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[in]     tagMode             -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_vlan_port_egrTagMode_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port,  yt_egr_tag_mode_t tagMode);


/**
 * @internal      fal_tiger_vlan_port_egrTagMode_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[out]    pTagMode            -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_vlan_port_egrTagMode_get(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port,  yt_egr_tag_mode_t *pTagMode);


/**
 * @internal      fal_tiger_vlan_port_egrDefaultVid_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[in]     default_vid         -vlan id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_vlan_port_egrDefaultVid_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_vlan_t default_vid);


/**
 * @internal      fal_tiger_vlan_port_egrDefaultVid_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[out]    pDefault_vid        -vlan id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_vlan_port_egrDefaultVid_get(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_vlan_t *pDefault_vid);


/**
 * @internal      fal_tiger_vlan_egrTpid_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     tpids               -yt_vlan.h
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_vlan_egrTpid_set(yt_unit_t unit,  yt_tpid_profiles_t tpids);


/**
 * @internal      fal_tiger_vlan_egrTpid_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[out]    pTpids              -yt_vlan.h
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_vlan_egrTpid_get(yt_unit_t unit,  yt_tpid_profiles_t *pTpids);


/**
 * @internal      fal_tiger_vlan_port_egrTpidIdx_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[in]     tpidIdx             -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_vlan_port_egrTpidIdx_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t port, uint8_t tpidIdx);


/**
 * @internal      fal_tiger_vlan_port_egrTpidIdx_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[out]    pTpidIdx            -x
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_vlan_port_egrTpidIdx_get(yt_unit_t unit, yt_vlan_type_t type, yt_port_t port, uint8_t *pTpidIdx);


/**
 * @internal      fal_tiger_vlan_port_egrTransparent_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[in]     enable              -enable or disable
 * @param[in]     port_mask           -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_vlan_port_egrTransparent_set(yt_unit_t unit, yt_vlan_type_t type,  yt_port_t port,  yt_enable_t enable, yt_port_mask_t port_mask);


/**
 * @internal      fal_tiger_vlan_port_egrTransparent_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     type                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @param[in]     port                -port num
 * @param[out]    pEnable             -enable or disable
 * @param[out]    pPort_mask          -port bit mask
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t fal_tiger_vlan_port_egrTransparent_get(yt_unit_t unit, yt_vlan_type_t type,  yt_port_t port,  yt_enable_t *pEnable, yt_port_mask_t *pPort_mask);


/**
 * @internal      fal_tiger_vlan_port_egrFilter_en_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     enabled             -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_port_egrFilter_en_set(yt_unit_t unit, yt_port_t port, yt_enable_t enabled);


/**
 * @internal      fal_tiger_vlan_port_egrFilter_en_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pEnabled            -enable or disable
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_vlan_port_egrFilter_en_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnabled);


/**
 * @internal      fal_tiger_port_vidTypeSel_set
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[in]     mode                -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_port_vidTypeSel_set(yt_unit_t unit, yt_port_t port, yt_vlan_type_t mode);


/**
 * @internal      fal_tiger_port_vidTypeSel_get
 * @endinternal
 *
 * @brief         Description
 * @param[in]     unit                -unit id
 * @param[in]     port                -port num
 * @param[out]    pMode               -VLAN_TYPE_CVLAN or VLAN_TYPE_SVLAN
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
extern yt_ret_t  fal_tiger_port_vidTypeSel_get(yt_unit_t unit, yt_port_t port, yt_vlan_type_t *pMode);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
