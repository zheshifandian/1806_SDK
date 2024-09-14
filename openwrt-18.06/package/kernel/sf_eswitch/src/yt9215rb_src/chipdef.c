/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/

#include "chipdef.h"


/* data define */
const yt_swchip_cap_t *gpChipCapList[] =
{
	[YT_SW_MODEL_9215] = &yt9215_capacity,
	[YT_SW_MODEL_9218] = &yt9218_capacity,
};

const yt_swchip_if_info_t *gpChipIntfInfoList[] =
{
	[YT_SW_MODEL_9215] = &yt9215_intf_info,
	[YT_SW_MODEL_9218] = &yt9218_intf_info,
};

/* extif id = extif start id+extif_start_mac-mac */
uint8_t chipdef_get_extif_by_macid(yt_macid_t mac_id, const yt_swchip_if_info_t *pChipIfInfo)
{
	uint8_t extif_id = INVALID_ID;

	if(mac_id < pChipIfInfo->extif_start_mac_id ||
		mac_id > pChipIfInfo->extif_start_mac_id + pChipIfInfo->extif_num)
	{
		return extif_id;
	}

	extif_id = pChipIfInfo->extif_start_id + mac_id - pChipIfInfo->extif_start_mac_id;

	return extif_id;
}
