/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BWL_BASE_WLAN_HAL_NL80211_H_
#define _BWL_BASE_WLAN_HAL_NL80211_H_

#include "wpa_ctrl_client.h"
#include <bwl/base_wlan_hal.h>
#include <bwl/nl80211_client.h>

#include <bcl/beerocks_state_machine.h>

#include <chrono>
#include <list>
#include <memory>
#include <unordered_map>

// Forward declaration
struct nl_sock;
struct nl_msg;

namespace bwl {
namespace nl80211 {

enum class nl80211_fsm_state { Delay, Init, GetRadioInfo, Attach, Operational, Detach };
enum class nl80211_fsm_event { Attach, Detach };

/*!
 * Base class for the wav abstraction layer.
 * Read more about virtual inheritance: https://en.wikipedia.org/wiki/Virtual_inheritance
 */
class base_wlan_hal_nl80211
    : public virtual base_wlan_hal,
      protected beerocks::beerocks_fsm<nl80211_fsm_state, nl80211_fsm_event> {

    // Public types:
public:
    typedef std::unordered_map<std::string, std::string> parsed_obj_map_t;
    typedef std::list<parsed_obj_map_t> parsed_obj_listed_map_t;

    // Public methods
public:
    virtual ~base_wlan_hal_nl80211();

    virtual HALState attach(bool block = false) override;
    virtual bool detach() override;
    virtual bool ping() override;
    virtual bool refresh_radio_info() override;
    virtual bool refresh_vaps_info(int id) override;
    virtual bool process_ext_events(int fd = 0) override;
    virtual bool process_nl_events() override;
    virtual std::string get_radio_mac() override;

    /**
     * @brief Gets channel utilization.
     *
     * @see base_wlan_hal::get_channel_utilization
     *
     * This implementation gets channel utilization through NL80211.
     *
     * @param[out] channel_utilization Channel utilization value.
     *
     * @return True on success and false otherwise.
     */
    bool get_channel_utilization(uint8_t &channel_utilization) override;

    // Protected methods
protected:
    base_wlan_hal_nl80211(HALType type, const std::string &iface_name, hal_event_cb_t callback,
                          int wpa_ctrl_buffer_size, const hal_conf_t &hal_conf = {});

    // Process hostapd/wpa_supplicant event
    virtual bool process_nl80211_event(parsed_obj_map_t &event) = 0;

    bool set(const std::string &param, const std::string &value,
             int vap_id = beerocks::IFACE_RADIO_ID);

    // Send a message via WPA Control Interface
    // (default: Empty ifname will send msg to radio/MainBSS interface)
    bool wpa_ctrl_send_msg(const std::string &cmd, parsed_obj_map_t &reply,
                           const std::string &ifname = {});
    bool wpa_ctrl_send_msg(const std::string &cmd, parsed_obj_listed_map_t &reply,
                           const std::string &ifname = {});
    bool wpa_ctrl_send_msg(const std::string &cmd, char **reply, const std::string &ifname = {});
    bool wpa_ctrl_send_msg(const std::string &cmd, const std::string &ifname = {});

    virtual void send_ctrl_iface_cmd(std::string cmd); // HACK for development, to be removed

    // Send NL80211 message to vap interface
    // (default: Empty ifname will send msg to radio/MainBSS interface)
    bool send_nl80211_msg(uint8_t command, int flags,
                          std::function<bool(struct nl_msg *msg)> msg_create,
                          std::function<bool(struct nl_msg *msg)> msg_handle,
                          const std::string &ifName = {});

    std::unique_ptr<nl80211_client> m_nl80211_client;

    // Private data-members:
private:
    bool fsm_setup();

    // FSM State and Timeout
    nl80211_fsm_state m_last_attach_state = nl80211_fsm_state::Detach;
    std::chrono::steady_clock::time_point m_state_timeout;

    // Manager of WPA Control Interface Objects
    wpa_ctrl_client m_wpa_ctrl_client;

    // NL80211 Socket
    std::shared_ptr<struct nl_sock> m_nl80211_sock;
    int m_nl80211_id = 0;

    // map of network interface index of vap interfaces
    std::map<std::string, int> m_iface_index;

    // WPA Control Interface Communication Buffer
    std::shared_ptr<char> m_wpa_ctrl_buffer;
    size_t m_wpa_ctrl_buffer_size = 0;

    /**
     * @brief Register interface for WPA Control handling.
     * WPA Ctrl socket file path is:
     * - read from hal_conf for primary BSS.
     * - deduced (same directory) for secondary BSSs.
     *
     * @param[in] interface Interface name.
     *
     * @return True on success and false:
     * - BSS interface name not suffixed with the main interface name.
     * - BSS interface not to be monitored, in hal_conf.
     * - Wpa_ctrl client failed to add interface.
     */
    bool register_wpa_ctrl_interface(const std::string &interface);
};

} // namespace nl80211
} // namespace bwl

#endif // _BWL_BASE_WLAN_HAL_NL80211_H_
