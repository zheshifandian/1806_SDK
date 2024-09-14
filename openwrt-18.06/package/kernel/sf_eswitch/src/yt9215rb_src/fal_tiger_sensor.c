/******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
*******************************************************************************
*/

#include "yt_error.h"
#include "hal_mem.h"
#include "fal_tiger_sensor.h"

yt_ret_t fal_tiger_sensor_temp_enable_set(yt_unit_t unit, yt_enable_t enable)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t regData;

    CMM_ERR_CHK(HAL_MEM_DIRECT_READ(unit, 0x8036c, &regData), ret);
    if(enable == YT_ENABLE)
    {
        regData |= (1<<18);
    }
    else
    {
        regData &= ~(1<<18);
    }

    CMM_ERR_CHK(HAL_MEM_DIRECT_WRITE(unit, 0x8036c, regData), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_sensor_temp_enable_get(yt_unit_t unit, yt_enable_t *pEnable)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t regData;

    CMM_ERR_CHK(HAL_MEM_DIRECT_READ(unit, 0x8036c, &regData), ret);
    *pEnable = (regData & (1<<18)) ? YT_ENABLE : YT_DISABLE;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_sensor_temp_value_get(yt_unit_t unit, yt_bool_t *pIsNegative, uint16_t *pAbsoluteValue)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t regData;

    CMM_ERR_CHK(HAL_MEM_DIRECT_READ(unit, 0x80374, &regData), ret);
    if(regData&0x8000)
    {
        *pIsNegative = TRUE;
        *pAbsoluteValue = (uint16_t)(51200 - (regData&0x0ffff)*100/128);
    }
    else
    {
        *pIsNegative = FALSE;
        *pAbsoluteValue = (uint16_t)((regData&0x0ffff)*100/128);
    }

    return CMM_ERR_OK;
}
