#include "yt_error.h"
#include "osal_mem.h"
#include "osal_print.h"
#include "fal_cmm.h"
#include "fal_tiger_vlan_translate.h"
#include "hal_mem.h"
#include "fal_tiger_struct.h"
#include "fal_tiger_entry.h"
#include "fal_tiger_mem.h"

yt_ret_t fal_tiger_vlan_trans_untagPvidIgnore_set(yt_unit_t unit, yt_vlan_type_t type,  yt_port_t  port,  yt_enable_t enable)
{
    vlan_trans_untag_vid_mode_ctrl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t tmpMode = 0;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, VLAN_TRANS_UNTAG_VID_MODE_CTRLm, 0,sizeof(vlan_trans_untag_vid_mode_ctrl_t), &entry), ret);
    HAL_FIELD_GET(VLAN_TRANS_UNTAG_VID_MODE_CTRLm, VLAN_TRANS_UNTAG_VID_MODE_CTRL_MODEf, &entry, &tmpMode);
    if(VLAN_TYPE_CVLAN == type)
    {
        if(enable)
        {
            SET_BIT(tmpMode, macid << 1);
        }
        else
        {
            CLEAR_BIT(tmpMode, macid << 1);
        }
    }
    else if(VLAN_TYPE_SVLAN == type)
    {
        if(enable)
        {
            SET_BIT(tmpMode, (macid << 1) + 1);
        }
        else
        {
            CLEAR_BIT(tmpMode, (macid << 1) + 1);
        }
    }
    HAL_FIELD_SET(VLAN_TRANS_UNTAG_VID_MODE_CTRLm, VLAN_TRANS_UNTAG_VID_MODE_CTRL_MODEf, &entry, tmpMode);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, VLAN_TRANS_UNTAG_VID_MODE_CTRLm, 0, sizeof(vlan_trans_untag_vid_mode_ctrl_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_vlan_trans_untagPvidIgnore_get(yt_unit_t unit, yt_vlan_type_t  type,  yt_port_t  port,  yt_enable_t *pEnable)
{
    vlan_trans_untag_vid_mode_ctrl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t tmpMode = 0;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    *pEnable = YT_DISABLE;
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, VLAN_TRANS_UNTAG_VID_MODE_CTRLm, 0,sizeof(vlan_trans_untag_vid_mode_ctrl_t), &entry), ret);
    HAL_FIELD_GET(VLAN_TRANS_UNTAG_VID_MODE_CTRLm, VLAN_TRANS_UNTAG_VID_MODE_CTRL_MODEf, &entry, &tmpMode);
    if ((VLAN_TYPE_CVLAN == type && (IS_BIT_SET(tmpMode, macid << 1))) ||
        (VLAN_TYPE_SVLAN == type && (IS_BIT_SET(tmpMode, (macid << 1) + 1))))
    {
        *pEnable = YT_ENABLE;
    }

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_vlan_trans_rangeProfile_add(yt_unit_t unit, yt_vlan_range_group_t vlan_range, yt_profile_id_t *pProfile_id)
{
    vlan_range_profilen_t vlan_range_profilen;
    cmm_err_t ret = CMM_ERR_OK;
    yt_vlan_range_group_t exist_vlan_range;
    yt_profile_id_t profile_id;

    /* profile 0 always used as default setting for all ports */
    for(profile_id = 1; profile_id < CAL_MAX_VLAN_RANGE_PROFILE_NUM(unit); profile_id++)
    {
        CMM_ERR_CHK(fal_tiger_vlan_trans_rangeProfile_get(unit, profile_id, &exist_vlan_range), ret);
        /* if all max id are zero, treate it's a blank entry */
        if(!exist_vlan_range.vid_range0_max && !exist_vlan_range.vid_range1_max &&
            !exist_vlan_range.vid_range2_max && !exist_vlan_range.vid_range3_max)
        {
            break;
        }
    }

    CMM_PARAM_CHK(CAL_MAX_VLAN_RANGE_PROFILE_NUM(unit) == profile_id, CMM_ERR_TABLE_FULL);

    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MIN_VID0f, &vlan_range_profilen, vlan_range.vid_range0_min);
    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID0f, &vlan_range_profilen, vlan_range.vid_range0_max);
    
    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MIN_VID1_0f, &vlan_range_profilen, (vlan_range.vid_range1_min & 0xFF));
    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MIN_VID1_1f, &vlan_range_profilen, (vlan_range.vid_range1_min >> 8));
    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID1f, &vlan_range_profilen, vlan_range.vid_range1_max);
    
    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MIN_VID2f, &vlan_range_profilen, vlan_range.vid_range2_min);
    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID2_0f, &vlan_range_profilen, vlan_range.vid_range2_max & 0xF);
    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID2_1f, &vlan_range_profilen, vlan_range.vid_range2_max >> 4);
    
    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MIN_VID3f, &vlan_range_profilen, vlan_range.vid_range3_min);
    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID3f, &vlan_range_profilen, vlan_range.vid_range3_max);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, VLAN_RANGE_PROFILENm, profile_id, sizeof(vlan_range_profilen), &vlan_range_profilen), ret);

    *pProfile_id = profile_id;

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_vlan_trans_rangeProfile_get(yt_unit_t unit, yt_profile_id_t profile_id, yt_vlan_range_group_t *pVlan_range)
{
    vlan_range_profilen_t vlan_range_profilen;
    cmm_err_t ret = CMM_ERR_OK;
    uint16_t tmp_range1 = 0;
    uint16_t tmp_range2 = 0;

    CMM_PARAM_CHK(0 == profile_id, CMM_ERR_EXCEED_RANGE);
    CMM_PARAM_CHK(CAL_MAX_VLAN_RANGE_PROFILE_NUM(unit) <= profile_id, CMM_ERR_EXCEED_RANGE);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, VLAN_RANGE_PROFILENm, profile_id, sizeof(vlan_range_profilen_t), &vlan_range_profilen), ret);
    
    HAL_FIELD_GET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MIN_VID0f, &vlan_range_profilen, &pVlan_range->vid_range0_min);
    HAL_FIELD_GET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID0f, &vlan_range_profilen, &pVlan_range->vid_range0_max);
    
    HAL_FIELD_GET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MIN_VID1_0f, &vlan_range_profilen, &tmp_range1);
    HAL_FIELD_GET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MIN_VID1_1f, &vlan_range_profilen, &tmp_range2);
    HAL_FIELD_GET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID1f, &vlan_range_profilen, &pVlan_range->vid_range1_max);
    pVlan_range->vid_range1_min = (tmp_range2 << 8) | tmp_range1;

    tmp_range2 = tmp_range1 = 0;
    HAL_FIELD_GET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MIN_VID2f, &vlan_range_profilen, &pVlan_range->vid_range2_min);
    HAL_FIELD_GET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID2_0f, &vlan_range_profilen, &tmp_range1);
    HAL_FIELD_GET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID2_1f, &vlan_range_profilen, &tmp_range2);
    pVlan_range->vid_range2_max = (tmp_range2 << 4) | tmp_range1;
    
    HAL_FIELD_GET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MIN_VID3f, &vlan_range_profilen, &pVlan_range->vid_range3_min);
    HAL_FIELD_GET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID3f, &vlan_range_profilen, &pVlan_range->vid_range3_max);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_vlan_trans_rangeProfile_del(yt_unit_t unit, yt_profile_id_t profile_id)
{
    vlan_range_profilen_t vlan_range_profilen;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(0 == profile_id, CMM_ERR_EXCEED_RANGE);
    CMM_PARAM_CHK(CAL_MAX_VLAN_RANGE_PROFILE_NUM(unit) <= profile_id, CMM_ERR_EXCEED_RANGE);

    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID0f, &vlan_range_profilen, 0);
    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID1f, &vlan_range_profilen, 0);
    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID2_0f, &vlan_range_profilen, 0);
    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID2_1f, &vlan_range_profilen, 0);
    HAL_FIELD_SET(VLAN_RANGE_PROFILENm, VLAN_RANGE_PROFILEN_MAX_VID3f, &vlan_range_profilen, 0);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, VLAN_RANGE_PROFILENm, profile_id, sizeof(vlan_range_profilen), &vlan_range_profilen), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_vlan_trans_port_rangeProfileSel_set(yt_unit_t unit, yt_port_t  port, yt_profile_id_t profile_id)
{
    port_vlan_ctrl1n_t tmpCtrl;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_PARAM_CHK(CAL_MAX_VLAN_RANGE_PROFILE_NUM(unit) <= profile_id, CMM_ERR_INPUT);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PORT_VLAN_CTRL1Nm, macid, sizeof(tmpCtrl), &tmpCtrl), ret);
    HAL_FIELD_SET(PORT_VLAN_CTRL1Nm, PORT_VLAN_CTRL1N_VLAN_RANGE_PROFILE_IDf, &tmpCtrl, profile_id);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, PORT_VLAN_CTRL1Nm, macid, sizeof(tmpCtrl), &tmpCtrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_vlan_trans_port_rangeProfileSel_get(yt_unit_t unit, yt_port_t  port, yt_profile_id_t *pProfile_id)
{
    port_vlan_ctrl1n_t tmpCtrl;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PORT_VLAN_CTRL1Nm, macid, sizeof(tmpCtrl), &tmpCtrl), ret);
    HAL_FIELD_GET(PORT_VLAN_CTRL1Nm, PORT_VLAN_CTRL1N_VLAN_RANGE_PROFILE_IDf, &tmpCtrl, pProfile_id);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_vlan_trans_mode_set(yt_unit_t unit, yt_port_t  port, yt_vlan_range_trans_mode_t rangeMode)
{
    port_vlan_ctrl1n_t tmpCtrl;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PORT_VLAN_CTRL1Nm, macid, sizeof(tmpCtrl), &tmpCtrl), ret);
    HAL_FIELD_SET(PORT_VLAN_CTRL1Nm, PORT_VLAN_CTRL1N_VLAN_RANGE_MODEf, &tmpCtrl, rangeMode);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, PORT_VLAN_CTRL1Nm, macid, sizeof(tmpCtrl), &tmpCtrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_vlan_trans_mode_get(yt_unit_t unit, yt_port_t  port, yt_vlan_range_trans_mode_t *pRangeMode)
{
    port_vlan_ctrl1n_t tmpCtrl;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);
    
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PORT_VLAN_CTRL1Nm, macid, sizeof(tmpCtrl), &tmpCtrl), ret);
    HAL_FIELD_GET(PORT_VLAN_CTRL1Nm, PORT_VLAN_CTRL1N_VLAN_RANGE_MODEf, &tmpCtrl, pRangeMode);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_vlan_igr_trans_table_add(yt_unit_t unit, yt_vlan_trans_tbl_t *pRuleTbl,  yt_vlan_trans_action_tbl_t *pAction, yt_trans_tbl_id_t *pEntry_id)
{
    vlan_xlate_tbl_t entry;
    vlan_xlate_action_tbl_t entry_action;
    cmm_err_t ret = CMM_ERR_OK;
    yt_port_mask_t  macmask;
    yt_port_mask_t  portmask;
    yt_trans_tbl_id_t entry_idx;
    uint32_t valid = 0;
    uint32_t tmpPortmask;

    for(entry_idx = 0; entry_idx < CAL_VLAN_XLATE_ENTRY_NUM(unit); entry_idx++)
    {
        CMM_ERR_CHK(HAL_TBL_REG_READ(unit, VLAN_XLATE_TBLm, entry_idx, sizeof(vlan_xlate_tbl_t), &entry), ret);
        HAL_FIELD_GET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_VALIDf, &entry, &valid);
        HAL_FIELD_GET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_SRC_PORT_MASKf, &entry, &tmpPortmask);
        if (!valid && !tmpPortmask)
        {
            break;
        }
    }
    CMM_PARAM_CHK(CAL_VLAN_XLATE_ENTRY_NUM(unit) == entry_idx, CMM_ERR_TABLE_FULL);

    portmask.portbits[0] = pRuleTbl->valid_port_mask;
    CAL_YTPLIST_TO_MLIST(unit, portmask, macmask);
    HAL_FIELD_SET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_VALIDf, &entry, YT_ENABLE);
    HAL_FIELD_SET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_SVID_0f, &entry, (pRuleTbl->svid&0x1));
    HAL_FIELD_SET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_SVID_1f, &entry, (pRuleTbl->svid>>1));
    HAL_FIELD_SET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_SVID_INCLf, &entry, pRuleTbl->svid_valid);
    HAL_FIELD_SET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_STAG_FMTf, &entry, pRuleTbl->stag_format);
    HAL_FIELD_SET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_STAG_FMT_INCLf, &entry, pRuleTbl->stag_format_valid);
    HAL_FIELD_SET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_CVIDf, &entry, pRuleTbl->cvid);
    HAL_FIELD_SET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_CVID_INCLf, &entry, pRuleTbl->cvid_valid);
    HAL_FIELD_SET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_CTAG_FMTf, &entry, pRuleTbl->ctag_format);
    HAL_FIELD_SET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_CTAG_FMT_INCLf, &entry, pRuleTbl->ctag_format_valid);
    HAL_FIELD_SET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_SRC_PORT_MASKf, &entry, macmask.portbits[0]);
    
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, VLAN_XLATE_TBLm, entry_idx, sizeof(vlan_xlate_tbl_t), &entry), ret);

    HAL_FIELD_SET(VLAN_XLATE_ACTION_TBLm, VLAN_XLATE_ACTION_TBL_SVID_CMDf, &entry_action, pAction->svid_action);
    HAL_FIELD_SET(VLAN_XLATE_ACTION_TBLm, VLAN_XLATE_ACTION_TBL_NEW_SVID_0f, &entry_action, pAction->assign_svid& 0x3F);
    HAL_FIELD_SET(VLAN_XLATE_ACTION_TBLm, VLAN_XLATE_ACTION_TBL_NEW_SVID_1f, &entry_action, pAction->assign_svid >> 6);
    HAL_FIELD_SET(VLAN_XLATE_ACTION_TBLm, VLAN_XLATE_ACTION_TBL_CVID_CMDf, &entry_action, pAction->cvid_action);
    HAL_FIELD_SET(VLAN_XLATE_ACTION_TBLm, VLAN_XLATE_ACTION_TBL_NEW_CVIDf, &entry_action, pAction->assign_cvid);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, VLAN_XLATE_ACTION_TBLm, entry_idx, sizeof(vlan_xlate_action_tbl_t), &entry_action), ret);

    *pEntry_id = entry_idx;

    return CMM_ERR_OK;
}


yt_ret_t  fal_tiger_vlan_igr_trans_table_get(yt_unit_t unit,  yt_trans_tbl_id_t entry_id, yt_vlan_trans_tbl_t *pRuleTbl,  yt_vlan_trans_action_tbl_t *pAction)
{
    vlan_xlate_tbl_t entry;
    vlan_xlate_action_tbl_t entry_action;
    cmm_err_t ret = CMM_ERR_OK;
    yt_port_mask_t portmask;
    yt_port_mask_t macmask;
    uint32_t tmpPortmask;
    uint32_t valid = 0;
    uint16_t svid0 = 0, svid1 = 0;
    
    CMM_PARAM_CHK(CAL_VLAN_XLATE_ENTRY_NUM(unit) <= entry_id, CMM_ERR_EXCEED_RANGE);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, VLAN_XLATE_TBLm, entry_id, sizeof(vlan_xlate_tbl_t), &entry), ret);
    HAL_FIELD_GET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_VALIDf, &entry, &valid);
    HAL_FIELD_GET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_SRC_PORT_MASKf, &entry, &tmpPortmask);
    if (!valid && !tmpPortmask)
    {
        osal_memset(pRuleTbl, 0, sizeof(yt_vlan_trans_tbl_t));
        osal_memset(pAction, 0, sizeof(yt_vlan_trans_action_tbl_t));
        return CMM_ERR_OK;
    }
    HAL_FIELD_GET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_SVID_INCLf, &entry, &pRuleTbl->svid_valid);
    HAL_FIELD_GET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_SVID_0f, &entry, &svid0);
    HAL_FIELD_GET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_SVID_1f, &entry, &svid1);
    pRuleTbl->svid = svid0 | (svid1 << 1);
    
    HAL_FIELD_GET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_STAG_FMT_INCLf, &entry, &pRuleTbl->stag_format_valid);
    HAL_FIELD_GET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_STAG_FMTf, &entry, &pRuleTbl->stag_format);
    HAL_FIELD_GET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_CVID_INCLf, &entry, &pRuleTbl->cvid_valid);
    HAL_FIELD_GET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_CVIDf, &entry, &pRuleTbl->cvid);
    HAL_FIELD_GET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_CTAG_FMT_INCLf, &entry, &pRuleTbl->ctag_format_valid);
    HAL_FIELD_GET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_CTAG_FMTf, &entry, &pRuleTbl->ctag_format);
    HAL_FIELD_GET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_SRC_PORT_MASKf, &entry, macmask.portbits);    
    CAL_MLIST_TO_YTPLIST(unit, macmask, portmask);
    pRuleTbl->valid_port_mask= portmask.portbits[0];

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, VLAN_XLATE_ACTION_TBLm, entry_id, sizeof(vlan_xlate_action_tbl_t), &entry_action), ret);
    HAL_FIELD_GET(VLAN_XLATE_ACTION_TBLm, VLAN_XLATE_ACTION_TBL_SVID_CMDf, &entry_action, &pAction->svid_action);
    HAL_FIELD_GET(VLAN_XLATE_ACTION_TBLm, VLAN_XLATE_ACTION_TBL_NEW_SVID_0f, &entry_action, &svid0);
    HAL_FIELD_GET(VLAN_XLATE_ACTION_TBLm, VLAN_XLATE_ACTION_TBL_NEW_SVID_1f, &entry_action, &svid1);
    pAction->assign_svid= svid0 | (svid1<< 6);
    HAL_FIELD_GET(VLAN_XLATE_ACTION_TBLm, VLAN_XLATE_ACTION_TBL_CVID_CMDf, &entry_action, &pAction->cvid_action);
    HAL_FIELD_GET(VLAN_XLATE_ACTION_TBLm, VLAN_XLATE_ACTION_TBL_NEW_CVIDf, &entry_action, &pAction->assign_cvid);

    return CMM_ERR_OK;
}


yt_ret_t  fal_tiger_vlan_igr_trans_table_del(yt_unit_t unit, yt_trans_tbl_id_t entry_id)
{
    vlan_xlate_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(CAL_VLAN_XLATE_ENTRY_NUM(unit) <= entry_id, CMM_ERR_EXCEED_RANGE);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, VLAN_XLATE_TBLm, entry_id, sizeof(vlan_xlate_tbl_t), &entry), ret);
    HAL_FIELD_SET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_VALIDf, &entry, YT_DISABLE);
    HAL_FIELD_SET(VLAN_XLATE_TBLm, VLAN_XLATE_TBL_SRC_PORT_MASKf, &entry, 0);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, VLAN_XLATE_TBLm, entry_id, sizeof(vlan_xlate_tbl_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_vlan_egr_trans_table_add(yt_unit_t unit, yt_egr_vlan_trans_tbl_t  *pRuleTbl, yt_egr_vlan_trans_action_tbl_t *pAction, yt_trans_tbl_id_t *pEntry_id)
{
    egr_vlan_trans_rule_ctrln_t entry;
    egr_vlan_trans_rule_ctrl1n_t entry1;
    egr_vlan_trans_data_ctrln_t entry_data;
    cmm_err_t ret = CMM_ERR_OK;
    yt_port_mask_t  macmask;
    yt_port_mask_t  portmask;
    yt_trans_tbl_id_t entry_idx;
    uint32_t valid = 0;
    uint32_t tmpPortmask;

    for(entry_idx = 0; entry_idx < CAL_VLAN_EGR_XLATE_TBL_NUM(unit); entry_idx++)
    {
        CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_VLAN_TRANS_RULE_CTRLNm, entry_idx, sizeof(egr_vlan_trans_rule_ctrln_t), &entry), ret);
        HAL_FIELD_GET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_VALIDf, &entry, &valid);
        HAL_FIELD_GET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_PORT_MASKf, &entry, &tmpPortmask);
        if (!valid && !tmpPortmask)
        {
            break;
        }
    }
    CMM_PARAM_CHK(CAL_VLAN_EGR_XLATE_TBL_NUM(unit) == entry_idx, CMM_ERR_TABLE_FULL);

    portmask.portbits[0] = pRuleTbl->valid_port_mask;
    CAL_YTPLIST_TO_MLIST(unit, portmask, macmask);

    HAL_FIELD_SET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_VALIDf, &entry, YT_ENABLE);
    HAL_FIELD_SET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_CVID_INCLf, &entry, pRuleTbl->cvid_valid);
    HAL_FIELD_SET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_SVID_INCLf, &entry, pRuleTbl->svid_valid);
    HAL_FIELD_SET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_MVR_INCLf, &entry, pRuleTbl->mvr_valid);
    HAL_FIELD_SET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_PORT_MASKf, &entry, macmask.portbits[0]);
    HAL_FIELD_SET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_VID0f, &entry, pRuleTbl->vid);
    HAL_FIELD_SET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_PKT_CTAG_INCLf, &entry, pRuleTbl->original_ctag_format_valid);
    HAL_FIELD_SET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_PKT_STAG_INCLf, &entry, pRuleTbl->original_stag_format_valid);

    HAL_FIELD_SET(EGR_VLAN_TRANS_RULE_CTRL1Nm, EGR_VLAN_TRANS_RULE_CTRL1N_VID_RANGE_TYPEf, &entry1, pRuleTbl->vid_range_mode);
    HAL_FIELD_SET(EGR_VLAN_TRANS_RULE_CTRL1Nm, EGR_VLAN_TRANS_RULE_CTRL1N_VID1f, &entry1, pRuleTbl->vid_range_min);
    HAL_FIELD_SET(EGR_VLAN_TRANS_RULE_CTRL1Nm, EGR_VLAN_TRANS_RULE_CTRL1N_VID2f, &entry1, pRuleTbl->vid_range_max);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, EGR_VLAN_TRANS_RULE_CTRLNm, entry_idx, sizeof(egr_vlan_trans_rule_ctrln_t), &entry), ret);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, EGR_VLAN_TRANS_RULE_CTRL1Nm, entry_idx, sizeof(egr_vlan_trans_rule_ctrl1n_t), &entry1), ret);
    
    HAL_FIELD_SET(EGR_VLAN_TRANS_DATA_CTRLNm, EGR_VLAN_TRANS_DATA_CTRLN_SVID_ENf, &entry_data, pAction->svid_enable);
    HAL_FIELD_SET(EGR_VLAN_TRANS_DATA_CTRLNm, EGR_VLAN_TRANS_DATA_CTRLN_CVID_ENf, &entry_data, pAction->cvid_enable);
    HAL_FIELD_SET(EGR_VLAN_TRANS_DATA_CTRLNm, EGR_VLAN_TRANS_DATA_CTRLN_SVIDf, &entry_data, pAction->svid);
    HAL_FIELD_SET(EGR_VLAN_TRANS_DATA_CTRLNm, EGR_VLAN_TRANS_DATA_CTRLN_CVIDf, &entry_data, pAction->cvid);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, EGR_VLAN_TRANS_DATA_CTRLNm, entry_idx, sizeof(egr_vlan_trans_data_ctrln_t), &entry_data), ret);

    *pEntry_id = entry_idx;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_vlan_egr_trans_table_get(yt_unit_t unit, yt_trans_tbl_id_t entry_idx, yt_egr_vlan_trans_tbl_t  *pRuleTbl, yt_egr_vlan_trans_action_tbl_t *pAction)
{
    egr_vlan_trans_rule_ctrln_t entry;
    egr_vlan_trans_rule_ctrl1n_t entry1;
    egr_vlan_trans_data_ctrln_t entry_data;
    cmm_err_t ret = CMM_ERR_OK;
    yt_port_mask_t portmask;
    yt_port_mask_t  macmask;
    uint32_t valid = 0;
    uint32_t tmpPortmask;

    CMM_PARAM_CHK(CAL_VLAN_EGR_XLATE_TBL_NUM(unit) <= entry_idx, CMM_ERR_EXCEED_RANGE);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_VLAN_TRANS_RULE_CTRLNm, entry_idx, sizeof(egr_vlan_trans_rule_ctrln_t), &entry), ret);
    HAL_FIELD_GET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_VALIDf, &entry, &valid);
    HAL_FIELD_GET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_PORT_MASKf, &entry, &tmpPortmask);
    if (!valid && !tmpPortmask)
    {
        osal_memset(pRuleTbl, 0, sizeof(yt_egr_vlan_trans_tbl_t));
        osal_memset(pAction, 0, sizeof(yt_egr_vlan_trans_action_tbl_t));
        return CMM_ERR_OK;
    }
    
    HAL_FIELD_GET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_SVID_INCLf, &entry, &pRuleTbl->svid_valid);
    HAL_FIELD_GET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_CVID_INCLf, &entry, &pRuleTbl->cvid_valid);
    HAL_FIELD_GET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_MVR_INCLf, &entry, &pRuleTbl->mvr_valid);
    HAL_FIELD_GET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_PORT_MASKf, &entry, macmask.portbits);
    CAL_MLIST_TO_YTPLIST(unit, macmask, portmask);
    pRuleTbl->valid_port_mask = portmask.portbits[0];
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_VLAN_TRANS_RULE_CTRL1Nm, entry_idx, sizeof(egr_vlan_trans_rule_ctrl1n_t), &entry1), ret);
    HAL_FIELD_GET(EGR_VLAN_TRANS_RULE_CTRL1Nm, EGR_VLAN_TRANS_RULE_CTRL1N_VID_RANGE_TYPEf, &entry1, &pRuleTbl->vid_range_mode);
    HAL_FIELD_GET(EGR_VLAN_TRANS_RULE_CTRL1Nm, EGR_VLAN_TRANS_RULE_CTRL1N_VID1f, &entry1, &pRuleTbl->vid_range_min);
    HAL_FIELD_GET(EGR_VLAN_TRANS_RULE_CTRL1Nm, EGR_VLAN_TRANS_RULE_CTRL1N_VID2f, &entry1, &pRuleTbl->vid_range_max);    
    HAL_FIELD_GET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_VID0f, &entry, &pRuleTbl->vid);
    HAL_FIELD_GET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_PKT_CTAG_INCLf, &entry, &pRuleTbl->original_ctag_format_valid);
    HAL_FIELD_GET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_PKT_STAG_INCLf, &entry, &pRuleTbl->original_stag_format_valid);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_VLAN_TRANS_DATA_CTRLNm, entry_idx, sizeof(egr_vlan_trans_data_ctrln_t), &entry_data), ret);
    HAL_FIELD_GET(EGR_VLAN_TRANS_DATA_CTRLNm, EGR_VLAN_TRANS_DATA_CTRLN_SVID_ENf, &entry_data, &pAction->svid_enable);
    HAL_FIELD_GET(EGR_VLAN_TRANS_DATA_CTRLNm, EGR_VLAN_TRANS_DATA_CTRLN_CVID_ENf, &entry_data, &pAction->cvid_enable);
    HAL_FIELD_GET(EGR_VLAN_TRANS_DATA_CTRLNm, EGR_VLAN_TRANS_DATA_CTRLN_SVIDf, &entry_data, &pAction->svid);
    HAL_FIELD_GET(EGR_VLAN_TRANS_DATA_CTRLNm, EGR_VLAN_TRANS_DATA_CTRLN_CVIDf, &entry_data, &pAction->cvid);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_vlan_egr_trans_table_del(yt_unit_t unit, yt_trans_tbl_id_t entry_idx)
{
    egr_vlan_trans_rule_ctrln_t entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK(CAL_VLAN_EGR_XLATE_TBL_NUM(unit) <= entry_idx, CMM_ERR_EXCEED_RANGE);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, EGR_VLAN_TRANS_RULE_CTRLNm, entry_idx, sizeof(egr_vlan_trans_rule_ctrln_t), &entry), ret);
    HAL_FIELD_SET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_VALIDf, &entry, YT_DISABLE);
    HAL_FIELD_SET(EGR_VLAN_TRANS_RULE_CTRLNm, EGR_VLAN_TRANS_RULE_CTRLN_PORT_MASKf, &entry, 0);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, EGR_VLAN_TRANS_RULE_CTRLNm, entry_idx, sizeof(egr_vlan_trans_rule_ctrln_t), &entry), ret);

    return CMM_ERR_OK;
}
