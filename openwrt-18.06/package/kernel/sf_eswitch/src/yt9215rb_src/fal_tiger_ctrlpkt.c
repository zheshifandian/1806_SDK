/******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
*******************************************************************************
*  File Name     : fal_tiger_ctrlpkt.c
*  Version       : Initial Draft
*  Created       : 2022/3/1
*  Last Modified :
*  Description   : To get set control packet action
*
******************************************************************************/
/**************************************************
 *      Include header files                       *
 **************************************************/
#include "fal_tiger_ctrlpkt.h"
#include "yt_error.h"
#include "hal_mem.h"
#include "fal_tiger_struct.h"
#include "fal_tiger_entry.h"
#include "fal_tiger_mem.h"
/**************************************************
 *      Constants or macros Declaration            *
 **************************************************/

/**************************************************
 *      Global variables                           *
 **************************************************/

/**************************************************
 *      Functions Declaration                      *
 **************************************************/



/**************************************************
 *      Functions implementations                  *
 **************************************************/

yt_ret_t  fal_tiger_ctrlpkt_unknown_ucast_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_uc_unknown_act_ctrl_t act;
    uint32_t l2_act;

    yt_macid_t macid = CAL_YTP_TO_MAC(unit,port);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_UC_UNKNOWN_ACT_CTRLm, 0, sizeof(l2_uc_unknown_act_ctrl_t), &act), ret);    
    HAL_FIELD_GET(L2_UC_UNKNOWN_ACT_CTRLm, L2_UC_UNKNOWN_ACT_CTRL_L2_UC_UNKNOWN_ACTf, &act, &l2_act);
    l2_act &= (~(3 << (macid * 2)));
    l2_act |= act_ctrl << (macid * 2);
    HAL_FIELD_SET(L2_UC_UNKNOWN_ACT_CTRLm, L2_UC_UNKNOWN_ACT_CTRL_L2_UC_UNKNOWN_ACTf, &act, l2_act);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_UC_UNKNOWN_ACT_CTRLm, 0, sizeof(l2_uc_unknown_act_ctrl_t), &act), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_ctrlpkt_unknown_ucast_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_uc_unknown_act_ctrl_t act;
    uint32_t l2_act;

    yt_macid_t macid = CAL_YTP_TO_MAC(unit,port);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_UC_UNKNOWN_ACT_CTRLm, 0, sizeof(l2_uc_unknown_act_ctrl_t), &act), ret);
    HAL_FIELD_GET(L2_UC_UNKNOWN_ACT_CTRLm, L2_UC_UNKNOWN_ACT_CTRL_L2_UC_UNKNOWN_ACTf, &act, &l2_act);
    *pAct_ctrl = (l2_act >> (macid * 2)) & 0x3;

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_ctrlpkt_unknown_mcast_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_mc_unknown_act_ctrl_t act;
    uint32_t l2_act;

    yt_macid_t macid = CAL_YTP_TO_MAC(unit,port);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_MC_UNKNOWN_ACT_CTRLm, 0, sizeof(l2_mc_unknown_act_ctrl_t), &act), ret);
    HAL_FIELD_GET(L2_MC_UNKNOWN_ACT_CTRLm, L2_MC_UNKNOWN_ACT_CTRL_L2_MC_UNKNOWN_ACTf, &act, &l2_act);
    l2_act &= (~(3 << (macid * 2)));
    l2_act |= act_ctrl << (macid * 2);
    HAL_FIELD_SET(L2_MC_UNKNOWN_ACT_CTRLm, L2_MC_UNKNOWN_ACT_CTRL_L2_MC_UNKNOWN_ACTf, &act, l2_act);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_MC_UNKNOWN_ACT_CTRLm, 0, sizeof(l2_mc_unknown_act_ctrl_t), &act), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_ctrlpkt_unknown_mcast_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl)
{
    cmm_err_t ret                   = CMM_ERR_OK;
    l2_mc_unknown_act_ctrl_t act;
    uint32_t l2_act;

    yt_macid_t macid = CAL_YTP_TO_MAC(unit,port);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_MC_UNKNOWN_ACT_CTRLm, 0, sizeof(l2_mc_unknown_act_ctrl_t), &act), ret);
    HAL_FIELD_GET(L2_MC_UNKNOWN_ACT_CTRLm, L2_MC_UNKNOWN_ACT_CTRL_L2_MC_UNKNOWN_ACTf, &act, &l2_act);
    *pAct_ctrl = (l2_act >> (macid * 2)) & 0x3;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_ctrlpkt_arp_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl)
{
    l2_arp_bcast_per_port_ctrl_t entry;
    yt_macid_t macid;
    uint32_t copyMask;
    uint32_t dropMask;
    cmm_err_t ret = CMM_ERR_OK;

    macid = CAL_YTP_TO_MAC(unit,port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_ARP_BCAST_PER_PORT_CTRLm, 0, sizeof(l2_arp_bcast_per_port_ctrl_t), &entry), ret);

    HAL_FIELD_GET(L2_ARP_BCAST_PER_PORT_CTRLm, L2_ARP_BCAST_PER_PORT_CTRL_COPY_TO_CPUf, &entry, &copyMask);
    HAL_FIELD_GET(L2_ARP_BCAST_PER_PORT_CTRLm, L2_ARP_BCAST_PER_PORT_CTRL_DROPf, &entry, &dropMask);

    switch(act_ctrl)
    {
        case L2_ACTION_FWD:
            copyMask = (uint32_t)CLR_FIELD(copyMask, macid, 1);
            dropMask = (uint32_t)CLR_FIELD(dropMask, macid, 1);
            break;

        case L2_ACTION_TRAP:
            copyMask |= (uint32_t)(1UL<<macid);
            dropMask |= (uint32_t)(1UL<<macid);
            break;

        case L2_ACTION_DROP:
            copyMask = (uint32_t)CLR_FIELD(copyMask, macid, 1);
            dropMask |= (uint32_t)(1UL<<macid);
            break;

        case L2_ACTION_COPY:
            copyMask |= (uint32_t)(1UL<<macid);
            dropMask = (uint32_t)CLR_FIELD(dropMask, macid, 1);;
            break;

        default:
            return CMM_ERR_NOT_SUPPORT;
    }
    
    HAL_FIELD_SET(L2_ARP_BCAST_PER_PORT_CTRLm, L2_ARP_BCAST_PER_PORT_CTRL_COPY_TO_CPUf, &entry, copyMask);
    HAL_FIELD_SET(L2_ARP_BCAST_PER_PORT_CTRLm, L2_ARP_BCAST_PER_PORT_CTRL_DROPf, &entry, dropMask);
    
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_ARP_BCAST_PER_PORT_CTRLm, 0, sizeof(l2_arp_bcast_per_port_ctrl_t), &entry), ret);  
   
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_ctrlpkt_arp_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl)
{
    l2_arp_bcast_per_port_ctrl_t entry;
    yt_macid_t macid;
    uint32_t copyMask;
    uint32_t dropMask;
    cmm_err_t ret = CMM_ERR_OK;

    macid = CAL_YTP_TO_MAC(unit,port);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_ARP_BCAST_PER_PORT_CTRLm, 0, sizeof(l2_arp_bcast_per_port_ctrl_t), &entry), ret);
        
    HAL_FIELD_GET(L2_ARP_BCAST_PER_PORT_CTRLm, L2_ARP_BCAST_PER_PORT_CTRL_COPY_TO_CPUf, &entry, &copyMask);
    HAL_FIELD_GET(L2_ARP_BCAST_PER_PORT_CTRLm, L2_ARP_BCAST_PER_PORT_CTRL_DROPf, &entry, &dropMask);

    if (copyMask & (1UL<<macid))
    {
        if (dropMask & (1UL<<macid))
        {
            *pAct_ctrl = L2_ACTION_TRAP;
        }
        else
        {
            *pAct_ctrl = L2_ACTION_COPY;
        }
    }
    else
    {
        if (dropMask & (1UL<<macid))
        {
            *pAct_ctrl = L2_ACTION_DROP;
        }
        else
        {
            *pAct_ctrl = L2_ACTION_FWD;
        }
    }
   
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_ctrlpkt_nd_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl)
{
    l2_nd_per_port_ctrl_t entry;
    yt_macid_t macid;
    uint32_t copyMask;
    uint32_t dropMask;
    cmm_err_t ret = CMM_ERR_OK;

    macid = CAL_YTP_TO_MAC(unit,port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_ND_PER_PORT_CTRLm, 0, sizeof(l2_nd_per_port_ctrl_t), &entry), ret);
    HAL_FIELD_GET(L2_ND_PER_PORT_CTRLm, L2_ND_PER_PORT_CTRL_COPY_TO_CPUf, &entry, &copyMask);
    HAL_FIELD_GET(L2_ND_PER_PORT_CTRLm, L2_ND_PER_PORT_CTRL_DROPf, &entry, &dropMask);

    switch(act_ctrl)
    {
        case L2_ACTION_FWD:
            copyMask = (uint32_t)CLR_FIELD(copyMask, macid, 1);
            dropMask = (uint32_t)CLR_FIELD(dropMask, macid, 1);
            break;

        case L2_ACTION_TRAP:
            copyMask |= (uint32_t)(1UL<<macid);
            dropMask |= (uint32_t)(1UL<<macid);
            break;

        case L2_ACTION_DROP:
            copyMask = (uint32_t)CLR_FIELD(copyMask, macid, 1);
            dropMask |= (uint32_t)(1UL<<macid);
            break;

        case L2_ACTION_COPY:
            copyMask |= (uint32_t)(1UL<<macid);
            dropMask = (uint32_t)CLR_FIELD(dropMask, macid, 1);;
            break;

        default:
            return CMM_ERR_NOT_SUPPORT;
    }
    
    HAL_FIELD_SET(L2_ND_PER_PORT_CTRLm, L2_ND_PER_PORT_CTRL_COPY_TO_CPUf, &entry, copyMask);
    HAL_FIELD_SET(L2_ND_PER_PORT_CTRLm, L2_ND_PER_PORT_CTRL_DROPf, &entry, dropMask);
    
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_ND_PER_PORT_CTRLm, 0, sizeof(l2_nd_per_port_ctrl_t), &entry), ret);
    
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_ctrlpkt_nd_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl)
{
    l2_nd_per_port_ctrl_t entry;
    yt_macid_t macid;
    uint32_t copyMask;
    uint32_t dropMask;
    cmm_err_t ret = CMM_ERR_OK;

    macid = CAL_YTP_TO_MAC(unit,port);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_ND_PER_PORT_CTRLm, 0, sizeof(l2_nd_per_port_ctrl_t), &entry), ret);
        
    HAL_FIELD_GET(L2_ND_PER_PORT_CTRLm, L2_ND_PER_PORT_CTRL_COPY_TO_CPUf, &entry, &copyMask);
    HAL_FIELD_GET(L2_ND_PER_PORT_CTRLm, L2_ND_PER_PORT_CTRL_DROPf, &entry, &dropMask);

    if (copyMask & (1UL<<macid))
    {
        if (dropMask & (1UL<<macid))
        {
            *pAct_ctrl = L2_ACTION_TRAP;
        }
        else
        {
            *pAct_ctrl = L2_ACTION_COPY;
        }
    }
    else
    {
        if (dropMask & (1UL<<macid))
        {
            *pAct_ctrl = L2_ACTION_DROP;
        }
        else
        {
            *pAct_ctrl = L2_ACTION_FWD;
        }
    }
   
    return CMM_ERR_OK;    
}

yt_ret_t fal_tiger_ctrlpkt_lldp_eee_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl)
{
    l2_lldp_eee_per_port_ctrl_t entry;
    yt_macid_t macid;
    uint32_t copyMask;
    uint32_t dropMask;
    cmm_err_t ret = CMM_ERR_OK;

    macid = CAL_YTP_TO_MAC(unit,port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LLDP_EEE_PER_PORT_CTRLm, 0, sizeof(l2_lldp_eee_per_port_ctrl_t), &entry), ret);
    HAL_FIELD_GET(L2_LLDP_EEE_PER_PORT_CTRLm, L2_LLDP_EEE_PER_PORT_CTRL_COPY_TO_CPUf, &entry, &copyMask);
    HAL_FIELD_GET(L2_LLDP_EEE_PER_PORT_CTRLm, L2_LLDP_EEE_PER_PORT_CTRL_DROPf, &entry, &dropMask);

    switch(act_ctrl)
    {
        case L2_ACTION_FWD:
            copyMask = (uint32_t)CLR_FIELD(copyMask, macid, 1);
            dropMask = (uint32_t)CLR_FIELD(dropMask, macid, 1);
            break;

        case L2_ACTION_TRAP:
            copyMask |= (uint32_t)(1UL<<macid);
            dropMask |= (uint32_t)(1UL<<macid);
            break;

        case L2_ACTION_DROP:
            copyMask = (uint32_t)CLR_FIELD(copyMask, macid, 1);
            dropMask |= (uint32_t)(1UL<<macid);
            break;

        case L2_ACTION_COPY:
            copyMask |= (uint32_t)(1UL<<macid);
            dropMask = (uint32_t)CLR_FIELD(dropMask, macid, 1);;
            break;

        default:
            return CMM_ERR_NOT_SUPPORT;
    }
    
    HAL_FIELD_SET(L2_LLDP_EEE_PER_PORT_CTRLm, L2_LLDP_EEE_PER_PORT_CTRL_COPY_TO_CPUf, &entry, copyMask);
    HAL_FIELD_SET(L2_LLDP_EEE_PER_PORT_CTRLm, L2_LLDP_EEE_PER_PORT_CTRL_DROPf, &entry, dropMask);
    
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_LLDP_EEE_PER_PORT_CTRLm, 0, sizeof(l2_lldp_eee_per_port_ctrl_t), &entry), ret);
    
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_ctrlpkt_lldp_eee_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl)
{
    l2_lldp_eee_per_port_ctrl_t entry;
    yt_macid_t macid;
    uint32_t copyMask;
    uint32_t dropMask;
    cmm_err_t ret = CMM_ERR_OK;

    macid = CAL_YTP_TO_MAC(unit,port);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LLDP_EEE_PER_PORT_CTRLm, 0, sizeof(l2_lldp_eee_per_port_ctrl_t), &entry), ret);
        
    HAL_FIELD_GET(L2_LLDP_EEE_PER_PORT_CTRLm, L2_LLDP_EEE_PER_PORT_CTRL_COPY_TO_CPUf, &entry, &copyMask);
    HAL_FIELD_GET(L2_LLDP_EEE_PER_PORT_CTRLm, L2_LLDP_EEE_PER_PORT_CTRL_DROPf, &entry, &dropMask);

    if (copyMask & (1UL<<macid))
    {
        if (dropMask & (1UL<<macid))
        {
            *pAct_ctrl = L2_ACTION_TRAP;
        }
        else
        {
            *pAct_ctrl = L2_ACTION_COPY;
        }
    }
    else
    {
        if (dropMask & (1UL<<macid))
        {
            *pAct_ctrl = L2_ACTION_DROP;
        }
        else
        {
            *pAct_ctrl = L2_ACTION_FWD;
        }
    }
   
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_ctrlpkt_lldp_act_set(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t act_ctrl)
{
    l2_lldp_per_port_ctrl_t entry;
    yt_macid_t macid;
    uint32_t copyMask;
    uint32_t dropMask;
    cmm_err_t ret = CMM_ERR_OK;

    macid = CAL_YTP_TO_MAC(unit,port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LLDP_PER_PORT_CTRLm, 0, sizeof(l2_lldp_per_port_ctrl_t), &entry), ret);
    HAL_FIELD_GET(L2_LLDP_PER_PORT_CTRLm, L2_LLDP_PER_PORT_CTRL_COPY_TO_CPUf, &entry, &copyMask);
    HAL_FIELD_GET(L2_LLDP_PER_PORT_CTRLm, L2_LLDP_PER_PORT_CTRL_DROPf, &entry, &dropMask);

    switch(act_ctrl)
    {
        case L2_ACTION_FWD:
            copyMask = (uint32_t)CLR_FIELD(copyMask, macid, 1);
            dropMask = (uint32_t)CLR_FIELD(dropMask, macid, 1);
            break;

        case L2_ACTION_TRAP:
            copyMask |= (uint32_t)(1UL<<macid);
            dropMask |= (uint32_t)(1UL<<macid);
            break;

        case L2_ACTION_DROP:
            copyMask = (uint32_t)CLR_FIELD(copyMask, macid, 1);
            dropMask |= (uint32_t)(1UL<<macid);
            break;

        case L2_ACTION_COPY:
            copyMask |= (uint32_t)(1UL<<macid);
            dropMask = (uint32_t)CLR_FIELD(dropMask, macid, 1);;
            break;

        default:
            return CMM_ERR_NOT_SUPPORT;
    }
    
    HAL_FIELD_SET(L2_LLDP_PER_PORT_CTRLm, L2_LLDP_PER_PORT_CTRL_COPY_TO_CPUf, &entry, copyMask);
    HAL_FIELD_SET(L2_LLDP_PER_PORT_CTRLm, L2_LLDP_PER_PORT_CTRL_DROPf, &entry, dropMask);
    
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_LLDP_PER_PORT_CTRLm, 0, sizeof(l2_lldp_per_port_ctrl_t), &entry), ret);
    
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_ctrlpkt_lldp_act_get(yt_unit_t unit, yt_port_t port, yt_ctrlpkt_l2_action_t *pAct_ctrl)
{
    l2_lldp_per_port_ctrl_t entry;
    yt_macid_t macid;
    uint32_t copyMask;
    uint32_t dropMask;
    cmm_err_t ret = CMM_ERR_OK;

    macid = CAL_YTP_TO_MAC(unit,port);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_LLDP_PER_PORT_CTRLm, 0, sizeof(l2_lldp_per_port_ctrl_t), &entry), ret);
        
    HAL_FIELD_GET(L2_LLDP_PER_PORT_CTRLm, L2_LLDP_PER_PORT_CTRL_COPY_TO_CPUf, &entry, &copyMask);
    HAL_FIELD_GET(L2_LLDP_PER_PORT_CTRLm, L2_LLDP_PER_PORT_CTRL_DROPf, &entry, &dropMask);

    if (copyMask & (1UL<<macid))
    {
        if (dropMask & (1UL<<macid))
        {
            *pAct_ctrl = L2_ACTION_TRAP;
        }
        else
        {
            *pAct_ctrl = L2_ACTION_COPY;
        }
    }
    else
    {
        if (dropMask & (1UL<<macid))
        {
            *pAct_ctrl = L2_ACTION_DROP;
        }
        else
        {
            *pAct_ctrl = L2_ACTION_FWD;
        }
    }
   
    return CMM_ERR_OK;
}
