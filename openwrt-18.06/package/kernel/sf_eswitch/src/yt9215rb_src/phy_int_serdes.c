/*******************************************************************************
*                                                                              *
*  Copyright (c), 2023, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
#include "phy_int_serdes.h"
#include "hal_ctrl.h"
#include "yt_error.h"
#include "yt_util.h"

yt_phy_drv_func_t int_serdes_drv_func =
{
    .phy_num = 2,
    .phy_init = int_serdes_init,
    .phy_restart = int_serdes_restart,
    .phy_enable_set = int_serdes_enable_set,
    .phy_enable_get = int_serdes_enable_get,
    .phy_medium_set = int_serdes_medium_set,
    .phy_medium_get = int_serdes_medium_get,
    .phy_autoNeg_enable_set = int_serdes_autoNeg_enable_set,
    .phy_autoNeg_enable_get = int_serdes_autoNeg_enable_get,
    .phy_autoNeg_ability_set = int_serdes_autoNeg_ability_set,
    .phy_autoNeg_ability_get = int_serdes_autoNeg_ability_get,
    .phy_force_speed_duplex_set = int_serdes_force_speed_duplex_set,
    .phy_force_speed_duplex_get = int_serdes_force_speed_duplex_get,
    .phy_link_status_get = int_serdes_link_status_get,
    .phy_eee_enable_set = int_serdes_eee_enable_set,
    .phy_eee_enable_get = int_serdes_eee_enable_get,
    .phy_combo_mode_set = int_serdes_combo_mode_set,
    .phy_combo_mode_get = int_serdes_combo_mode_get,
    .phy_cable_diag = int_serdes_cable_diag,
    .phy_interrupt_status_get = int_serdes_interrupt_status_get,
    .phy_test_template = int_serdes_test_template,
};

yt_phy_drv_t int_serdes_drv =
{
    .chip_id = YT_PHY_ID_INTSERDES,
    .chip_model = YT_PHY_MODEL_INTSERDES,
    .pDrvFunc = &int_serdes_drv_func
};

yt_ret_t int_serdes_init(yt_unit_t unit)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t phyAddr;
    static yt_bool_t initFlag = 0;
    uint16_t phyData;
    yt_port_t port;

    if(initFlag)
    {
        return CMM_ERR_OK;
    }

    for(port = 0; port < CAL_PORT_NUM_ON_UNIT(unit); port++)
    {
        if (!CAL_IS_SERDES(unit, port))
        {
            continue;
        }

        phyAddr = CAL_YTP_TO_PHYADDR(unit, port);
        if (phyAddr == INVALID_ID)
        {
            continue;
        }
        CMM_ERR_CHK(int_phy_ext_reg_read(unit, phyAddr, 0xa0, &phyData), ret);
        if (IS_BIT_SET(phyData, 14))
        {
            CLEAR_BIT(phyData, 14);
            CMM_ERR_CHK(int_phy_ext_reg_write(unit, phyAddr, 0xa0, phyData), ret);
        }
    }
    initFlag = 1;

    return CMM_ERR_OK;
}

yt_ret_t int_serdes_restart(yt_unit_t unit, uint8_t phyAddr)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_enable_set(yt_unit_t unit, uint8_t phyAddr, yt_enable_t enable)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint16_t phyData = 0;

    CMM_ERR_CHK(HALSWDRV_FUNC(unit)->switch_intif_read(unit, phyAddr, PHY_BASE_CTRL_REG_0, &phyData), ret);
    if(YT_ENABLE == enable)
    {
        CLEAR_BIT(phyData, 11);
        SET_BIT(phyData, 9);
    }
    else
    {
        SET_BIT(phyData, 11);
    }
    CMM_ERR_CHK(HALSWDRV_FUNC(unit)->switch_intif_write(unit, phyAddr, PHY_BASE_CTRL_REG_0, phyData), ret);

    return CMM_ERR_OK;
}

yt_ret_t int_serdes_enable_get(yt_unit_t unit, uint8_t phyAddr, yt_enable_t *pEnable)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint16_t phyData = 0;

    CMM_ERR_CHK(HALSWDRV_FUNC(unit)->switch_intif_read(unit, phyAddr, PHY_BASE_CTRL_REG_0, &phyData), ret);
    *pEnable = IS_BIT_SET(phyData, 11) ? YT_DISABLE : YT_ENABLE;

    return CMM_ERR_OK;
}

yt_ret_t int_serdes_medium_set(yt_unit_t unit, uint8_t phyAddr, yt_port_medium_t medium)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(medium);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_medium_get(yt_unit_t unit, uint8_t phyAddr, yt_port_medium_t *pMedium)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pMedium);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_autoNeg_enable_set(yt_unit_t unit, uint8_t phyAddr, yt_enable_t enable)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(enable);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_autoNeg_enable_get(yt_unit_t unit, uint8_t phyAddr, yt_enable_t *pEnable)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pEnable);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_autoNeg_ability_set(yt_unit_t unit, uint8_t phyAddr, yt_port_an_ability_t ability)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(ability);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_autoNeg_ability_get(yt_unit_t unit, uint8_t phyAddr, yt_port_an_ability_t *pAbility)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pAbility);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_force_speed_duplex_set(yt_unit_t unit, uint8_t phyAddr, yt_port_speed_duplex_t speed_dup)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(speed_dup);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_force_speed_duplex_get(yt_unit_t unit, uint8_t phyAddr, yt_port_speed_duplex_t *pSpeedDup)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pSpeedDup);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_link_status_get(yt_unit_t unit, uint8_t phyAddr, yt_port_linkStatus_all_t *pLinkStatus)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pLinkStatus);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_eee_enable_set(yt_unit_t unit, uint8_t phyAddr, yt_enable_t enable)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(enable);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_eee_enable_get(yt_unit_t unit, uint8_t phyAddr, yt_enable_t *pEnable)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pEnable);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_combo_mode_set(yt_unit_t unit, uint8_t phyAddr, yt_combo_mode_t mode)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(mode);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_combo_mode_get(yt_unit_t unit, uint8_t phyAddr, yt_combo_mode_t *pMode)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pMode);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_cable_diag(yt_unit_t unit, uint8_t phyAddr, yt_port_cableDiag_t *pCableDiagStatus)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pCableDiagStatus);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_interrupt_status_get(yt_unit_t unit, uint8_t phyAddr, uint16_t *pStatusData)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pStatusData);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_test_template(yt_unit_t unit, uint8_t phyAddr, yt_utp_template_testmode_t mode)
{
    cmm_err_t ret = CMM_ERR_OK;

    if(mode < YT_UTP_TEMPLATE_TMODE_SDS2500M || mode > YT_UTP_TEMPLATE_TMODE_SDS1000M)
    {
        return CMM_ERR_INPUT;
    }

    switch(mode)
    {
        case YT_UTP_TEMPLATE_TMODE_SDS2500M:
        case YT_UTP_TEMPLATE_TMODE_SDS1000M:
            /*CMM_ERR_CHK(int_phy_ext_reg_write(unit, phyAddr, 0x5, 0xc100), ret);*/
            CMM_ERR_CHK(int_phy_ext_reg_write(unit, phyAddr, 0xa0, 0x8c00), ret);
            break;
        default:
            return CMM_ERR_INPUT;
    }

    return CMM_ERR_OK;
}
