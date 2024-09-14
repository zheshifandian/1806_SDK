/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/

#include "hal_ctrl.h"
#include "cal_cmm.h"
#include "yt_error.h"

yt_hal_ctrl_t gHalCtrl[YT_UNIT_NUM];

uint32_t hal_ctrl_init(void)
{
    yt_unit_t unit;
    yt_port_t port_id;
    yt_macid_t mac_id;
    yt_switch_chip_model_t swModel;
    yt_phy_chip_model_t phyModel;

    for(unit = 0; unit < YT_UNIT_NUM; unit++)
    {
        swModel = CAL_SWCHIP_MODEL(unit);
        if(swModel >= YT_SW_MODEL_END)
        {
            return CMM_ERR_FAIL;
        }
        gHalCtrl[unit].pHalSwDrv = gpSwDrvList[swModel];

        for(port_id = 0; port_id < CAL_PORT_NUM_ON_UNIT(unit); port_id++)
        {
            if(CAL_IS_SERDES(unit, port_id))
            {
                mac_id = CAL_YTP_TO_MAC(unit, port_id);
                gHalCtrl[unit].pHalPhyDrv[mac_id] = gpPhyDrvList[YT_PHY_MODEL_INTSERDES];
            }
            else if(CAL_IS_PHY_PORT(unit, port_id))
            {
                phyModel = CAL_PHYCHIP_MODEL(unit, port_id);
                if(phyModel >= YT_PHY_MODEL_END)
                {
                    return CMM_ERR_FAIL;
                }
                mac_id = CAL_YTP_TO_MAC(unit, port_id);
                gHalCtrl[unit].pHalPhyDrv[mac_id] = gpPhyDrvList[phyModel];
            }
        }
    }

    return CMM_ERR_OK;
}
