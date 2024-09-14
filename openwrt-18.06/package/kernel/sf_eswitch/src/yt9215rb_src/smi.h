/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
#ifndef __CTRLIF_SMI_H__
#define __CTRLIF_SMI_H__
#include "yt_types.h"

uint32_t smi_switch_write(uint8_t unit, uint32_t reg_addr, uint32_t reg_value);
uint32_t smi_switch_read(uint8_t unit, uint32_t reg_addr, uint32_t *reg_value);
#endif
