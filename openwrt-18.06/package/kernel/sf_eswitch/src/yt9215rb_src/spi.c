/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
#include "yt_error.h"
#include "yt_util.h"
#include "spi.h"
#include "ctrlif.h"

uint32_t spi_switch_write(uint8_t unit, uint32_t regAddr, uint32_t regValue)
{
    uint8_t addr[UINT32_BYTES_NUM];
    uint8_t value[UINT32_BYTES_NUM];
    int32_t ret;

    CMM_PARAM_CHK(NULL == SWITCH_SPI_CONTROLLER_ON_UNIT(unit).spi_write, CMM_ERR_NOT_INIT);

    word_to_bytes(regAddr, addr, sizeof(addr), 1, 1);
    word_to_bytes(regValue, value, sizeof(value), 0, 1);
    CMM_ERR_CHK(SWITCH_SPI_CONTROLLER_ON_UNIT(unit).spi_write(addr, sizeof(addr), value, sizeof(value)), ret);

    return CMM_ERR_OK;
}

uint32_t spi_switch_read(uint8_t unit, uint32_t regAddr, uint32_t *pRegValue)
{
    uint8_t addr[UINT32_BYTES_NUM] = {0};
    uint8_t value[UINT32_BYTES_NUM] = {0};
    int32_t ret;

    CMM_PARAM_CHK(NULL == pRegValue, CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK(NULL == SWITCH_SPI_CONTROLLER_ON_UNIT(unit).spi_read, CMM_ERR_NOT_INIT);

    word_to_bytes(regAddr, addr, sizeof(addr), 1, 1);
    CMM_ERR_CHK(SWITCH_SPI_CONTROLLER_ON_UNIT(unit).spi_read(addr, sizeof(addr), value, sizeof(value)), ret);
    CMM_ERR_CHK(bytes_to_word(value, sizeof(value), pRegValue, 1), ret);

    return CMM_ERR_OK;
}
