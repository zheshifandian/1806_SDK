#include "yt_error.h"
#include "cal_mgm.h"
#include "cal_cmm.h"

uint32_t cal_ytportmask_isvalid(yt_unit_t unit, yt_port_mask_t port_mask, yt_bool_t incCpuPort)
{
    uint8_t i = 0;

    CMM_PARAM_CHK(!gcal_inited, CMM_ERR_FAIL);

    for(i = 0; i < YT_PORTS_WORD_NUM; i++)
    {
        if (TRUE == incCpuPort)
        {
            if(port_mask.portbits[i] & (~gBoardInfo.allportmask[unit].portbits[i]))
            {
                return CMM_ERR_FAIL;
            }
        }
        else
        {
            if(port_mask.portbits[i] & (~gBoardInfo.portmask[unit].portbits[i]))
            {
                return CMM_ERR_FAIL;
            }
        }
    }

    return CMM_ERR_OK;
}

/* add or remove port to yt port mask, for cpu port set */
uint32_t cal_ytportmask_update(yt_unit_t unit, yt_port_t port, yt_bool_t flag)
{
    if(TRUE == flag)
    {
        CMM_SET_MEMBER_PORT(gBoardInfo.portmask[unit], port);
    }
    else
    {
        CMM_RESET_MEMBER_PORT(gBoardInfo.portmask[unit], port);
    }
    
    return CMM_ERR_OK;
}

yt_phy_chip_model_t cal_phy_model_get(yt_unit_t unit, yt_port_t port)
{
    uint8_t phy_index;

    if(!CMM_PORT_VALID(unit, port))
    {
        return INVALID_ID;
    }

    phy_index = UNITINFO(unit)->pPortDescp[port]->phy_index;
    if(phy_index != INVALID_ID)
    {
        return UNITINFO(unit)->pPhyDescp[phy_index]->chip_model;
    }

    return INVALID_ID;
}

yt_serdes_mode_t cal_serdes_mode_get(yt_unit_t unit, yt_port_t port)
{
    uint8_t sds_id;

    if(!CMM_PORT_VALID(unit, port))
    {
        return INVALID_ID;
    }

    sds_id = UNITINFO(unit)->pPortDescp[port]->serdes_index;
    if(sds_id != INVALID_ID)
    {
        return UNITINFO(unit)->pSerdesDescp[sds_id]->mode;
    }

    return INVALID_ID;
}

yt_bool_t cal_is_combo_port(yt_unit_t unit, yt_port_t port)
{
    yt_port_medium_t media;

    if(!CMM_PORT_VALID(unit, port))
    {
        return INVALID_ID;
    }

    media = UNITINFO(unit)->pPortDescp[port]->medium;

    if(PORT_MEDI_COMBO_FIBER == media ||
        PORT_MEDI_COMBO_COPPER == media)
    {
        return TRUE;
    }

    return FALSE;
}
