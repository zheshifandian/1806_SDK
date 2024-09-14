/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
#ifndef __PHY_DRV_H__
#define __PHY_DRV_H__

#include "phy_chipdef.h"
#include "yt_port.h"
#include "yt_error.h"

typedef struct yt_phy_drv_func_s
{
    uint8_t phy_num; /* max interface num */
    yt_ret_t (*phy_init)(yt_unit_t);
    yt_ret_t (*phy_restart)(yt_unit_t, uint8_t);
    yt_ret_t (*phy_enable_set)(yt_unit_t, uint8_t, yt_enable_t);
    yt_ret_t (*phy_enable_get)(yt_unit_t, uint8_t, yt_enable_t *);
    yt_ret_t (*phy_medium_set)(yt_unit_t, uint8_t, yt_port_medium_t);
    yt_ret_t (*phy_medium_get)(yt_unit_t, uint8_t, yt_port_medium_t *);
    yt_ret_t (*phy_autoNeg_enable_set)(yt_unit_t, uint8_t, yt_enable_t);
    yt_ret_t (*phy_autoNeg_enable_get)(yt_unit_t, uint8_t, yt_enable_t *);
    yt_ret_t (*phy_autoNeg_ability_set)(yt_unit_t, uint8_t, yt_port_an_ability_t);
    yt_ret_t (*phy_autoNeg_ability_get)(yt_unit_t, uint8_t, yt_port_an_ability_t *);
    yt_ret_t (*phy_force_speed_duplex_set)(yt_unit_t, uint8_t, yt_port_speed_duplex_t);
    yt_ret_t (*phy_force_speed_duplex_get)(yt_unit_t, uint8_t, yt_port_speed_duplex_t *);
    yt_ret_t (*phy_link_status_get)(yt_unit_t, uint8_t, yt_port_linkStatus_all_t *);
    yt_ret_t (*phy_eee_enable_set)(yt_unit_t, uint8_t, yt_enable_t);
    yt_ret_t (*phy_eee_enable_get)(yt_unit_t, uint8_t, yt_enable_t *);
    yt_ret_t (*phy_combo_mode_set)(yt_unit_t, uint8_t, yt_combo_mode_t);
    yt_ret_t (*phy_combo_mode_get)(yt_unit_t, uint8_t, yt_combo_mode_t *);
    yt_ret_t (*phy_cable_diag)(yt_unit_t, uint8_t, yt_port_cableDiag_t *);
    yt_ret_t (*phy_interrupt_status_get)(yt_unit_t, uint8_t, uint16_t *);
    yt_ret_t (*phy_test_template)(yt_unit_t, uint8_t, yt_utp_template_testmode_t);
}yt_phy_drv_func_t;

typedef struct yt_phy_drv_s
{
    yt_phy_chip_id_t    chip_id;
    yt_phy_chip_model_t chip_model;
    yt_phy_drv_func_t   *pDrvFunc;
}yt_phy_drv_t;

extern yt_phy_drv_t *gpPhyDrvList[];

extern yt_ret_t int_phy_mmd_reg_read(yt_unit_t unit, uint8_t phy_addr, uint8_t mmd_id, uint16_t regAddr, uint16_t *pData);
extern yt_ret_t int_phy_mmd_reg_write(yt_unit_t unit, uint8_t phy_addr, uint8_t mmd_id, uint16_t regAddr, uint16_t data);
extern yt_ret_t int_phy_ext_reg_read(yt_unit_t unit, uint8_t phy_addr, uint16_t regAddr, uint16_t *pData);
extern yt_ret_t int_phy_ext_reg_write(yt_unit_t unit, uint8_t phy_addr, uint16_t regAddr, uint16_t data);
extern yt_ret_t ext_phy_mmd_reg_read(yt_unit_t unit, uint8_t phy_addr, uint8_t mmd_id, uint16_t regAddr, uint16_t *pData);
extern yt_ret_t ext_phy_mmd_reg_write(yt_unit_t unit, uint8_t phy_addr, uint8_t mmd_id, uint16_t regAddr, uint16_t data);
extern yt_ret_t ext_phy_ext_reg_read(yt_unit_t unit, uint8_t phy_addr, uint16_t regAddr, uint16_t *pData);
extern yt_ret_t ext_phy_ext_reg_write(yt_unit_t unit, uint8_t phy_addr, uint16_t regAddr, uint16_t data);

#endif
