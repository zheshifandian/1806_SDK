/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/

#ifndef __FAL_INIT_H__
#define __FAL_INIT_H__

#include "yt_error.h"
#include "yt_types.h"
#include "chipdef.h"

typedef yt_ret_t (*yt_switch_chip_init_t)(yt_unit_t);

typedef struct yt_fal_init_info_s
{
    yt_switch_chip_id_t chip_id;
    yt_switch_chip_init_t chipFalInitFunc;
}yt_fal_init_info_t;


extern yt_ret_t fal_init(void);

#endif
