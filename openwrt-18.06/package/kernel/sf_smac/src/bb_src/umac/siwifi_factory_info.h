/**
 ******************************************************************************
 *
 * @file siwifi_factory_info.h
 *
 * @brief siwifi driver use factory info func
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */

#ifndef _SIWIFI_FACTORY_INFO_H_
#define _SIWIFI_FACTORY_INFO_H_

#include "siwifi_defs.h"

#ifdef CONFIG_SFAX8_FACTORY_READ
#include <sfax8_factory_read.h>
#endif

/* 4(11b) + 8(11g) + 8(HT20 MCS 0-7) + 8(HT40 MCS 0-7) */
#define LB_ONE_CHAN_GAIN_NUM 28
/* 8(11a) + 8(HT20 MCS 0-7) + 8(HT40 MCS 0-7) + 9(VHT20 MCS 0-8) + 10(VHT40 MCS 0-9) + 10(VHT80 MCS 0-9) */
#define HB_ONE_CHAN_GAIN_NUM 53

#define LB_CHANNEL_COUNT 13 /* channel 1-13 */
#define HB_CHANNEL_COUNT 25 /* channel 36-64, 100-144, 149-165 */

#define LB_LIST_LEN (LB_ONE_CHAN_GAIN_NUM * LB_CHANNEL_COUNT)
#define HB_LIST_LEN (HB_ONE_CHAN_GAIN_NUM * HB_CHANNEL_COUNT)

#define LB_V2_MAGIC_INFO_SIZE      1  /* value is 24 */
#define HB_V2_MAGIC_INFO_SIZE      1  /* value is 5 */

#define LB_V2_MORE_INFO_SIZE       (LB_V2_MAGIC_INFO_SIZE + LB_ONE_CHAN_GAIN_NUM)
#define HB_V2_MORE_INFO_SIZE       (HB_V2_MAGIC_INFO_SIZE + HB_ONE_CHAN_GAIN_NUM)

#define SIWIFI_TXPOWER_SLEEPMODE_LOW_NAME     "txp_offset_sleepmode_low.ini"
#define SIWIFI_TXPOWER_SLEEPMODE_LOW_SECOND_NAME     "txp_offset_sleepmode_low_second.ini"

struct siwifi_factory_info {
    int version;
    // we set xo value in rf driver, so xo_value will not be used.
    int xo_value;
	char *sleepmode_txpower_list;
	char *low_txpower_list;
	char *normal_txpower_list;
    char *high_txpower_list;
#ifdef CFG_DUAL_ANTENNA_CALIBRATE
	char *sleepmode_txpower_list_second_antenna;
	char *low_txpower_list_second_antenna;
	char *normal_txpower_list_second_antenna;
#endif
    int list_len;
};

int sf_wifi_init_wifi_factory_info(struct siwifi_hw *siwifi_hw);
void sf_wifi_deinit_wifi_factory_info(struct siwifi_hw *siwifi_hw);

#endif /* _SIWIFI_FACTORY_INFO_H_ */
