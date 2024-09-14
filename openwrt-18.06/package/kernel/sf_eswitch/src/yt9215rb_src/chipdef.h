/*******************************************************************************
*                                                                              *
*  Copyright (c), 2022, Motorcomm Electronic Technology Co.,Ltd.               *
*  Motorcomm Confidential and Proprietary.                                     *
*                                                                              *
********************************************************************************
*/

#ifndef __CHIP_DEFINE_H__
#define __CHIP_DEFINE_H__

#include "yt_types.h"
#if defined(SWITCH_SERIES_TIGER)
#include "chipdef_tiger.h"
#endif

#define INVALID_ID  (0xFF)

typedef enum yt_switch_chip_model_e
{
	YT_SW_MODEL_9215 = 0,
	YT_SW_MODEL_9218,
	YT_SW_MODEL_END
}yt_switch_chip_model_t;

typedef enum yt_switch_chip_id_e
{
    YT_SW_ID_9215	= 0x9002,
    YT_SW_ID_9218	= 0x9001,
}yt_switch_chip_id_t;

typedef enum yt_switch_chip_rev_e
{
    YT_SW_REV_A	 = 1,
    YT_SW_REV_B,
    YT_SW_REV_C,
    YT_SW_REV_D
}yt_switch_chip_rev_t;

typedef enum yt_switch_device_id_e
{
    YT_TIGER_DEVICE_FPGA	= 0x9001,
    YT_TIGER_DEVICE_ASIC	= 0x9002,
}yt_switch_device_id_t;

/* capability of switch unit */
typedef struct yt_sw_chip_cap_s
{
    uint8_t max_ucast_queue_num;
    uint8_t max_mcast_queue_num;
    uint8_t max_meter_entry_num;
    uint8_t max_value_of_int_pri;
    uint8_t max_value_of_int_drop;
    uint8_t max_value_of_dscp;
    uint8_t max_vlan_xlate_entry_num;
    uint8_t max_vlan_egr_xlate_tbl_num;
    uint8_t max_protocol_vlan_tbl_num;
    uint8_t max_value_of_msti;
    uint8_t max_vlan_range_profile_num;
    uint8_t max_vlan_meter_entry_num;
}yt_swchip_cap_t;

/* chip interface define */
typedef struct yt_sw_chip_if_info_s
{
	uint8_t allif_num;
	uint8_t intif_num;
	uint8_t intif_start_mac_id;
	uint8_t extif_num;
	uint8_t extif_start_mac_id;
	uint8_t extif_start_id;
	uint8_t intcpu_mac_id;
}yt_swchip_if_info_t;

/* data define */
#if defined(SWITCH_SERIES_TIGER)
extern const yt_swchip_cap_t yt9218_capacity;
extern const yt_swchip_cap_t yt9215_capacity;
extern const yt_swchip_if_info_t yt9218_intf_info;
extern const yt_swchip_if_info_t yt9215_intf_info;
#endif

extern const yt_swchip_cap_t *gpChipCapList[];
extern const yt_swchip_if_info_t *gpChipIntfInfoList[];

/* function define */
extern uint8_t chipdef_get_extif_by_macid(yt_macid_t mac_id, const yt_swchip_if_info_t *pChipIfInfo);

#endif /* __CHIP_DEFINE_H__ */
