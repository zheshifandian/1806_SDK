/**
 ******************************************************************************
 *
 * @file siwifi_vendor.h
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */

/**
 ******************************************************************************
 * ACS function
 *
 * step1:   Set the macro "CONFIG_DRIVER_NL80211_QCA" of hostapd-full.config to y firstly
 * (path:   openwrt-18.06/package/network/services/hostapd/files/hostapd-full.config)
 *
 * step2:   Set the macro "CONFIG_SIWIFI_ACS" of umac-config.mk to y
 * (path:   openwrt-18.06/package/kernel/sf_smac/src/bb_src/umac/umac_config.mk)
 *
 ******************************************************************************
 */

#ifndef _SIWIFI_VENDOR_H_
#define _SIWIFI_VENDOR_H_

#include "siwifi_defs.h"

#define SF_OUI	0x001374

enum siwifi_vendor_commands {
	SIWIFI_VENDOR_CMD_DO_ACS = 54, // see hostapd QCA_NL80211_VENDOR_SUBCMD_DO_ACS define

	NUM_SIWIFI_VENDOR_CMD,
	MAX_SIWIFI_VENDOR_CMD = NUM_SIWIFI_VENDOR_CMD - 1
};

enum siwifi_vendor_attributes {
	SIWIFI_WLAN_VENDOR_ATTR_ACS_CHANNEL_INVALID = 0,
	SIWIFI_WLAN_VENDOR_ATTR_ACS_PRIMARY_CHANNEL,
	SIWIFI_WLAN_VENDOR_ATTR_ACS_SECONDARY_CHANNEL,
	SIWIFI_WLAN_VENDOR_ATTR_ACS_HW_MODE,
	SIWIFI_WLAN_VENDOR_ATTR_ACS_HT_ENABLED,
	SIWIFI_WLAN_VENDOR_ATTR_ACS_HT40_ENABLED,
	SIWIFI_WLAN_VENDOR_ATTR_ACS_VHT_ENABLED,
	SIWIFI_WLAN_VENDOR_ATTR_ACS_CHWIDTH,
	SIWIFI_WLAN_VENDOR_ATTR_ACS_CH_LIST,
	SIWIFI_WLAN_VENDOR_ATTR_ACS_VHT_SEG0_CENTER_CHANNEL,
	SIWIFI_WLAN_VENDOR_ATTR_ACS_VHT_SEG1_CENTER_CHANNEL,
	SIWIFI_WLAN_VENDOR_ATTR_ACS_FREQ_LIST,

    /* keep last */
	SIWIFI_WLAN_VENDOR_ATTR_ACS_AFTER_LAST,
	SIWIFI_WLAN_VENDOR_ATTR_ACS_MAX =
		SIWIFI_WLAN_VENDOR_ATTR_ACS_AFTER_LAST - 1
};

struct hostapd_acs_param_t {
	//enum ieee80211_phymode hw_mode;
	uint16_t ht_enabled:1,
			 ht40_enabled:1,
			 vht_enabled:1;

	uint16_t ch_width;
	uint16_t ch_list_len;
	const uint8_t *ch_list;

	const uint32_t *freq_list;
};

void siwifi_set_vendor_commands(struct wiphy *wiphy);
int siwifi_acs_scan_done(struct siwifi_hw *siwifi_hw);
int siwifi_acs_scan_abord(struct siwifi_hw *siwifi_hw, bool abord);
#ifdef CONFIG_SIWIFI_ACS_INTERNAL
int siwifi_check_cca(struct siwifi_vif *siwifi_vif);
int siwifi_do_acs(struct siwifi_hw *siwifi_hw, struct siwifi_vif *siwifi_vif);
void siwifi_channel_ctxt_switch(struct work_struct *ws);
void siwifi_chandef_create(struct cfg80211_chan_def *chandef, struct ieee80211_channel *chan, enum nl80211_chan_width width);
int siwifi_acs_scan_abord(struct siwifi_hw *siwifi_hw, bool abord);
int siwifi_fast_channel_switch(struct siwifi_hw *siwifi_hw);
#endif
#endif /* _SIWIFI_VENDOR_H_ */
