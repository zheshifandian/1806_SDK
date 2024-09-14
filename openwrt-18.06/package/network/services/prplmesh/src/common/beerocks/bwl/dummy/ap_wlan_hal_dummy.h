/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BWL_AP_WLAN_HAL_DUMMY_H_
#define _BWL_AP_WLAN_HAL_DUMMY_H_

#include "base_wlan_hal_dummy.h"
#include <bwl/ap_wlan_hal.h>

namespace bwl {
namespace dummy {

/*!
 * Hardware abstraction layer for WLAN Access Point.
 */
class ap_wlan_hal_dummy : public base_wlan_hal_dummy, public ap_wlan_hal {

    // Public methods
public:
    /*!
     * Constructor.
     *
     * @param [in] iface_name AP interface name.
     * @param [in] callback Callback for handling internal events.
     */
    ap_wlan_hal_dummy(const std::string &iface_name, hal_event_cb_t callback,
                      const hal_conf_t &hal_conf);

    virtual ~ap_wlan_hal_dummy();

    virtual HALState attach(bool block = false) override;
    virtual bool enable() override;
    virtual bool disable() override;
    virtual bool set_start_disabled(bool enable, int vap_id = beerocks::IFACE_RADIO_ID) override;
    virtual bool
    set_channel(int chan, beerocks::eWiFiBandwidth bw = beerocks::eWiFiBandwidth::BANDWIDTH_UNKNOWN,
                int center_channel = 0) override;
    virtual bool sta_allow(const std::string &mac, const std::string &bssid) override;
    virtual bool sta_deny(const std::string &mac, const std::string &bssid) override;
    virtual bool sta_disassoc(int8_t vap_id, const std::string &mac, uint32_t reason = 0) override;
    virtual bool sta_deauth(int8_t vap_id, const std::string &mac, uint32_t reason = 0) override;
    virtual bool sta_bss_steer(int8_t vap_id, const std::string &mac, const std::string &bssid,
                               int oper_class, int chan, int disassoc_timer_btt, int valid_int_btt,
                               int reason) override;
    virtual bool
    update_vap_credentials(std::list<son::wireless_utils::sBssInfoConf> &bss_info_conf_list,
                           const std::string &backhaul_wps_ssid,
                           const std::string &backhaul_wps_passphrase) override;
    virtual bool sta_unassoc_rssi_measurement(const std::string &mac, int chan, int bw,
                                              int vht_center_frequency, int delay,
                                              int window_size) override;
    virtual bool sta_softblock_add(const std::string &vap_name, const std::string &client_mac,
                                   uint8_t reject_error_code, uint8_t probe_snr_threshold_hi,
                                   uint8_t probe_snr_threshold_lo,
                                   uint8_t authetication_snr_threshold_hi,
                                   uint8_t authetication_rsnr_threshold_lo) override;

    virtual bool sta_softblock_remove(const std::string &vap_name,
                                      const std::string &client_mac) override;
    virtual bool switch_channel(int chan, int bw, int vht_center_frequency) override;
    virtual bool cancel_cac(int chan, beerocks::eWiFiBandwidth bw, int vht_center_frequency,
                            int secondary_chan) override;
    virtual bool failsafe_channel_set(int chan, int bw, int vht_center_frequency) override;
    virtual bool failsafe_channel_get(int &chan, int &bw) override;
    virtual bool is_zwdfs_supported() override;
    virtual bool set_zwdfs_antenna(bool enable) override;
    virtual bool is_zwdfs_antenna_enabled() override;
    virtual bool hybrid_mode_supported() override;
    virtual bool restricted_channels_set(char *channel_list) override;
    virtual bool restricted_channels_get(char *channel_list) override;
    virtual bool read_acs_report() override;
    virtual bool set_tx_power_limit(int tx_pow_limit) override;
    virtual bool set_vap_enable(const std::string &iface_name, const bool enable) override;
    virtual bool get_vap_enable(const std::string &iface_name, bool &enable) override;

    /**
     * @brief Generates connected events for already connected clients.
     *
     * @see ap_wlan_hal::generate_connected_clients_events
     */
    virtual bool generate_connected_clients_events(
        bool &is_finished_all_clients,
        const std::chrono::steady_clock::time_point max_iteration_timeout =
            std::chrono::steady_clock::time_point::max()) override;

    /**
     * @brief Clear progress of generate-connected-clients-events.
     * 
     * @see ap_wlan_hal::pre_generate_connected_clients_events
     */
    virtual bool pre_generate_connected_clients_events() override;

    virtual bool start_wps_pbc() override;
    virtual bool set_mbo_assoc_disallow(const std::string &bssid, bool enable) override;
    virtual bool set_radio_mbo_assoc_disallow(bool enable) override;
    virtual bool set_primary_vlan_id(uint16_t primary_vlan_id) override;

    // Protected methods:
protected:
    virtual bool process_dummy_event(parsed_obj_map_t &parsed_obj) override;
    virtual bool process_dummy_data(parsed_obj_map_t &parsed_obj) override;

    // Overload for AP events
    bool event_queue_push(ap_wlan_hal::Event event, std::shared_ptr<void> data = {})
    {
        return base_wlan_hal::event_queue_push(int(event), data);
    }

    virtual bool set(const std::string &param, const std::string &value, int vap_id) override;

private:
    // Unassociated measurement state variables
    std::chrono::steady_clock::time_point m_unassoc_measure_start;
    int m_unassoc_measure_window_size = 0;
    int m_unassoc_measure_delay       = 0;

    bool m_drop_csa = false;
    std::chrono::steady_clock::time_point m_csa_event_filtering_timestamp;
};

} // namespace dummy
} // namespace bwl

#endif // _BWL_AP_WLAN_HAL_DUMMY_H_
