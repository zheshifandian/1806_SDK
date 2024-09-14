/**
 ******************************************************************************
 *
 * @file siwifi_mod_params.h
 *
 * @brief Declaration of module parameters
 *
 * Copyright (C) Siflower 2018-2025
 *
 ******************************************************************************
 */

#ifndef _SIWIFI_MOD_PARAM_H_
#define _SIWIFI_MOD_PARAM_H_

struct siwifi_mod_params {
    bool is_hb;
    bool ht_on;
    bool vht_on;
    bool he_on;
    int mcs_map;
    int he_mcs_map;
    bool ldpc_on;
    bool stbc_on;
    int phy_cfg;
    int uapsd_timeout;
    bool ap_uapsd_on;
    bool sgi;
    bool sgi80;
    bool use_2040;
    bool use_80;
    bool custregd;
    int nss;
    int amsdu_rx_max;
    bool bfmee;
    bool bfmer;
    bool mesh;
    bool murx;
    bool mutx;
    bool mutx_on;
    unsigned int roc_dur_max;
    int listen_itv;
    bool listen_bcmc;
    int lp_clk_ppm;
    bool ps_on;
    int tx_lft;
    int amsdu_maxnb;
    int uapsd_queues;
    bool tdls;
    unsigned int txpower_lvl;
    bool addr_maskall;
	bool not_send_null;
    unsigned int ampdu_max_cnt;
	int rts_cts_change;
#ifdef INDEPENDENT_ANTENNA_CONTROL
	bool independent_antenna_control;
#endif
    int fixed_mcs_index;
    bool uf;
    int amsdu_int;
    unsigned int led_status;
    bool ant_div;
    int tx_queue_num;
    bool radar_debugmode;
    bool radar_debug_printk;
    int ampdu_density;
};

//extern struct siwifi_mod_params siwifi_mod_params;

struct siwifi_hw;
struct wiphy;
int siwifi_handle_dynparams(struct siwifi_hw *siwifi_hw, struct wiphy *wiphy);
void siwifi_enable_wapi(struct siwifi_hw *siwifi_hw);
void siwifi_enable_mfp(struct siwifi_hw *siwifi_hw);

#endif /* _SIWIFI_MOD_PARAM_H_ */
