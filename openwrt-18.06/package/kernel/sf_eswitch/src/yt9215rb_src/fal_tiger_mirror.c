/*
 * Include Files
 */
#include "yt_error.h"
#include "osal_mem.h"
#include "osal_print.h"
#include "fal_tiger_mirror.h"
#include "hal_mem.h"
#include "fal_tiger_struct.h"
#include "fal_tiger_entry.h"
#include "fal_tiger_mem.h"
#include "fal_tiger_qos.h"

/*
 * Symbol Definition
 */

/*
 * Macro Declaration
 */

/*
 * Data Declaration
 */

/*
 * Function Declaration
 */

static uint32_t fal_tiger_que_colorAware_enable_set(yt_unit_t unit, yt_macid_t macid, yt_enable_t enable)
{
    uint32_t regAddr;
    uint32_t regVal;
    uint32_t regVal2;
    uint32_t qid;
    cmm_err_t ret = CMM_ERR_OK;

    for (qid = 0; qid < CAL_MAX_UCAST_QUEUE_NUM(unit); qid++)
    {
        regAddr = QOS_FORCEAC_UCASTQUE_REG(unit, macid, qid);
        CMM_ERR_CHK(HAL_MEM_DIRECT_READ(unit, regAddr, &regVal2), ret);
        regAddr += 4;
        CMM_ERR_CHK(HAL_MEM_DIRECT_READ(unit, regAddr, &regVal), ret);
        if (enable)
        {
            SET_BIT(regVal, 27);
        }
        else
        {
            CLEAR_BIT(regVal, 27);
        }
        CMM_ERR_CHK(HAL_MEM_DIRECT_WRITE(unit, regAddr - 4, regVal2), ret);
        CMM_ERR_CHK(HAL_MEM_DIRECT_WRITE(unit, regAddr, regVal), ret);
    }

    for (qid = 0; qid < CAL_MAX_MCAST_QUEUE_NUM(unit); qid++)
    {
        regAddr = QOS_FORCEAC_MCASTQUE_REG(unit, macid, qid);
        CMM_ERR_CHK(HAL_MEM_DIRECT_READ(unit, regAddr, &regVal), ret);
        if (enable)
        {
            SET_BIT(regVal, 28);
        }
        else
        {
            CLEAR_BIT(regVal, 28);
        }
        CMM_ERR_CHK(HAL_MEM_DIRECT_WRITE(unit, regAddr, regVal), ret);
    }

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_mirror_port_set(yt_unit_t unit, yt_port_t target_port, yt_port_mask_t rx_portmask, yt_port_mask_t tx_portmask)
{
    mirror_ctrl_t mirror_ctrl;
    yt_macid_t macid;
    yt_macid_t orgMacid;
    yt_port_mask_t macmask;
    cmm_err_t ret = CMM_ERR_OK;

    macid = CAL_YTP_TO_MAC(unit,target_port);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, MIRROR_CTRLm, 0, sizeof(mirror_ctrl_t), &mirror_ctrl), ret);
    HAL_FIELD_GET(MIRROR_CTRLm, MIRROR_CTRL_MIRROR_PORTf, &mirror_ctrl, &orgMacid);

    if(orgMacid != macid)
    {
        fal_tiger_que_colorAware_enable_set(unit, orgMacid, YT_ENABLE);
    }

    if(rx_portmask.portbits[0] == 0 && tx_portmask.portbits[0] == 0)/*disable portmirror*/
    {
        fal_tiger_que_colorAware_enable_set(unit, macid, YT_ENABLE);
    }
    else
    {
        fal_tiger_que_colorAware_enable_set(unit, macid, YT_DISABLE);
    }

    HAL_FIELD_SET(MIRROR_CTRLm, MIRROR_CTRL_MIRROR_PORTf, &mirror_ctrl, macid);
    CAL_YTPLIST_TO_MLIST(unit,rx_portmask, macmask);
    HAL_FIELD_SET(MIRROR_CTRLm, MIRROR_CTRL_INGR_MIRROR_ENf, &mirror_ctrl, macmask.portbits[0]);
    CAL_YTPLIST_TO_MLIST(unit,tx_portmask, macmask);
    HAL_FIELD_SET(MIRROR_CTRLm, MIRROR_CTRL_EGR_MIRROR_ENf, &mirror_ctrl, macmask.portbits[0]);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, MIRROR_CTRLm, 0, sizeof(mirror_ctrl_t), &mirror_ctrl), ret);  
   
    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_mirror_port_get(yt_unit_t unit, yt_port_t *p_target_port, yt_port_mask_t *p_rx_portmask, yt_port_mask_t *p_tx_portmask)
{
    mirror_ctrl_t mirror_ctrl;
    yt_port_mask_t macmask;
    uint32_t macid;
    uint32_t ingr_mirror;
    uint32_t egr_mirror;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, MIRROR_CTRLm, 0, sizeof(mirror_ctrl_t), &mirror_ctrl), ret); 
    
    HAL_FIELD_GET(MIRROR_CTRLm, MIRROR_CTRL_MIRROR_PORTf, &mirror_ctrl, &macid);
    CAL_MAC_TO_YTP(unit, macid, (*p_target_port));

    CMM_CLEAR_MEMBER_PORT(macmask);
    
    HAL_FIELD_GET(MIRROR_CTRLm, MIRROR_CTRL_INGR_MIRROR_ENf, &mirror_ctrl, &ingr_mirror);
    macmask.portbits[0] = ingr_mirror;
    CAL_MLIST_TO_YTPLIST(unit, macmask, (*p_rx_portmask));
    
    HAL_FIELD_GET(MIRROR_CTRLm, MIRROR_CTRL_EGR_MIRROR_ENf, &mirror_ctrl, &egr_mirror);
    macmask.portbits[0] = egr_mirror;
    CAL_MLIST_TO_YTPLIST(unit, macmask, (*p_tx_portmask));
   
    return CMM_ERR_OK;
}
