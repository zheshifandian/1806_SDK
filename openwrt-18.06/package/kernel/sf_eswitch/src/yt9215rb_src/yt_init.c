/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
/*
 * Include Files
 */
#include "yt_error.h"
#include "yt_types.h"
#include "yt_init.h"
#include "yt_vlan.h"
#include "cal_mgm.h"
#include "fal_dispatch.h"
#include "hal_cmm.h"
#include "drv.h"
#include "fal_init.h"

yt_ret_t  yt_basic_init(void)
{
    cmm_err_t ret = CMM_ERR_OK;
    CMM_ERR_CHK(cal_mgm_init(), ret);
    CMM_ERR_CHK(hal_init(), ret);
    CMM_ERR_CHK(fal_dispatch_init(), ret);
    CMM_ERR_CHK(yt_drv_init(), ret);
    CMM_ERR_CHK(fal_init(), ret);

    return CMM_ERR_OK;
}

yt_ret_t  yt_device_init(yt_unit_t unit, uint8_t *dev)
{
    cmm_err_t ret = CMM_ERR_OK;
    CMM_UNUSED_PARAM(unit);
    CMM_ERR_CHK(drv_init(dev),ret);
    return CMM_ERR_OK;
}

yt_ret_t  yt_device_close(yt_unit_t unit)
{
    cmm_err_t ret = CMM_ERR_OK;
    CMM_UNUSED_PARAM(unit);
    CMM_ERR_CHK(drv_close(),ret);
    return CMM_ERR_OK;
}

yt_ret_t yt_modules_init(void)
{
    yt_unit_t unit;

    for(unit = 0; unit < YT_UNIT_NUM; unit++)
    {
#ifdef L2_INCLUDED
        yt_l2_init(unit);
#endif
#ifdef PORT_INCLUDED
        yt_port_init(unit);
#endif
#ifdef VLAN_INCLUDED
        yt_vlan_init(unit);
#endif
#ifdef RATE_INCLUDED
        yt_rate_init(unit);
#endif
#ifdef STORM_CTRL_INCLUDED
        yt_storm_ctrl_init(unit);
#endif
#ifdef ACL_INCLUDED
        yt_acl_init(unit);
#endif
#ifdef DOT1X_INCLUDED
        yt_dot1x_init(unit);
#endif
#ifdef STAT_INCLUDED
        yt_stat_mib_init(unit);
#endif
#ifdef NIC_INCLUDED
        yt_nic_init(unit);
#endif
#ifdef DOS_INCLUDED
        yt_dos_init(unit);
#endif
    }

    return CMM_ERR_OK;
}

yt_ret_t  yt_init(void)
{
    cmm_err_t ret = CMM_ERR_OK;

    CMM_ERR_CHK(yt_basic_init(),ret);

    CMM_ERR_CHK(yt_modules_init(),ret);

    return CMM_ERR_OK;
}

yt_ret_t  yt_drv_init(void)
{
    yt_unit_t unit;

    for(unit = 0; unit < YT_UNIT_NUM; unit++)
    {
#ifdef ACC_UART
        yt_device_init(unit, "ttyS2");
#else
        yt_device_init(unit, NULL);
#endif
    }

    return CMM_ERR_OK;
}
