/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BWL_AP_WLAN_HAL_H_
#define _BWL_AP_WLAN_HAL_H_

#include "base_wlan_hal.h"
#include <bcl/beerocks_string_utils.h>

namespace bwl {

/**
 * Hardware abstraction layer for WLAN Access Point.
 * Read more about virtual inheritance: https://en.wikipedia.org/wiki/Virtual_inheritance
 */
class ap_wlan_hal : public virtual base_wlan_hal {

    // Public definitions
public:
    enum class Event {
        Invalid = 0,

        AP_Attached,
        AP_Enabled,
        AP_Disabled,
        AP_Sta_Possible_Psk_Mismatch,

        STA_Connected,
        STA_Disconnected,
        STA_Unassoc_RSSI,
        STA_Softblock_Drop,
        STA_Steering_Probe_Req,
        STA_Steering_Auth_Fail,

        Interface_Enabled,
        Interface_Disabled,

        ACS_Started,
        ACS_Completed,
        ACS_Failed,

        CSA_Finished,
        CTRL_Channel_Switch,

        BSS_TM_Query,
        BSS_TM_Response,

        DFS_CAC_Started,
        DFS_CAC_Completed,
        DFS_NOP_Finished,
        DFS_RADAR_Detected,
        AP_MGMT_FRAME_RECEIVED,

        MGMT_Frame /**< 802.11 management frame payload */
    };

    // Public methods
public:
    virtual ~ap_wlan_hal() = default;

    /**
     * @brief Enable the radio interface
     *
     * @return true on success or false on error.
     */
    virtual bool enable() = 0;

    /**
     * @brief Disable the radio interface
     *
     * @return true on success or false on error.
     */
    virtual bool disable() = 0;

    /**
     * @brief Set start_disabled flag
     * 
     * @param [in] enable The start_disabled flag
     * @param [in] vap_id vap_id to set
     * 
     * @return true on success or false on error.
     */
    virtual bool set_start_disabled(bool enable, int vap_id = beerocks::IFACE_RADIO_ID) = 0;

    /**
     * @brief Set the AP channel
     * 
     * @param [in] chan The channel to switch to.
     * @param [in] bw The bandwidth (in Mhz) of the target channel.
     * @param [in] center_channel VHT center frequency.
     * 
     * @return true on success or false on error.
     */
    virtual bool set_channel(int chan, beerocks::eWiFiBandwidth bw, int center_channel) = 0;

    /**
     * @brief Allow the station with the given MAC address to connect.
     *
     * @param [in] mac The MAC address of the station.
     * @param [in] bssid The BSSID to which the operation is applicable.
     *
     * @return true on success or false on error.
     */
    virtual bool sta_allow(const std::string &mac, const std::string &bssid) = 0;

    /**
     * @brief Deny the station with the given MAC address from connecting to the AP.
     *
     * @param [in] mac The MAC address of the station.        
     * @param [in] bssid The BSSID to which the operation is applicable.
     * 
     * @return true on success or false on error.
     */
    virtual bool sta_deny(const std::string &mac, const std::string &bssid) = 0;

    /**
     * @brief Disassociate the station with the given MAC address.
     *
     * @param [in] vap_id
     * @param [in] mac The MAC address of the station.
     * @param [in] reason The reason for the disassociation
     *
     * @return true on success or false on error.
     */
    virtual bool sta_disassoc(int8_t vap_id, const std::string &mac, uint32_t reason = 0) = 0;

    /**
     * @brief Deauthenticate the station with the given MAC address.
     *
     * @param [in] vap_id
     * @param [in] mac The MAC address of the station.
     * @param [in] reason The reason for the deauthenticate
     *
     * @return true on success or false on error.
     */
    virtual bool sta_deauth(int8_t vap_id, const std::string &mac, uint32_t reason = 0) = 0;

    /**
     * @brief Send a 802.11v steer request (BSS Transition) to a connected station.
     *
     * @param [in] vap_id The VAP index of source AP.
     * @param [in] mac The MAC address of the station.
     * @param [in] bssid The MAC address of the target AP.
     * @param [in] chan The channel of the target AP.
     * @param [in] disassoc_timer_btt Time in beacon transmit inteval units  (1 BTT = ~100ms)
     *             before the AP should forcefully disconnect the client. 
     *             Setting a non-ZERO value should enable the "disassociation imminent" function and
     *             arm the internal AP timer (usually performed by the hardware).
     * @param [in] valid_int_btt The number of beacon transmission times (TBTTs) 
     *             until the BSS transition candidate list is no longer valid.
     * @param [in] reason The reason code for the steer based on Table 18 @ Wi-Fi Agile Multiband Technical Specification
     * @return true on success or false on error.
     */
    virtual bool sta_bss_steer(int8_t vap_id, const std::string &mac, const std::string &bssid,
                               int oper_class, int chan, int disassoc_timer_btt, int valid_int_btt,
                               int reason) = 0;

    /**
     * @brief Update wifi credentials.
     *
     * @param [in] bss_info_conf_list List of wifi credentials.
     * @param [in] backhaul_wps_ssid backhaul ssid used for wps onboarding
     * @param [in] backhaul_wps_passphrase backhaul passphrase used for wps onboarding
     * @return true on success or false on error.
     */

    virtual bool
    update_vap_credentials(std::list<son::wireless_utils::sBssInfoConf> &bss_info_conf_list,
                           const std::string &backhaul_wps_ssid,
                           const std::string &backhaul_wps_passphrase) = 0;

    /**
     * TODO: Move to the base class?
     * 
     * @brief Measure the RSSI of an unassociated station.
     * The result of the measurement should be sent as an internal event.
     * 
     * @param [in] mac The MAC address of the station.
     * @param [in] chan The channel of the target AP.
     * @param [in] bw The bandwidth (in Mhz) of the target channel.
     * @param [in] vht_center_frequency VHT center frequency.
     * @param [in] delay Delay in milliseconds before the beginning of the measurement window.
     * @param [in] window_size Measurement window size (in milliseconds).
     *
     * @return true on success or false on error.
     */
    virtual bool sta_unassoc_rssi_measurement(const std::string &mac, int chan, int bw,
                                              int vht_center_frequency, int delay,
                                              int window_size) = 0;

    /**
     * @brief Add a station to softblock list
     * 
     * @param [in] vap_name The of the vap
     * @param [in] client_mac The MAC address of the station.
     * @param [in] reject_error_code The reject error code that will be send to the client when it try to connect if it not between the thresholds.
     * @param [in] probe_snr_threshold_hi Probe response SNR high threshold.
     * @param [in] probe_snr_threshold_lo Probe response SNR low threshold
     * @param [in] authetication_snr_threshold_hi Authetication response SNR high threshold.
     * @param [in] authetication_snr_threshold_lo Authetication response SNR low threshold.
     *
     * @return true on success or false on error.
     */
    virtual bool sta_softblock_add(const std::string &vap_name, const std::string &client_mac,
                                   uint8_t reject_error_code, uint8_t probe_snr_threshold_hi,
                                   uint8_t probe_snr_threshold_lo,
                                   uint8_t authetication_snr_threshold_hi,
                                   uint8_t authetication_snr_threshold_lo) = 0;

    // TODO: To be removed?  since hostapd doesn't support removeing STA from softblock list.
    /**
     * @brief Remove a station from softblock list
     * 
     * @param [in] vap_name The of the vap
     * @param [in] client_mac The MAC address of the station.
     *
     * @return true on success or false on error.
     */
    virtual bool sta_softblock_remove(const std::string &vap_name,
                                      const std::string &client_mac) = 0;
    /**
     * @brief Switch the AP to the given channel.
     *
     * @param [in] chan The channel to switch to.
     * @param [in] bw The bandwidth (in Mhz) of the target channel.
     * @param [in] vht_center_frequency VHT center frequency.
     *
     * @return true on success or false on error.
     */
    virtual bool switch_channel(int chan, int bw, int vht_center_frequency) = 0;

    /**
     * @brief cancel active cac if exsits.
     *
     * @param [in] chan The channel to switch to after the cancelation.
     * @param [in] bw The bandwidth (in Mhz) of the target channel.
     * @param [in] vht_center_frequency VHT center frequency.
     * @param [in] secondary_channel_offset The secondary channel's offset
     * from chan. either: -1, 0, or +1
     *
     * @return true if everything went well or false on error.
     * note: returns true if there was no active cac.
     */
    virtual bool cancel_cac(int chan, beerocks::eWiFiBandwidth bw, int vht_center_frequency,
                            int secondary_channel_offset) = 0;

    /**
     * @brief Set failsafe channel.
     * 
     * @param [in] chan Channel number.
     * @param [in] bw Channel bandwidth.
     * @param [in] vht_center_frequency VHT center frequency.
     * 
     * @return true on success or false on error.
     */
    virtual bool failsafe_channel_set(int chan, int bw, int vht_center_frequency) = 0;

    /**
     * @brief Get the failsafe channel.
     * 
     * @param [out] chan Channel number.
     * @param [out] bw Channel bandwidth.
     * 
     * @return true on success or false on error.
     */
    virtual bool failsafe_channel_get(int &chan, int &bw) = 0;

    /**
     * @brief Check if radio hardware supports zwdfs.
     * 
     * @return true when supported, otherwise false.
     */
    virtual bool is_zwdfs_supported() = 0;

    /**
     * @brief Switch zwdfs antenna off/on
     * 
     * @param enable true to switch on, false to switch off.
     * 
     * @return true on success or false on error.
     */
    virtual bool set_zwdfs_antenna(bool enable) = 0;

    /**
     * @brief Check if zwdfs antenna is enabled on the radio.
     * 
     * @return true when antenna is enabled, otherwise false.
     */
    virtual bool is_zwdfs_antenna_enabled() = 0;

    /**
     * @brief Check if the radio supports configuring a bssid on hybrid mode (fBSS=1 & bBSS=1).
     * 
     * @return true if hybrid mode is supported, otherwise false.
     */
    virtual bool hybrid_mode_supported() = 0;

    // TODO: UPDATE AFTER THE MERGE WITH CHANNEL SELECTION
    virtual bool restricted_channels_set(char *channel_list) = 0;
    virtual bool restricted_channels_get(char *channel_list) = 0;
    //virtual bool restricted_channels_set(...);
    //virtual bool restricted_channels_get(...);

    /**
     * @brief Read the ACS (Automatic Channel Selection) report from the hardware.
     * On successful completion the infromation can be retrieved 
     * using the get_acs_report() method.
     *
     * @return true on success or false on error.
     */
    virtual bool read_acs_report() = 0;

    /**
     * @brief Set Transmit Power Limit 
     *
     * @param [in] tx_pow_limit Transmit Power Limit in dBm.
     *
     * @return true on success or false on error.
     */
    virtual bool set_tx_power_limit(int tx_pow_limit) = 0;

    /**
     * @brief Set/Get enable vap beacon transmittion.
     * 
     * @return true on success or false on error.
     */
    virtual bool set_vap_enable(const std::string &iface_name, const bool enable) = 0;
    virtual bool get_vap_enable(const std::string &iface_name, bool &enable)      = 0;

    /**
     * @brief Set MBO Association Disallow parameter for BSSID.
     * 
     * @param [in] bssid BSSID of the VAP to set the parameter.
     * @param [in] enable Enable or disable the MBO Association Disallow parameter.
     * 
     * @return true on success or false on error.
     */
    virtual bool set_mbo_assoc_disallow(const std::string &bssid, bool enable) = 0;

    /**
     * @brief Set MBO Association Disallow parameter all available vaps.
     * 
     * @param [in] enable enable or disable the MBO Association Disallow.
     * 
     * @return true on success or false on error.
     */
    virtual bool set_radio_mbo_assoc_disallow(bool enable) = 0;

    /**
     * @brief Generates client-connected event for already connected clients.
     * This is used to overcome a scenario where clients that are already connected
     * are not known to prplmesh and "missed" the "connected" event for them. This scenario
     * can happen due to prplmesh unexpected restart, son-slave unexpected restart and/or during development
     * when prplmesh is intentionally restarted.
     * 
     * @param [out] is_finished_all_clients - Is generation for all clients complete
     * @param [in] max_iteration_timeout - The time when thread awake time expires and function must return
     * 
     * @return true if finished generating, false otherwise
     */
    virtual bool generate_connected_clients_events(
        bool &is_finished_all_clients,
        const std::chrono::steady_clock::time_point max_iteration_timeout =
            std::chrono::steady_clock::time_point::max()) = 0;

    /**
     * @brief The generate connected clients events can be called several times by an agent (after
     * the agent re-establishes connection to a controller). To support this we need to be able to clear
     * the "progress" of the client's events generation before calling the generate_connected_clients_events
     * API repetitively.
     *  The API resets the lists of "handled_clients" and "completed_vaps" that manage the already handled clients and VAPs.
     * 
     * @return true 
     * @return false 
     */
    virtual bool pre_generate_connected_clients_events() = 0;

    /**
     * @brief Start WPS PBC procedure on a given VAP 
     *
     * @param iface_name VAP interface on which to start WPS PBC
     *
     * @return true on success or false on error
     */
    virtual bool start_wps_pbc() = 0;

    /**
     * @brief Set primary VLAN ID value on the Radio.
     * The primary VLAN ID will be added into the Multi-AP extention IE.
     * If the primary VLAN ID is zero, it unset it, and not add it to the IE.
     *
     * @param primary_vlan_id Primary VLAN ID.
     * @return true on success, false otherwise.
     */
    virtual bool set_primary_vlan_id(uint16_t primary_vlan_id) = 0;

private:
    static const int frame_body_idx = (sizeof(s80211MgmtFrame::sHeader) * 2);

public:
    /**
     * @brief Return the Management Request frame body
     *
     * @param assoc_req input assumed (Re)Association request hex string buffer
     * @return string containing bytes of frame body.
     */
    static std::string get_binary_association_frame(const char assoc_req[])
    {
        auto sub_str = std::string(assoc_req).substr(0, ASSOCIATION_FRAME_SIZE - 1);
        if (sub_str.length() <= frame_body_idx) {
            return {};
        }
        sub_str.erase(0, frame_body_idx);

        //convert the hex string to binary
        return beerocks::string_utils::hex_to_bytes<std::string>(sub_str);
    };
};

// AP HAL factory types
std::shared_ptr<ap_wlan_hal> ap_wlan_hal_create(std::string iface_name, hal_conf_t hal_conf,
                                                base_wlan_hal::hal_event_cb_t cb);
} // namespace bwl

#endif // _BWL_AP_WLAN_HAL_H_
