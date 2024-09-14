/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/

#ifndef __DRV_CTRLIF_H__
#define __DRV_CTRLIF_H__
#include "yt_types.h"
#include "cal_cmm.h"

#define SWITCH_ACCESOR_ON_UNIT(unit)   (UNITINFO(unit)->sw_access)
#define SWITCH_ACCESS_METHOD_ON_UNIT(unit)   (SWITCH_ACCESOR_ON_UNIT(unit).swreg_acc_method)
#define SWITCH_ACCESOR_CONTROLLER_ON_UNIT(unit)   (SWITCH_ACCESOR_ON_UNIT(unit).controller)
#define SWITCH_I2C_CONTROLLER_ON_UNIT(unit)   (SWITCH_ACCESOR_CONTROLLER_ON_UNIT(unit).i2c_controller)
#define SWITCH_SPI_CONTROLLER_ON_UNIT(unit)   (SWITCH_ACCESOR_CONTROLLER_ON_UNIT(unit).spi_controller)
#define UINT32_BYTES_NUM     (sizeof(uint32_t))

extern uint32_t ctrlif_reg_write(uint8_t unit, uint32_t regAddr, uint32_t regValue);
extern uint32_t ctrlif_reg_read(uint8_t unit, uint32_t regAddr, uint32_t *pRegValue);

int32_t word_to_bytes(uint32_t data, uint8_t *out, uint8_t out_len, bool is_addr, bool big_endian);
int32_t bytes_to_word(uint8_t *data, uint32_t len, uint32_t *reg_value, bool big_endian);
#endif
