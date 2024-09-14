/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BWL_MON_WLAN_HAL_DWPAL_H_
#define _BWL_MON_WLAN_HAL_DWPAL_H_

#include "base_wlan_hal_dwpal.h"
#include "mon_wlan_hal_dwpal_types.h"
#include <bcl/network/network_utils.h>
#include <bwl/mon_wlan_hal.h>

namespace bwl {
namespace dwpal {

/*!
 * Hardware abstraction layer for WLAN Monitor.
 */
class mon_wlan_hal_dwpal : public base_wlan_hal_dwpal, public mon_wlan_hal {

    // Public methods
public:
    /*!
     * Constructor.
     *
     * @param [in] iface_name Monitor interface name.
     * @param [in] callback Callback for handling internal events.
     */
    mon_wlan_hal_dwpal(const std::string &iface_name, hal_event_cb_t callback,
                       const bwl::hal_conf_t &hal_conf);
    virtual ~mon_wlan_hal_dwpal();

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
    virtual bool process_dwpal_event(char *buffer, int bufLen, const std::string &opcode) override;
    virtual bool process_dwpal_nl_event(struct nl_msg *msg, void *arg = nullptr) override;

    // Overload for Monitor events
    bool event_queue_push(mon_wlan_hal::Event event, std::shared_ptr<void> data = {})
    {
        return base_wlan_hal::event_queue_push(int(event), data);
    }

    // Private data-members:
private:
    bool dwpal_get_scan_params_fg(sScanCfgParams &params, size_t &result_size)
    {
        ssize_t received_result_size =
            dwpal_nl_cmd_get(m_radio_info.iface_name, LTQ_NL80211_VENDOR_SUBCMD_GET_SCAN_PARAMS,
                             m_nl_buffer, NL_MAX_REPLY_BUFFSIZE);
        if (received_result_size <= 0) {
            LOG(ERROR) << "LTQ_NL80211_VENDOR_SUBCMD_GET_SCAN_PARAMS failed!"
                       << " received size <= 0";
            result_size = ScanCfgParams_size_invalid;
            return false;
        }
        result_size = received_result_size;
        if (result_size != ScanCfgParams_size) {
            LOG(ERROR) << "LTQ_NL80211_VENDOR_SUBCMD_GET_SCAN_PARAMS failed!"
                       << " expected size = " << ScanCfgParams_size
                       << ", received size = " << result_size;
            return false;
        }

        std::copy_n(m_nl_buffer, result_size, reinterpret_cast<unsigned char *>(&params));
        return true;
    }

    bool dwpal_get_scan_params_bg(sScanCfgParamsBG &params, size_t &result_size)
    {
        ssize_t received_result_size =
            dwpal_nl_cmd_get(m_radio_info.iface_name, LTQ_NL80211_VENDOR_SUBCMD_GET_SCAN_PARAMS_BG,
                             m_nl_buffer, NL_MAX_REPLY_BUFFSIZE);
        if (received_result_size <= 0) {
            LOG(ERROR) << "LTQ_NL80211_VENDOR_SUBCMD_GET_SCAN_PARAMS_BG failed!"
                       << " received size <= 0";
            result_size = ScanCfgParams_size_invalid;
            return false;
        }
        result_size = received_result_size;
        if (result_size < ScanCfgParamsBG_min_size) {
            LOG(ERROR) << "LTQ_NL80211_VENDOR_SUBCMD_GET_SCAN_PARAMS_BG failed!"
                       << " expected minimal size = " << ScanCfgParamsBG_min_size
                       << ", received size = " << result_size;
            return false;
        }
        if (result_size > ScanCfgParamsBG_size) {
            LOG(ERROR) << "LTQ_NL80211_VENDOR_SUBCMD_GET_SCAN_PARAMS_BG failed!"
                       << " expected maximal size = " << ScanCfgParamsBG_size
                       << ", received size = " << result_size;
            return false;
        }

        std::copy_n(m_nl_buffer, result_size, reinterpret_cast<unsigned char *>(&params));
        return true;
    }

    bool dwpal_set_scan_params_fg(const sScanCfgParams &params, const size_t &size)
    {
        if (!dwpal_nl_cmd_set(m_radio_info.iface_name, LTQ_NL80211_VENDOR_SUBCMD_SET_SCAN_PARAMS,
                              &params, size)) {
            LOG(ERROR) << __func__ << " LTQ_NL80211_VENDOR_SUBCMD_SET_SCAN_PARAMS failed!";
            return false;
        }
        return true;
    }

    bool dwpal_set_scan_params_bg(const sScanCfgParamsBG &params, const size_t &size)
    {
        if (!dwpal_nl_cmd_set(m_radio_info.iface_name, LTQ_NL80211_VENDOR_SUBCMD_SET_SCAN_PARAMS_BG,
                              &params, size)) {
            LOG(ERROR) << __func__ << " LTQ_NL80211_VENDOR_SUBCMD_SET_SCAN_PARAMS_BG failed!";
            return false;
        }
        return true;
    }

    std::shared_ptr<char> m_temp_dwpal_value;
    // Unique sequence number for the scan result dump sequence
    uint32_t m_nl_seq = 0;
    // Flag indicating if we are currently in a dump sequence
    bool m_scan_dump_in_progress = false;
    // Flag indicating if a scan was triggered internally
    bool m_scan_was_triggered_internally = false;

    static constexpr int INVALID_VAP_ID = -1;

    std::set<int> m_completed_vaps;
    std::unordered_set<sMacAddr> m_handled_clients;
    sMacAddr m_prev_client_mac = beerocks::net::network_utils::ZERO_MAC;
    bool m_queried_first       = false;
    int m_vap_id_in_progress   = INVALID_VAP_ID;
};

} // namespace dwpal
} // namespace bwl

#endif // _BWL_MON_WLAN_HAL_DWPAL_H_
