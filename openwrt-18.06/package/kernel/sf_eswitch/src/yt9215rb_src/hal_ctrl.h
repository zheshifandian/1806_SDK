/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/

#ifndef __HAL_CTRL_H__
#define __HAL_CTRL_H__

#include "yt_types.h"
#include "yt_cmm.h"
#include "sw_drv.h"
#include "phy_drv.h"

typedef struct yt_hal_ctrl_s
{
    yt_switch_drv_t    *pHalSwDrv;
    yt_phy_drv_t    *pHalPhyDrv[YT_MAX_PORT_PER_UNIT];
}yt_hal_ctrl_t;

extern yt_hal_ctrl_t gHalCtrl[YT_UNIT_NUM];

#define HALCTRL(unit)   gHalCtrl[unit]
#define HALSWDRV(unit)  gHalCtrl[unit].pHalSwDrv
#define HALSWDRV_FUNC(unit)  gHalCtrl[unit].pHalSwDrv->pDrvFunc
#define HALPHYDRV(unit, macid)  gHalCtrl[unit].pHalPhyDrv[macid]
#define HALPHYDRV_FUNC(unit, macid)  gHalCtrl[unit].pHalPhyDrv[macid]->pDrvFunc

extern uint32_t hal_ctrl_init(void);

#endif
