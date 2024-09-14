/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _SON_WIRELESS_UTILS_H_
#define _SON_WIRELESS_UTILS_H_

#include "../beerocks_defines.h"
#include "../beerocks_message_structs.h"

#include <tlvf/WSC/eWscAuth.h>
#include <tlvf/WSC/eWscEncr.h>
#include <tlvf/WSC/eWscVendorExt.h>

#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#define NOISE_FIGURE 8
#define TX_POWER_MIN 19
#define ESTIMATE_RSSI_DELTA 4

#define PHY_RATE_TABLE_ANT_MODE_MAX (beerocks::ANT_MODE_2X2_SS2 + 1)
#define PHY_RATE_TABLE_MCS_MAX (beerocks::MCS_9 + 1)
#define PHY_RATE_TABLE_BANDWIDTH_MAX (beerocks::BANDWIDTH_160 + 1)

#define BIT_RATE_MAX_TABLE_SIZE 42

#define LAST_2G_CHANNEL 14

#define START_OF_HIGH_BAND 100
#define END_OF_HIGH_BAND 165
#define END_OF_LOW_BAND 64

#define START_OF_LOW_BAND_NON_DFS 36
#define START_OF_HIGH_BAND_NON_DFS 149
#define END_OF_HIGH_BAND_NON_DFS 161
#define END_OF_LOW_BAND_NON_DFS 48
//DFS CHANNELS
#define START_OF_LOW_DFS_SUBBAND 52
#define START_OF_HIGH_DFS_SUBBAND 100
#define END_OF_LOW_DFS_SUBBAND 64
#define END_OF_HIGH_DFS_SUBBAND 144

#define START_OF_FIRST_DFS_SUBBAND 100
#define START_OF_SECOND_DFS_SUBBAND 116
#define START_OF_THIRD_DFS_SUBBAND 132
#define END_OF_FIRST_DFS_SUBBAND 112
#define END_OF_SECOND_DFS_SUBBAND 128
#define END_OF_THIRD_DFS_SUBBAND 144
//

#define BAND_5G_CHANNEL_CHECK 14
#define RADAR_STATS_LIST_MAX 10

#define RESTRICTED_2G_OVERLAPP_SIZE 5

namespace son {
class wireless_utils {
public:
    enum eEstimationStatus {
        ESTIMATION_SUCCESS              = 0,
        ESTIMATION_FAILURE_BELOW_RANGE  = 1,
        ESTIMATION_FAILURE_INVALID_RSSI = 2
    };

    typedef struct {
        int tx_power;
        int rssi;
        eEstimationStatus status;
    } sPhyUlParams;

    typedef struct {
        bool is_5ghz;
        beerocks::eWiFiBandwidth bw;
        beerocks::eWiFiAntNum ant_num;
        int ant_gain;
        int tx_power;
    } sPhyApParams;

    typedef struct {
        std::string ssid;
        uint16_t vlan_id;
    } sTrafficSeparationSsid;

    typedef struct {
        uint16_t primary_vlan_id;
        uint8_t default_pcp;
    } s8021QSettings;

    typedef struct {
        std::list<uint8_t> operating_class;
        std::string ssid;
        WSC::eWscAuth authentication_type;
        WSC::eWscEncr encryption_type;
        std::string network_key;
        bool fronthaul                                    = false;
        bool backhaul                                     = false;
        bool profile1_backhaul_sta_association_disallowed = false;
        bool profile2_backhaul_sta_association_disallowed = false;
    } sBssInfoConf;

    typedef struct {
        uint16_t phy_rate_100kb;
        double bit_rate_max_mbps;
    } sPhyRateBitRateEntry;

    static sPhyUlParams
    estimate_ul_params(int ul_rssi, uint16_t sta_phy_tx_rate_100kb,
                       const beerocks::message::sRadioCapabilities *capabilities,
                       beerocks::eWiFiBandwidth ap_bw, bool is_5ghz);
    static int estimate_dl_rssi(int ul_rssi, int tx_power, const sPhyApParams &ap_params);
    static double estimate_ap_tx_phy_rate(int estimated_dl_rssi,
                                          const beerocks::message::sRadioCapabilities *capabilities,
                                          beerocks::eWiFiBandwidth ap_bw, bool is_5ghz);

    static double get_load_max_bit_rate_mbps(double phy_rate_100kb);
    static bool get_mcs_from_rate(const uint16_t rate, const beerocks::eWiFiAntMode ant_mode,
                                  const beerocks::eWiFiBandwidth bw, uint8_t &mcs,
                                  uint8_t &short_gi);

    static int channel_to_freq(int channel);

    /**
     * @brief Obtains the channel number that corresponds to given frequency value.
     *
     * @param freq frequency value in MHz.
     * @return channel number.
     */
    static int freq_to_channel(int freq);
    static uint16_t channel_to_vht_center_freq(int channel, beerocks::eWiFiBandwidth bandwidth,
                                               bool channel_ext_above_secondary);
    static beerocks::eFreqType which_freq(uint32_t chn);
    static bool is_same_freq_band(int chn1, int chn2);
    static beerocks::eSubbandType which_subband(uint32_t chn);
    static bool is_low_subband(const uint32_t chn);
    static bool is_high_subband(const uint32_t chn);
    static bool is_dfs_channel(const uint32_t chn);
    static bool is_same_interface(const std::string &ifname1, const std::string &ifname2);
    static std::vector<std::pair<uint8_t, beerocks::eWifiChannelType>>
    split_channel_to_20MHz(int channel, beerocks::eWiFiBandwidth bw,
                           bool channel_ext_above_secondary, bool channel_ext_above_primary);
    static uint8_t channel_step_multiply(bool channel_ext_above_secondary,
                                         bool channel_ext_above_primary);
    static std::vector<uint8_t> get_5g_20MHz_channels(beerocks::eWiFiBandwidth bw,
                                                      uint16_t vht_center_frequency);
    static std::vector<uint8_t> calc_5g_20MHz_subband_channels(beerocks::eWiFiBandwidth prev_bw,
                                                               uint16_t prev_vht_center_frequency,
                                                               beerocks::eWiFiBandwidth bw,
                                                               uint16_t vht_center_frequency);
    static const std::set<uint8_t> &operating_class_to_channel_set(uint8_t operating_class);
    static const beerocks::eWiFiBandwidth &operating_class_to_bandwidth(uint8_t operating_class);
    static std::string wsc_to_bwl_authentication(WSC::eWscAuth authtype);
    static std::string wsc_to_bwl_encryption(WSC::eWscEncr enctype);
    static beerocks::eBssType wsc_to_bwl_bss_type(WSC::eWscVendorExtSubelementBssType bss_type);
    static std::list<uint8_t> string_to_wsc_oper_class(const std::string &operating_class);
    static uint8_t get_5g_center_channel(uint8_t channel, beerocks::eWiFiBandwidth bandwidth);
    static uint8_t get_operating_class_by_channel(const beerocks::message::sWifiChannel &channel);

    /**
    * @brief Match channel number in the given operating class.
    *
    * @param operating_class operating class
    * @param channel channel number
    * @return True if channel matches to operating class
    */
    static bool is_channel_in_operating_class(uint8_t operating_class, uint8_t channel);

    /**
     * @brief Check if frequency band is 5GHz frequency
     *
     * @return False if band is not 5GHz or there is not enoguh data, true otherwise
     */
    static bool is_frequency_band_5ghz(beerocks::eFreqType frequency_band);

    /**
     * @brief A vector of overlapping channels and bandwidths.
     * E.g taken from wireless_utils::channels_table_5g (see the cpp file):
     * {{104,beerocks::BANDWIDTH_80}, {112,beerocks::BANDWIDTH_40}, ... }
     * they are both overlapping because both share channel 108.
     * Note: the common channel is NOT part of this structure
     */
    using OverlappingChannels = std::vector<std::pair<uint8_t, beerocks::eWiFiBandwidth>>;

    /**
     * @brief Calculates the list of overlapping channels and bandwidths
     * for the given source channel.
     *
     * @param source_channel The channel to calculate its overlapping pairs.
     * @return OverlappingChannles A vector of the overlapping channels for the
     * given source channel. Empty list if not as such.
     */
    static OverlappingChannels get_overlapping_channels(uint8_t source_channel);

    /**
     * @brief Get a list of overlapping beacon channel for a given channel and bandwidth.
     *
     * @param beacon_channel Channel.
     * @param bw Bandwidth.
     * @return std::vector<uint8_t> List of overlapping beacon channels (20 MHz).
     */
    static std::vector<uint8_t> get_overlapping_beacon_channels(uint8_t beacon_channel,
                                                                beerocks::eWiFiBandwidth bw);

    /**
     * @brief Get list of all possible beacon channels for a give center channel and bandwidth on
     * the 5G band.
     *
     * @param center_channel Center channel.
     * @param bw Bandwidth.
     * @return List of beacon channels that have the given center channel.
     */
    static std::vector<uint8_t> center_channel_5g_to_beacon_channels(uint8_t center_channel,
                                                                     beerocks::eWiFiBandwidth bw);

    struct sChannel {
        uint8_t center_channel;
        std::pair<uint8_t, uint8_t> overlap_beacon_channels_range;
    };
    static const std::map<uint8_t, std::map<beerocks::eWiFiBandwidth, sChannel>> channels_table_5g;

    struct sOperatingClass {
        std::set<uint8_t> channels;
        beerocks::eWiFiBandwidth band;
    };
    // Key: Operating Class
    static const std::map<uint8_t, sOperatingClass> operating_classes_list;

    static bool has_operating_class_channel(const sOperatingClass &oper_class, uint8_t channel,
                                            beerocks::eWiFiBandwidth bw);

    /**
     * @brief get max supported bandwidth in station capabilities.
     * in this order:
     * - max_ch_width (valid even for a/b/g)
     * - vht_bw (valid for ac)
     * - ht_bw (valid for n)
     * @param sta_caps in station capabilities
     * @param max_bw out filled max supported bandwidth
     * @return false if none of above is valid bw (+unchanged out param)
     */
    static bool get_station_max_supported_bw(beerocks::message::sRadioCapabilities &sta_caps,
                                             beerocks::eWiFiBandwidth &max_bw);

    /**
     * @brief Makes conversion from RSSI to RCPI.
     *
     * RCPI means Received channel power indicator.
     * RSSI means Received signal strength indicator.
     *
     * This method can only return between 0-220 values.
     *
     * Between 221-254 values are reserved (MultiAP Spec.).
     * 255 means measurement is not avaliable.
     *
     * @param rssi signal strength mostly negative value.
     * @return converted rcpi value.
     */
    static uint8_t convert_rcpi_from_rssi(int8_t rssi);

    /**
     * @brief Makes conversion from RCPI to RSSI.
     *
     * RCPI means Received channel power indicator.
     * RSSI means Received signal strength indicator.
     *
     * Between 221-254 values are reserved.
     * In case of these values are requested to be converted, it returns RSSI_INVALID value.
     *
     * @param rcpi signal power indicator value.
     * @return converted rssi value.
     */
    static int8_t convert_rssi_from_rcpi(uint8_t rcpi);

    /**
     * @brief Retrieve the subset of 20MHz channels of the given channel & bandwidth
     * 
     * @param [in] channel_number Central channel number.
     * @param [in] operating_bandwidth Bandwidth of the given channel.
     * @param [out] resulting_channels set containing the resulting 20MHz channels
     * @return true if the operation was successful, otherwise false. 
     */
    static bool get_subset_20MHz_channels(const uint8_t channel_number,
                                          const uint8_t operating_class,
                                          const beerocks::eWiFiBandwidth operating_bandwidth,
                                          std::unordered_set<uint8_t> &resulting_channels);

    /**
     * @brief Print station capabilities.
     *
     * @param sta_caps Capabilities to be displayed.
     * @return void.
     */
    static void print_station_capabilities(beerocks::message::sRadioCapabilities &sta_caps);

    /**
     * @brief Calculate the vht MCS set (mask of 16bits)
     * using the maximum supported MCS and number of Spatial streams
     * (Cf. IEEE802.11-2016 Figure 9-562-Rx VHT-MCS Map and Tx VHT-MCS Map subfields).
     *
     * @param vht_mcs_max Maximum supported MCS value.
     * @param vht_ss_max Maximum supported number of spatial stream.
     * @return Calculated VHT MCS set 16 bits mask.
     */
    static uint16_t get_vht_mcs_set(uint8_t vht_mcs, uint8_t vht_ss);

private:
    enum eAntennaFactor {
        ANT_FACTOR_1X1 = 0,
        ANT_FACTOR_2X2 = 0,
        ANT_FACTOR_3X3 = 2,
        ANT_FACTOR_4X4 = 3,
    };

    typedef struct {
        uint16_t gi_long_rate;
        uint16_t gi_short_rate;
        int16_t rssi;
    } sPhyRateTableValues;

    typedef struct {
        int8_t tx_power_2_4;
        int8_t tx_power_5;
        std::map<uint8_t, sPhyRateTableValues> bw_values; //20/40/80/160
    } sPhyRateTableEntry;

    // LUT for phy parameters //
    static constexpr beerocks::eWiFiAntNum
        phy_rate_table_mode_to_ant_num[PHY_RATE_TABLE_ANT_MODE_MAX] = {
            beerocks::ANT_1X1, beerocks::ANT_2X2, beerocks::ANT_2X2};
    static constexpr beerocks::eWiFiSS phy_rate_table_mode_to_ss[PHY_RATE_TABLE_ANT_MODE_MAX] = {
        beerocks::SS_1, beerocks::SS_1, beerocks::SS_2};

    // clang-format off
    static const sPhyRateTableEntry phy_rate_table[PHY_RATE_TABLE_ANT_MODE_MAX][PHY_RATE_TABLE_MCS_MAX];

    static constexpr sPhyRateBitRateEntry bit_rate_max_table_mbps[BIT_RATE_MAX_TABLE_SIZE] = {
            {65,3},     {130,86},   {135,71},   {195,89},   {234,107},  {260,92},   {270,125},  {293,129},  {390,133},  {405,139},
            {520,140},  {540,141},  {585,146},  {650,147},  {780,148},  {810,151},  {878,151},  {1040,161}, {1080,172}, {1170,182},
            {1215,156}, {1300,189}, {1350,157}, {1560,196}, {1620,202}, {1755,208}, {1800,161}, {2160,213}, {2340,241}, {2430,220},
            {2633,169}, {2700,232}, {2925,189}, {3240,237}, {3510,251}, {3600,246}, {3900,328}, {4680,251}, {5265,253}, {5850,261},
            {7020,266}, {7800,266}
    };

    // clang-format on
};
} // namespace son

#endif
