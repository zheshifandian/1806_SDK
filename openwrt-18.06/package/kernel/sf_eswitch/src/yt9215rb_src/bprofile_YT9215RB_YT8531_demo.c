/******************************************************************************
*                                                                             *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.              *
*  Motorcomm Confidential and Proprietary.                                    *
*                                                                             *
*******************************************************************************
*  File Name     : bprofile_YT9215RB_YT8531_demo.c
*  Version       : Initial Draft
*  Created       : 
*  Last Modified :
*  Description   : board profile for yt9215rb board,port(5+2 ext phy with yt8531)
*
******************************************************************************/

/**************************************************
 *      Include header files                       *
 **************************************************/
#include "cal_bprofile.h"
#include "yt_error.h"
/**************************************************
 *      Constants or macros Declaration            *
 **************************************************/

/**************************************************
 *      Global variables                           *
 **************************************************/

/**************************************************
 *      Functions Declaration                      *
 **************************************************/
yt_ret_t profile_yt9215rb_yt8531_init(yt_hwProfile_info_t *hwprofile_info);

yt_swDescp_t yt9215rb_yt8531_swDescp;

const board_profile_identify_t YT9215rbYT8531ProfileIdentifier = {BOARD_ID_YT9215RB, "YT9215RB YT8531 Demo"};

/* port descp */
const yt_portDescp_t YT9215rbYT8531PortDescp[] =
{
    /*macid	attribute		phy_index	phy_addr	serdes_index	ethtype			medium				smi */
    {0,		PORT_ATTR_ETH,	0,			0,			INVALID_ID,		ETH_TYPE_GE,	PORT_MEDI_COPPER,	YT_SMI_INT},
    {1,		PORT_ATTR_ETH,	0,			1,			INVALID_ID,		ETH_TYPE_GE,	PORT_MEDI_COPPER,	YT_SMI_INT},
    {2,		PORT_ATTR_ETH,	0,			2,			INVALID_ID,		ETH_TYPE_GE,	PORT_MEDI_COPPER,	YT_SMI_INT},
    {3,		PORT_ATTR_ETH,	0,			3,			INVALID_ID,		ETH_TYPE_GE,	PORT_MEDI_COPPER,	YT_SMI_INT},
    {4,		PORT_ATTR_ETH,	0,			4,			INVALID_ID,		ETH_TYPE_GE,	PORT_MEDI_COPPER,	YT_SMI_INT},
    {8,		PORT_ATTR_ETH,	1,			1,			INVALID_ID,		ETH_TYPE_GE,	PORT_MEDI_COPPER,	YT_SMI_EXT},
    {9,		PORT_ATTR_ETH,	2,			2,			INVALID_ID,		ETH_TYPE_GE,	PORT_MEDI_COPPER,	YT_SMI_EXT},
#ifdef INTER_MCU
    /* internel cpu port */
    {10,    PORT_ATTR_INT_CPU,			    INVALID_ID,	INVALID_ID,	PORT_ATTR_ETH,	ETH_TYPE_GE,	PORT_MEDI_COPPER,	INVALID_ID},
#endif
    {INVALID_ID,INVALID_ID,INVALID_ID,INVALID_ID,INVALID_ID,INVALID_ID,INVALID_ID,YT_SMI_NONE},
};

/* phy.descp */
const yt_phyDescp_t YT9215rbYT8531PhyDescp[] =
{
	/*chip_id chip_model				start_mac_id	phy_max*/
	{0,		  YT_PHY_MODEL_INT861X,	0,				5},
	{1,		  YT_PHY_MODEL_8531,		8,				1},
	{2,		  YT_PHY_MODEL_8531,		9,				1},
	{INVALID_ID,INVALID_ID,INVALID_ID,INVALID_ID},
};

/* LED description */
const yt_sled_remapInfo_t YT9215rbYT8531RemapInfo[] = 
{
    {6, 1}, {4, 0}, {5, 1}, {3, 0}, {2, 0}, {1, 0}, {0, 0},
    {6, 0}, {5, 0}, {4, 1}, {3, 1}, {2, 1}, {1, 1}, {0, 1},
    {6, 2}, {5, 2}, {4, 2}, {3, 2}, {2, 2}, {1, 2}, {0, 2}
};

yt_sled_param_t YT9215rbYT8531SLEDParam = {
    LED_SERIAL_ACTIVE_MODE_LOW,
    SLED_DATANUM_21, 
    0,
    YT9215rbYT8531RemapInfo
};

const yt_ledDescp_t YT9215rbYT8531LEDDescp = {LED_MODE_SERIAL, &YT9215rbYT8531SLEDParam};

/* hardware profile */
const yt_hwProfile_t yt9215rb_yt8531_demo =
{
    .pIdentifier = &YT9215rbYT8531ProfileIdentifier,
    .profile_init = profile_yt9215rb_yt8531_init,
};

yt_ret_t profile_yt9215rb_yt8531_init(yt_hwProfile_info_t *hwprofile_info)
{
    uint8_t	i;

    hwprofile_info->pIdentifier = &YT9215rbYT8531ProfileIdentifier;

    /* switch info */
    hwprofile_info->switch_count = 1;

    yt9215rb_yt8531_swDescp.chip_id = YT_SW_ID_9215;
    yt9215rb_yt8531_swDescp.chip_model = YT_SW_MODEL_9215;

    i = 0;
    while(YT9215rbYT8531PortDescp[i].mac_id != INVALID_ID)
    {
        yt9215rb_yt8531_swDescp.pPortDescp[i] = &YT9215rbYT8531PortDescp[i];
        i++;
    }
    yt9215rb_yt8531_swDescp.port_num = i;

    i = 0;
    while(YT9215rbYT8531PhyDescp[i].phy_index!= INVALID_ID)
    {
        yt9215rb_yt8531_swDescp.pPhyDescp[i] = &YT9215rbYT8531PhyDescp[i];
        i++;
    }

    YT9215rbYT8531SLEDParam.remapInfoNum = sizeof(YT9215rbYT8531RemapInfo)/sizeof(yt_sled_remapInfo_t);
    yt9215rb_yt8531_swDescp.pLEDDescp = &YT9215rbYT8531LEDDescp;
    
    hwprofile_info->pSwDescp[0] = &yt9215rb_yt8531_swDescp;

    return CMM_ERR_OK;
}
