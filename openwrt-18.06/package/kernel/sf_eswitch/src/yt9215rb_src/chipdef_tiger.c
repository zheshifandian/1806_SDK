/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/
#ifndef __CHIP_DEFINE_TIGER_C__
#define __CHIP_DEFINE_TIGER_C__

#include "chipdef.h"

/* chip capability */
/* chip 9001 related to yt9218 */
const yt_swchip_cap_t yt9218_capacity =
{
    .max_ucast_queue_num = 8,
    .max_mcast_queue_num = 4,
    .max_meter_entry_num = 64,
    .max_value_of_int_pri = 7,
    .max_value_of_int_drop = 2,
    .max_value_of_dscp = 63,
    .max_vlan_xlate_entry_num = 64,
    .max_vlan_egr_xlate_tbl_num = 32,
    .max_protocol_vlan_tbl_num = 4,
    .max_value_of_msti = 15,
    .max_vlan_range_profile_num = 10,
    .max_vlan_meter_entry_num = 32
};

/* chip 9002 related to yt9215 */
const yt_swchip_cap_t yt9215_capacity =
{
    .max_ucast_queue_num = 8,
    .max_mcast_queue_num = 4,
    .max_meter_entry_num = 64,
    .max_value_of_int_pri = 7,
    .max_value_of_int_drop = 2,
    .max_value_of_dscp = 63,
    .max_vlan_xlate_entry_num = 64,
    .max_vlan_egr_xlate_tbl_num = 32,
    .max_protocol_vlan_tbl_num = 0,
    .max_value_of_msti = 15,
    .max_vlan_range_profile_num = 10,
    .max_vlan_meter_entry_num = 32
};

const yt_swchip_if_info_t yt9218_intf_info =
{
	.allif_num = 11,
	.intif_num = 8,
	.intif_start_mac_id = 0,
	.extif_num = 2,
	.extif_start_mac_id = 8,
	.extif_start_id = 0,
	.intcpu_mac_id = 10
};

const yt_swchip_if_info_t yt9215_intf_info =
{
	.allif_num = 8,
	.intif_num = 5,
	.intif_start_mac_id = 0,
	.extif_num = 2,
	.extif_start_mac_id = 8,
	.extif_start_id = 0,
	.intcpu_mac_id = 10
};

#endif
