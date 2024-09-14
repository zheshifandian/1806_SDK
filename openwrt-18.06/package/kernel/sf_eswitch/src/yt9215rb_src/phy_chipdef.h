/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
#ifndef __PHY_CHIP_DEFINE_H__
#define __PHY_CHIP_DEFINE_H__

#include "yt_types.h"

#define PHY_BASE_CTRL_REG_0		(0)
#define PHY_AUTONEG_REG_4		(4)
#define PHY_1000BASE_CTRL_REG_9	(9)
#define PHY_LINK_STATUS_REG_17	(0x11)

typedef enum yt_phy_chip_model_e
{
	YT_PHY_MODEL_INT861X = 0,
	YT_PHY_MODEL_8531,
    YT_PHY_MODEL_INTSERDES,
	YT_PHY_MODEL_END
}yt_phy_chip_model_t;

typedef enum yt_phy_chip_id_e
{
    YT_PHY_ID_INT861X	= 0x4F51E800,
    YT_PHY_ID_INTSERDES	= 0x4F51E801,
    YT_PHY_ID_8531	= 0x4F51E91B,
    YT_PHY_ID_8614	= 0x4F51E899,
    YT_PHY_ID_8618	= 0x4F51E889,
}yt_phy_chip_id_t;

typedef struct yt_phy_chip_cap_s
{
	uint8_t phy_num;
}yt_phy_chip_cap_t;

#endif
