/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
#include "cal_bprofile.h"
#include "yt_error.h"
#include "cal_cmm.h"

#if defined(BOARD_YT9215RB_DEFAULT_DEMO) || defined(BOARD_AUTO_DETECT)
#include "bprofile_YT9215RB_default_demo.c"
#endif
#if defined(BOARD_YT9215RB_YT8531_DEMO) || defined(BOARD_AUTO_DETECT)
#include "bprofile_YT9215RB_YT8531_demo.c"
#endif
#if defined(BOARD_YT9215S_YT8531_FIB_DEMO) || defined(BOARD_AUTO_DETECT)
#include "bprofile_YT9215S_YT8531_Fib_demo.c"
#endif
#if defined(BOARD_YT9215S_FIB_DEMO) || defined(BOARD_AUTO_DETECT)
#include "bprofile_YT9215S_Fib_demo.c"
#endif
#if defined(BOARD_YT9215SC_DEFAULT_DEMO) || defined(BOARD_AUTO_DETECT)
#include "bprofile_YT9215SC_default_demo.c"
#endif

/*
 * Data Type Definition
 */

yt_hwProfile_info_t	gBoardInfo;
const yt_swDescp_t	*gpSwitchUnit[YT_MAX_UNIT];

/* list of hardware profiles */
const yt_hwProfile_t *gSupport_hwProfile_list[] =
{
#if defined(BOARD_YT9215RB_DEFAULT_DEMO) || defined(BOARD_AUTO_DETECT)
    &yt9215rb_default_demo,
#endif
#if defined(BOARD_YT9215RB_YT8531_DEMO) || defined(BOARD_AUTO_DETECT)
    &yt9215rb_yt8531_demo,
#endif
#if defined(BOARD_YT9215S_YT8531_FIB_DEMO) || defined(BOARD_AUTO_DETECT)
    &yt9215s_yt8531_fib_demo,
#endif
#if defined(BOARD_YT9215S_FIB_DEMO) || defined(BOARD_AUTO_DETECT)
    &yt9215s_fib_demo,
#endif
#if defined(BOARD_YT9215SC_DEFAULT_DEMO) || defined(BOARD_AUTO_DETECT)
    &yt9215sc_default_demo,
#endif
    NULL
};

/*
 * Function Declaration
 */

/*****************************************************************************
*   Prototype    : cal_board_profile_init
*   Description  : init board profile info
*   Input        : void
*   Output       : None
*   Return Value : uint32_t
*
*****************************************************************************/
uint32_t cal_board_profile_init(void)
{
    const yt_hwProfile_t     **pbProfile = gSupport_hwProfile_list;
    uint8_t	i;
    uint8_t j = 0;

    while (*pbProfile != NULL)
    {
        /*if(cal_board_identifier_cmp(gUSERDEFINE, *pbProfile->pIdentifier))*/
        {
            if ((*pbProfile)->profile_init)
            {
                (*pbProfile)->profile_init(&gBoardInfo);
            }

            for (i=0; i<gBoardInfo.switch_count; i++)
            {
                gpSwitchUnit[i] = gBoardInfo.pSwDescp[i];

                for(j = 0; j < gBoardInfo.pSwDescp[i]->port_num; j++)
                {
                    if(gBoardInfo.pSwDescp[i]->pPortDescp[j] &&
                        gBoardInfo.pSwDescp[i]->pPortDescp[j]->mac_id != INVALID_ID)
                    {
                        CMM_SET_MEMBER_PORT(gBoardInfo.portmask[i], j);
                        CMM_SET_MEMBER_PORT(gBoardInfo.allportmask[i], j);
                        CMM_SET_MEMBER_PORT(gBoardInfo.macmask[i],
                            gBoardInfo.pSwDescp[i]->pPortDescp[j]->mac_id);
                    }
                }

                gBoardInfo.pSwDescp[i]->pChipCap = gpChipCapList[gBoardInfo.pSwDescp[i]->chip_model];
                gBoardInfo.pSwDescp[i]->pChipIfInfo = gpChipIntfInfoList[gBoardInfo.pSwDescp[i]->chip_model];
            }

            return CMM_ERR_OK;
        }
        pbProfile++;
    }

    return CMM_ERR_FAIL;
}

/* TODO: select default board profile according to chip id and model, if did not spcific */
