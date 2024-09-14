/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
#include "yt_error.h"
#include "yt_types.h"
#include "yt_cmm.h"
#include "fal_init.h"

#ifdef SWITCH_SERIES_TIGER
#include "fal_tiger_init.h"
#endif

yt_fal_init_info_t gFal_init_info[] =
{
#ifdef SWITCH_SERIES_TIGER
    {YT_SW_ID_9215, fal_tiger_init},
#endif
};

yt_ret_t fal_init(void)
{
    yt_unit_t unit;
    uint8_t num = sizeof(gFal_init_info) /sizeof(yt_fal_init_info_t);
    uint8_t i;

    for(unit = 0; unit < YT_UNIT_NUM; unit++)
    {
        for(i = 0; num > i; i++)
        {
            if(gFal_init_info[i].chip_id == UNITINFO(unit)->chip_id)
            {
                gFal_init_info[i].chipFalInitFunc(unit);
            }
        }
    }

    return CMM_ERR_OK;
}
