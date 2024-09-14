/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */
#ifndef _AGENT_DB_H_
#define _AGENT_DB_H_

#include "cac_capabilities.h"
#include "cac_status_interface.h"
#include "tasks/task_messages.h"
#include <bcl/beerocks_defines.h>
#include <bcl/network/network_utils.h>
#include <beerocks/tlvf/beerocks_message.h>
#include <beerocks/tlvf/enums/eDfsState.h>
#include <beerocks/tlvf/structs/sSupportedBandwidth.h>
#include <bwl/sta_wlan_hal.h>
#include <memory>
#include <mutex>
#include <tlvf/wfa_map/tlvProfile2ApCapability.h>
#include <tlvf/wfa_map/tlvProfile2MultiApProfile.h>
#include <tlvf/wfa_map/tlvServicePrioritizationRule.h>
#include <unordered_set>

#ifdef ENABLE_NBAPI
#include "ambiorix_impl.h"

#else
#include "ambiorix_dummy.h"

#endif // ENABLE_NBAPI

namespace beerocks {

/**
 * @brief AgentDB is a class used by all main Agent threads on the Agent process to store the Agent
 * common data.
 *
 * The class is implemented using singleton design pattern.
 * It is thread safe and being locked and released automatically.
 *
 * How to use:
 * @code
 * auto db = AgentDB::get();            // Get DB and automatically lock it. The lock will be
 *                                      // Released when 'db' will be destructed.
 *
 * db->foo = 42;                        // Change database member 'foo' to value 42.
 * db->bar = "prpl"                     // Set another database member.
 * auto &my_foo = db->foo;              // Get a refernce to a member.
 *
 * AgentDB::get()->foo = 44;            // Set database member directly.
 *
 *
 * Unsafe operation which should never be done:
 * auto my_foo = AgentDB::get()->foo;   // Unsafe! Don't do it! The database might be unlocked,
 *                                      // if used on ranged loop. So better not use it.
 *
 * auto &foo = AgentDB::get()->foo;     // Unsafe! Don't do it! The database will be unlocked,
 *                                      // but the reference will remain.
 *
 * std::string &get_foo() {             // Unsafe! Don't do it! Returning refernce to the database
 *     auto db = AgentDB::get();        // on a wrapper function is unsafe because the database will
 *     return db->foo;                  // be unlocked when the function ends, and the caller will
 * }                                    // hold a refernce to it.
 * @endcode
 */
class AgentDB {
public:
    class SafeDB {
    public:
        explicit SafeDB(AgentDB &db) : m_db(db) { m_db.db_lock(); }
        ~SafeDB() { m_db.db_unlock(); }
        AgentDB *operator->() { return &m_db; }

    private:
        AgentDB &m_db;
    };
    static SafeDB get()
    {
        // Guaranteed to be destroyed.
        // Instantiated on first use.
        static AgentDB instance;
        return SafeDB(instance);
    }
    AgentDB(const AgentDB &) = delete;
    void operator=(const AgentDB &) = delete;

private:
    // Private constructor so that no objects can be created.
    AgentDB() = default;
    std::recursive_mutex m_db_mutex;
    void db_lock() { m_db_mutex.lock(); }
    void db_unlock() { m_db_mutex.unlock(); }
    std::shared_ptr<beerocks::nbapi::Ambiorix> m_ambiorix_datamodel{};

    /* Put down from here database members and functions used by the Agent modules */

public:
    /* Agent Configuration */
    struct sDeviceConf {
        struct sFrontRadio {
            char ssid[beerocks::message::WIFI_SSID_MAX_LENGTH];
            char pass[beerocks::message::WIFI_PASS_MAX_LENGTH];
            bwl::WiFiSec security_type;

            /* Front radio wlan settings */
            struct sWlanSettings {
                bool band_enabled;
                // Front radio configured channel, if 0 auto channel selection.
                uint8_t configured_channel;
                bool sub_band_dfs;
            };

            // Wlan settings mapped by front interface name.
            std::unordered_map<std::string, sWlanSettings> config;
        } front_radio;

        struct sBackRadio {
            std::string ssid;
            std::string pass;
            bwl::WiFiSec security_type;

            bool mem_only_psk;
            uint8_t backhaul_max_vaps;
            bool backhaul_network_enabled;
            beerocks::eFreqType backhaul_preferred_radio_band;
        } back_radio;

        bool local_gw;
        bool local_controller;
        uint8_t operating_mode;
        uint8_t management_mode;
        bool certification_mode;
        uint8_t stop_on_failure_attempts;

        bool client_band_steering_enabled;
        bool client_optimal_path_roaming_enabled;
        bool client_optimal_path_roaming_prefer_signal_strength_enabled;
        bool client_11k_roaming_enabled;
        bool load_balancing_enabled;
        bool service_fairness_enabled;
        bool rdkb_extensions_enabled;
        bool zwdfs_enable;
        uint32_t best_channel_rank_threshold;

        std::string vendor;
        std::string model;
        std::string software_version;
        std::string operating_system;
        std::string device_serial_number;
        uint16_t ucc_listener_port;
        CountryCode country_code;
        wfa_map::tlvProfile2ApCapability::eByteCounterUnits byte_counter_units;
        uint32_t max_prioritization_rules;
    } device_conf;

    struct sControllerInfo {
        bool prplmesh_controller;
        sMacAddr bridge_mac;
        wfa_map::tlvProfile2MultiApProfile::eMultiApProfile profile_support =
            wfa_map::tlvProfile2MultiApProfile::eMultiApProfile::PRPLMESH_PROFILE_UNKNOWN;
    } controller_info;

    struct sStatus {
        bool ap_autoconfiguration_completed;
        uint32_t zwdfs_cac_remaining_time_sec;
    } statuses;

    /**
     * Agent Sub Entities Data
     */
    struct sBridge {
        sMacAddr mac;
        std::string iface_name;
    } bridge;

    struct sBackhaul {
        enum class eConnectionType { Invalid = 0, Wired, Wireless } connection_type;
        std::string selected_iface_name;
        sMacAddr preferred_bssid;
        uint8_t bssid_multi_ap_profile;
        sMacAddr backhaul_bssid;
    } backhaul;

    struct {
        struct sEthernetPort {
            explicit sEthernetPort(const std::string &iface_name_,
                                   const sMacAddr &mac_ = net::network_utils::ZERO_MAC)
                : iface_name(iface_name_), mac(mac_)
            {
            }
            sEthernetPort() : mac(net::network_utils::ZERO_MAC){};
            std::string iface_name;
            sMacAddr mac;
        } wan;
        std::vector<sEthernetPort> lan;
    } ethernet;

    struct sRadio {
        sRadio(const std::string &front_iface_name, const std::string &back_iface_name)
            : front(front_iface_name), back(back_iface_name)
        {
        }

        struct sFront {
            explicit sFront(const std::string &iface_name_)
                : iface_name(iface_name_), iface_mac(net::network_utils::ZERO_MAC)
            {
            }
            std::string iface_name;
            sMacAddr iface_mac;
            eWiFiBandwidth max_supported_bw;

            // When set, radio can only be used for ZWDFS purpose.
            bool zwdfs;
            bool hybrid_mode_supported;

            struct sBssid {
                sMacAddr mac;
                std::string ssid;
                bool fronthaul_bss;
                bool backhaul_bss;
                bool backhaul_bss_disallow_profile1_agent_association;
                bool backhaul_bss_disallow_profile2_agent_association;
            };
            std::array<sBssid, eBeeRocksIfaceIds::IFACE_TOTAL_VAPS> bssids{};
        } front;

        struct sBack {
            explicit sBack(const std::string &iface_name_)
                : iface_name(iface_name_), iface_mac(net::network_utils::ZERO_MAC)
            {
            }
            std::string iface_name;
            sMacAddr iface_mac;
        } back;

        struct sStatus {
            bool channel_scan_in_progress = false;
        } statuses;

        struct sClient {
            sClient(sMacAddr bssid_, size_t association_frame_length_, uint8_t *association_frame_)
                : bssid(bssid_), association_time(std::chrono::steady_clock::now()),
                  association_frame_length(association_frame_length_)
            {
                std::copy_n(association_frame_, association_frame_length_,
                            association_frame.begin());
            }
            sMacAddr bssid;
            std::chrono::steady_clock::time_point association_time;
            size_t association_frame_length;
            std::array<uint8_t, ASSOCIATION_FRAME_SIZE> association_frame;
        };

        struct sCacCapabilities {
            struct sCacMethodCapabilities {
                wfa_map::eCacMethod cac_method;
                uint32_t cac_duration_sec;
                CacCapabilities::CacOperatingClasses operating_classes;
            };
            // for each cac method - the capabilities for it
            std::map<wfa_map::eCacMethod, sCacMethodCapabilities> cac_method_capabilities;
        } cac_capabilities;

        struct sChannelInfo {
            int8_t tx_power_dbm;
            std::vector<beerocks_message::sSupportedBandwidth> supported_bw_list;
            beerocks_message::eDfsState dfs_state;
        };

        // Key: Channel
        std::unordered_map<uint8_t, sChannelInfo> channels_list;

        // Key: Channel number
        // Value: Pair containing:
        //              Timestamp of Scan request
        //              Vector of Neighboring APs as ChannelScanResults.
        std::unordered_map<uint8_t, std::pair<std::chrono::system_clock::time_point,
                                              std::vector<beerocks_message::sChannelScanResults>>>
            channel_scan_results;

        // Associated clients grouped by Client MAC.
        std::unordered_map<sMacAddr, sClient> associated_clients;

        bool sta_iface_filter_low;
        eFreqType freq_type             = eFreqType::FREQ_UNKNOWN;
        eWiFiBandwidth max_supported_bw = eWiFiBandwidth::BANDWIDTH_UNKNOWN;
        uint8_t number_of_antennas;
        uint8_t antenna_gain_dB;
        uint8_t tx_power_dB;
        uint8_t channel;
        beerocks::eWiFiBandwidth bandwidth;
        bool channel_ext_above_primary;
        uint16_t vht_center_frequency;

        bool ht_supported; ///< Is 802.11n (High Throughput) protocol supported
        uint8_t ht_capability;

        // 16-byte attribute containing the MCS set as defined in 802.11n
        std::array<uint8_t, beerocks::message::HT_MCS_SET_SIZE> ht_mcs_set;

        bool vht_supported; ///< Is 802.11ac (Very High Throughput) protocol supported
        uint16_t vht_capability;

        // 32-byte attribute containing the MCS set as defined in 802.11ac
        std::array<uint8_t, beerocks::message::VHT_MCS_SET_SIZE> vht_mcs_set;

        bool he_supported = false; ///< Is 802.11ax (High Efficiency) protocol supported
        uint16_t he_capability;

        // 32-byte attribute containing the MCS set as defined in 802.11ax
        std::array<uint8_t, beerocks::message::HE_MCS_SET_SIZE> he_mcs_set;

        bool report_indepent_scans_policy = false;

        // Information on the last channel switch request which containing the requested channel
        // and bandwidth.
        std::shared_ptr<sSwitchChannelRequest> last_switch_channel_request;

        std::string chipset_vendor;
    };
    struct {
        uint16_t max_number_of_vlans_ids;
        // Key: SSID, Value: VID
        std::unordered_map<std::string, uint16_t> ssid_vid_mapping;
        uint16_t primary_vlan_id;
        uint8_t default_pcp;
        std::unordered_set<uint16_t> secondary_vlans_ids;
    } traffic_separation;

    struct {
        // Key: rule ID
        std::unordered_map<uint32_t,
                           wfa_map::tlvServicePrioritizationRule::sServicePrioritizationRule>
            rules;
        std::array<uint8_t, 64> dscp_mapping_table;
    } service_prioritization;
    struct {
        uint32_t reporting_interval_sec;
        bool report_independent_channel_scans               = false;
        bool report_unsuccessful_associations               = false;
        uint32_t failed_associations_maximum_reporting_rate = 0;
        std::chrono::steady_clock::time_point failed_association_last_reporting_time_point =
            std::chrono::steady_clock::time_point::min();
        uint32_t number_of_reports_in_last_minute = 0;
    } link_metrics_policy;

    /**
     * @brief Get pointer to the radio data struct of a specific interface. The function can
     * accepts either front or back interface name.
     *
     * @param iface_name Interface name of a radio, front or back.
     * @return std::unique_ptr<sRadio> to the radio struct if exist, otherwise, nullptr.
     */
    sRadio *radio(const std::string &iface_name);

    /**
     * @brief Add radio node to the database. At least one of the input arguments must not be empty.
     * This function should be called only once with valid arguments. If called once and then called
     * again, the radio struct on the database will not be updated with new arguments, and the
     * function will return nullptr.
     *
     * @param front_iface_name Front interface name.
     * @param back_iface_name Back interface name.
     * @return true if a radio struct has been added to the database, otherwise return false.
     * @return pointer to the radio struct added to the database on success, nullptr otherwise.
     */
    sRadio *add_radio(const std::string &front_iface_name, const std::string &back_iface_name);

    /**
     * @brief Get list of all radio objects on the database.
     * This function shall be used in order to iterate over all the radios.
     *
     * @return const std::vector<std::string &>& Intefaces names list.
     */
    const std::vector<sRadio *> &get_radios_list() { return m_radios_list; };

    /**
     * @brief Removes a radio object from the radios list.
     * This function does not remove the radio completely from the database, only the pointer to
     * that radio, from the list which is get when calling "get_radios_list()".
     *
     * @param iface_name Front or back interface name.
     */
    void remove_radio_from_radios_list(const std::string &iface_name);

    /* Helper enum for get_radio_by_mac() function */
    enum class eMacType : uint8_t { ALL, RADIO, BSSID, CLIENT };

    /**
     * @brief Get a pointer to the parent radio struct of given MAC address.
     *
     * @param mac MAC address of radio interface or bssid.
     * @param mac_type_hint Hint for the MAC type, for faster lookup.
     * @return sRadio* A pointer to the radio struct containing the given MAC address.
     */
    sRadio *get_radio_by_mac(const sMacAddr &mac, eMacType mac_type_hint = eMacType::ALL);

    /**
     * @brief Erase client from associated_clients list.
     * If @a bssid is given, then remove client only from its radio, otherwise remove from all
     * radios.
     *
     * @param client_mac The client MAC address.
     * @param bssid The bssid that the client will be removed from its radio.
     */
    void erase_client(const sMacAddr &client_mac, sMacAddr bssid = net::network_utils::ZERO_MAC);

    /**
     * @brief Get the MAC address (or bssid) of an AP based on the ruid and ssid.
     *
     * @param[in] ruid The Radio UID.
     * @param[in] ssid The ssid of the AP.
     * @param[out] value The mac address/bssid if found, else an invalid MAC (zero).
     * @return true if the mac/bssid was found, false otherwise.
     */
    bool get_mac_by_ssid(const sMacAddr &ruid, const std::string &ssid, sMacAddr &value);

    /**
     * @brief Initialize Agent Data model.
     *
     * This method should be called in initialization state, otherwise data-model methods fail.
     *
     * @param dm Ambiorix shared ptr to access data model.
     * @return True if success otherwise false.
     */
    bool init_data_model(std::shared_ptr<beerocks::nbapi::Ambiorix> dm);

    /**
     * @brief Sets MACAddress to the Agent Data model.
     *
     * Path is: "Agent.MACAddress".
     *
     * @param mac MAC address for Bridge Interface.
     * @return True if success otherwise false.
     */
    bool dm_set_agent_mac(const std::string &mac);

    /**
     * @brief 1905.1 Neighbor device information
     * Information gathered from a neighbor device upon reception of a Topology Discovery message.
     */
    struct sNeighborDevice {
        // MAC address of the interface on which the Topology Discovery message is transmitted.
        sMacAddr transmitting_iface_mac;

        // Timestamp of the last Topology Discovery message received from this neighbor device.
        std::chrono::steady_clock::time_point timestamp;

        // Name of the network interface on which the neighbor has been seen
        std::string receiving_iface_name;
    };

    /**
     * @brief List of known 1905 neighbor devices.
     *
     * Upper key: Local interface MAC on which the Topology Discovery message was received from.
     * Upper value: Map containing 1905.1 device information ordered by neighbor device al_mac -
     *  Sub-key: 1905.1 AL MAC address of the Topology Discovery message transmitting device.
     *  Sub-value: 1905.1 device information.
     *
     * Devices are being added to the list when receiving a 1905.1 Topology Discovery message from
     * an unknown 1905.1 device. Every 1905.1 device shall send this message every 60 seconds, and
     * we update the time stamp in which the message is received.
     */
    std::unordered_map<sMacAddr, std::unordered_map<sMacAddr, sNeighborDevice>> neighbor_devices;

private:
    std::list<sRadio> m_radios;
    std::vector<sRadio *> m_radios_list;
};

} // namespace beerocks

#endif // _AGENT_DB_H_
