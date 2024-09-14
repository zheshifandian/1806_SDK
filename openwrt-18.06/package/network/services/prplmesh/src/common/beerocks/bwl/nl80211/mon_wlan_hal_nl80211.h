/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BWL_MON_WLAN_HAL_NL80211_H_
#define _BWL_MON_WLAN_HAL_NL80211_H_

#include "base_wlan_hal_nl80211.h"
#include <bwl/mon_wlan_hal.h>

namespace bwl {
namespace nl80211 {

/*!
 * Hardware abstraction layer for WLAN Monitor.
 */
class mon_wlan_hal_nl80211 : public base_wlan_hal_nl80211, public mon_wlan_hal {

    // Public methods
public:
    /*!
     * Constructor.
     *
     * @param [in] iface_name Monitor interface name.
     * @param [in] callback Callback for handling internal events.
     */
    mon_wlan_hal_nl80211(const std::string &iface_name, hal_event_cb_t callback,
                         const bwl::hal_conf_t &hal_conf);
    virtual ~mon_wlan_hal_nl80211();

    virtual bool update_radio_stats(SRadioStats &radio_stats) override;
    virtual bool update_vap_stats(const std::string &vap_iface_name, SVapStats &vap_stats) override;
    virtual bool update_stations_stats(const std::string &vap_iface_name,
                                       const std::string &sta_mac, SStaStats &sta_stats) override;
    virtual bool update_station_qos_control_params(const std::string &vap_iface_name,
                                                   const std::string &sta_mac,
                                                   SStaQosCtrlParams &sta_qos_ctrl_params) override;

    virtual bool sta_channel_load_11k_request(const std::string &vap_iface_name,
                                              const SStaChannelLoadRequest11k &req) override;
    virtual bool sta_beacon_11k_request(const std::string &vap_iface_name,
                                        const SBeaconRequest11k &req, int &dialog_token) override;
    virtual bool sta_link_measurements_11k_request(const std::string &vap_iface_name,
                                                   const std::string &sta_mac) override;
    virtual bool channel_scan_trigger(int dwell_time_msec,
                                      const std::vector<unsigned int> &channel_pool) override;
    virtual bool channel_scan_dump_results() override;
    virtual bool channel_scan_dump_cached_results() override;

    /**
     * @brief Generates connected events for already connected clients.
     *
     * @see mon_wlan_hal::generate_connected_clients_events
     */
    virtual bool generate_connected_clients_events(
        bool &is_finished_all_clients,
        const std::chrono::steady_clock::time_point max_iteration_timeout =
            std::chrono::steady_clock::time_point::max()) override;

    /**
     * @brief Clear progress of generate-connected-clients-events.
     * 
     * @see mon_wlan_hal::pre_generate_connected_clients_events
     */
    virtual bool pre_generate_connected_clients_events() override;

    virtual bool channel_scan_abort() override;
    virtual bool set_estimated_service_parameters(uint8_t *esp_info_field) override;

    // Protected methods:
protected:
    virtual bool process_nl80211_event(parsed_obj_map_t &parsed_obj) override;

    // Overload for Monitor events
    bool event_queue_push(mon_wlan_hal::Event event, std::shared_ptr<void> data = {})
    {
        return base_wlan_hal::event_queue_push(int(event), data);
    }

    // Private data-members:
private:
    std::shared_ptr<char> m_temp_wav_value;
};

} // namespace nl80211
} // namespace bwl

#endif // _BWL_MON_WLAN_HAL_NL80211_H_
