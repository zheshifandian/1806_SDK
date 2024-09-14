/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
#include "drv.h"
#ifdef ACC_UART
#include "uart.h"
#endif
#include "yt_error.h"
#include "yt_lock.h"
#include "yt_util.h"

uint32_t drv_init(uint8_t *dev)
{
    cmm_err_t ret = CMM_ERR_OK;

    ACCESS_LOCK_INIT();
    
#ifdef ACC_UART
    ret = uart_init(dev);
#else
    CMM_UNUSED_PARAM(dev);
#endif

    return ret;
}

uint32_t drv_close(void)
{    
#ifdef ACC_UART
    uart_exit();
#endif

    return CMM_ERR_OK;
}
