/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BWL_BASE_WLAN_HAL_H_
#define _BWL_BASE_WLAN_HAL_H_

#include "base_802_11_defs.h"
#include "base_wlan_hal_types.h"

#include <bcl/beerocks_thread_safe_queue.h>
#include <bcl/son/son_wireless_utils.h>

#include <functional>
#include <memory>
#include <string>

namespace bwl {

// Allocate a char array wrapped in a shared_ptr
#define ALLOC_SMART_BUFFER(size)                                                                   \
    std::shared_ptr<char>(new char[size], [](char *obj) {                                          \
        if (obj)                                                                                   \
            delete[] obj;                                                                          \
    })

/*!
 * Base class for the WLAN hardware abstraction layer.
 */
class base_wlan_hal {

    // Public definitions
public:
    // Pair of event ID and payload pointer
    typedef std::pair<int, std::shared_ptr<void>> hal_event_t;
    typedef std::shared_ptr<hal_event_t> hal_event_ptr_t;
    typedef std::function<bool(hal_event_ptr_t)> hal_event_cb_t;

    // Public methods
public:
    virtual ~base_wlan_hal();

    /*!
     * Attach to the WLAN hardware/middleware.
     * 
     * Unless the "block" argument is set to "true", the implementation should
     * be non-blocking. If the internal implementation may block, the state
     * should be set to "Initializing" and the upper layer will call this method
     * again until the state will change to "Operational" or "Failed".
     *
     * @param [in] block Execute in non-blocking (default) or blocking mode.
     *
     * @return The state of the attach process.
     */
    virtual HALState attach(bool block = false) = 0;

    /*!
     * Detach from the WLAN hardware/middleware.
     *
     * @return true on success or false on error.
     */
    virtual bool detach() = 0;

    /*!
     * Refresh the internal radio (and VAPs) information structure 
     * with the latest values from the hardware.
     *
     * @return true on success or false on error.
     */
    virtual bool refresh_radio_info() = 0;

    /*!
     * Try to ping the control interface
     *
     * @return true on success or false on error.
     */
    virtual bool ping() = 0;

    /*!
     * Refresh the VAPs information structure on the 
     * internal radio information structure.
     *
     * @param [in] id vap to refresh, or all vaps if radio id.
     * 
     * @return true on success or false on error.
     */
    virtual bool refresh_vaps_info(int id = beerocks::IFACE_RADIO_ID) = 0;

    /*!
     * Process incoming events from the underlying hardware/middleware.
     * This method should be called if the file descriptor returned by
     * get_ext_events_fd() generated an event.
     *
     * @param [in] fd File descriptor receiving the incoming event.
     * The value (0) trigger polling for new events on all external fds.
     *
     * @return true on success or false on error.
     */
    virtual bool process_ext_events(int fd = 0) = 0;

    /*!
     * Process incoming nl events from the underlying hardware/middleware.
     * This method should be called if the file descriptor returned by
     * get_nl_events_fd() generated an event.
     *
     * @return true on success or false on error.
     */
    virtual bool process_nl_events() = 0;

    /*!
     * Process internal events (queued by the underlying hardware/middleware).
     * This method should be called if the file descriptor returned by
     * get_int_events_fd() generated an event.
     *
     * @return true on success or false on error.
     */
    virtual bool process_int_events();

    /**
     * @brief Gets channel utilization.
     *
     * The channel utilization is defined as the percentage of time, linearly scaled with 255
     * representing 100%, that the AP sensed the medium was busy. When more than one channel
     * is in use for the BSS, the channel utilization value is calculated only for the primary
     * channel.
     *
     * @param[out] channel_utilization Channel utilization value.
     *
     * @return True on success and false otherwise.
     */
    virtual bool get_channel_utilization(uint8_t &channel_utilization) = 0;

    /**
     * @brief Converts a string-based radio state to an eRadioState.
     */
    eRadioState radio_state_from_string(const std::string &state);

    /**
     * Check if the given event (by OpCode) should be filtered.
     * 
     * @return true if the event should be filtered, otherwise false.
     */
    bool is_filtered_event(const std::string &opcode);

    // Public getter methods:
public:
    /*!
     * Returns the type of the HAL instance.
     */
    HALType get_type() const { return (m_type); }

    /*!
     * Returns the current state.
     */
    HALState get_state() const { return (m_hal_state); }

    /*!
     * Returns indexed file descriptor to the external events queue, 0 is events
     * should processed synchronously (by directly calling the process method),
     * or -1 on error.
     * 
     * The returned file descriptor supports select(), poll() and epoll().
     */
    int get_ext_events_fd(size_t pos = 0) const
    {
        if (pos >= m_fds_ext_events.size()) {
            return -1;
        }
        return (m_fds_ext_events[pos]);
    }

    /*!
     * Returns a vector of external events file descriptors, where:
     * - Empty or only including 0 values, means that events should 
     * be processed synchronously (by directly calling the process method).
     * - only including -1 indicates error.
     *
     * The returned file descriptors support select(), poll() and epoll().
     */
    const std::vector<int> &get_ext_events_fds() const { return (m_fds_ext_events); }

    /*!
     * Returns a file descriptor to the internal events queue, or -1 on error.
     * The returned file descriptor supports select(), poll() and epoll().
     */
    int get_int_events_fd() const { return (m_fd_int_events); }

    /*!
     * Returns a file descriptor to the netlink events queue, or -1 on error.
     * The returned file descriptor supports select(), poll() and epoll().
     */
    int get_nl_events_fd() const { return (m_fd_nl_events); }

    /*!
     * Returns the interface name.
     */
    const std::string &get_iface_name() const { return (m_iface_name); }

    /*!
     * Returns the interface type
     */
    IfaceType get_iface_type() const { return (m_iface_type); }

    /*!
     * Return the radio information.
     */
    const RadioInfo &get_radio_info() const { return (m_radio_info); }

    /*!
     * Return HAL configuration.
     */
    const hal_conf_t &get_hal_conf() const { return (m_hal_conf); }

    /*!
     * Returns the Radio's main MAC address.
     */
    virtual std::string get_radio_mac() = 0;

    /*!
     * Returns vap id using vap bssid.
     */
    int get_vap_id_with_mac(const std::string &bssid) const
    {
        for (auto &it : m_radio_info.available_vaps) {
            if (it.second.mac == bssid) {
                return it.first;
            }
        }
        return beerocks::IFACE_ID_INVALID;
    }

    /*!
     * Returns vap id using vap interface name.
     */
    int get_vap_id_with_bss(const std::string &bss) const
    {
        for (auto &it : m_radio_info.available_vaps) {
            if (it.second.bss == bss) {
                return it.first;
            }
        }
        return beerocks::IFACE_ID_INVALID;
    }

    /*!
     * Returns true if vap_id exists.
     */
    bool check_vap_id(int vap_id) const
    {
        return (m_radio_info.available_vaps.find(vap_id) != m_radio_info.available_vaps.end());
    }

    // Protected methods
protected:
    /*!
     * Constructor.
     *
     * @param [in] type The type of the HAL.
     * @param [in] iface_name Interface name.
     * @param [in] iface_type Interface type.
     * @param [in] callback Callback for handling internal events.
     * @param [in] hal_conf HAL configuration.
     */
    base_wlan_hal(HALType type, std::string iface_name, IfaceType iface_type,
                  hal_event_cb_t callback, hal_conf_t hal_conf = {});

    /*!
     * Default constructor (for virtual inheritance)
     */
    base_wlan_hal() = default;

    /*!
     * Push a new (internal) event into the queue.
     * 
     * @param [in] event Event opcode.
     * @param [in] data Pointer to the event payload.
     * 
     * @return true on success of false on failure.
     */
    bool event_queue_push(int event, std::shared_ptr<void> data = {});

    /**
     * Create a management frame notification event.
     * 
     * @param [in] mgmt_frame Hex representation of the management frame.
     * 
     * @return Pointer to the event structure or null on failure.
     */
    std::shared_ptr<sMGMT_FRAME_NOTIFICATION>
    create_mgmt_frame_notification(const char *mgmt_frame);

    void calc_curr_traffic(uint64_t val, uint64_t &total, uint32_t &curr);

    /**
     * @brief Check if a given BSS is monitored or not.
     * If the hal_conf's monitored BSSIDs list is empty all BSSs are considered as "monitored".
     * 
     * @param bssid BSSID interface name.
     * @return True if the VAP should be monitored, false otherwise.
     */
    bool is_BSS_monitored(const std::string &bssid);

    /*!
     * set a parameter in the interface
     *
     * @return true on success or false on error.
     */
    virtual bool set(const std::string &param, const std::string &value, int vap_id) = 0;

    // Protected data-members:
protected:
    RadioInfo m_radio_info;

    HALState m_hal_state = HALState::Uninitialized;

    std::vector<int> m_fds_ext_events = {-1};

    int m_fd_nl_events = -1;

    hal_conf_t m_hal_conf;
    std::set<std::string> m_filtered_events;

    // Private data-members:
private:
    HALType m_type = HALType::Invalid;

    std::string m_iface_name;

    IfaceType m_iface_type = IfaceType::Unsupported;

    bool m_acs_enabled = false;

    int m_fd_int_events = -1;

    hal_event_cb_t m_int_event_cb = nullptr;

    beerocks::thread_safe_queue<hal_event_ptr_t> m_queue_events;
};

} // namespace bwl

#endif // _BWL_AP_MANAGER_HAL_H_
