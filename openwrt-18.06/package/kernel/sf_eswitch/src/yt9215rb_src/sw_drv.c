/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/

#include "sw_drv.h"
#ifdef SWITCH_SERIES_TIGER
#include "sw_yt9215.h"
#endif

yt_switch_drv_t *gpSwDrvList[] =
{
#ifdef SWITCH_SERIES_TIGER
    [YT_SW_MODEL_9215] = &yt9215_drv,
#endif
    [YT_SW_MODEL_9218] = NULL,
};
