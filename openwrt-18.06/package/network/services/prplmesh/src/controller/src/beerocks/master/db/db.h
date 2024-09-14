/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _DB_H_
#define _DB_H_

#include "agent.h"
#include "node.h"
#include "station.h"

#include <bcl/beerocks_defines.h>
#include <bcl/beerocks_logging.h>
#include <bcl/network/network_utils.h>
#include <bcl/son/son_wireless_utils.h>
#include <bpl/bpl_board.h>

#include <tlvf/wfa_map/tlvApHeCapabilities.h>
#include <tlvf/wfa_map/tlvApHtCapabilities.h>
#include <tlvf/wfa_map/tlvApOperationalBSS.h>
#include <tlvf/wfa_map/tlvApRadioBasicCapabilities.h>
#include <tlvf/wfa_map/tlvApVhtCapabilities.h>
#include <tlvf/wfa_map/tlvAssociatedStaExtendedLinkMetrics.h>

#include <algorithm>
#include <mutex>
#include <queue>
#include <vector>

#ifdef ENABLE_NBAPI
#define NBAPI_ON 1
#include "ambiorix_impl.h"

#else
#include "ambiorix_dummy.h"
#define NBAPI_ON 0
#endif // ENABLE_NBAPI

using namespace beerocks_message;

using Agent   = prplmesh::controller::db::Agent;
using Station = prplmesh::controller::db::Station;

namespace son {

// Forward declaration for Controller context saving
class Controller;

class db {

    /*
        * none of the functions are thread-safe
        * code that uses database should be wrapped with calls
        * to lock() and unlock()
        */

    typedef struct {
        int sd;
        bool map_updates;
        bool stats_updates;
        bool events_updates;
        bool topology_updates;
    } sBmlListener;

public:
    /**
     * @brief An unordered map of parameters and their values.
     */
    using ValuesMap = std::unordered_map<std::string, std::string>;

    /**
     * @brief Client parameter names.
     * The parameter names can be used to set/get multiple parameters in one-shot.
     * This is done using key-value map (where key is the param name and value is it value)
     */
    static const std::string TIMESTAMP_STR;
    static const std::string TIMELIFE_DELAY_STR;
    static const std::string INITIAL_RADIO_ENABLE_STR;
    static const std::string INITIAL_RADIO_STR;
    static const std::string SELECTED_BANDS_STR;
    static const std::string IS_UNFRIENDLY_STR;

    // VAPs info list type
    typedef std::list<std::shared_ptr<beerocks_message::sConfigVapInfo>> vaps_list_t;

    typedef struct {
        std::string vendor;
        std::string model;
        std::string load_steer_on_vaps;
        std::vector<uint8_t> global_restricted_channels;
        int ucc_listener_port;
        int diagnostics_measurements_polling_rate_sec;
        int ire_rssi_report_rate_sec;
        bool load_dfs_reentry;
        bool load_rdkb_extensions;
        bool load_client_band_steering;
        bool load_client_optimal_path_roaming;
        bool load_client_11k_roaming;
        bool load_legacy_client_roaming;
        bool load_ire_roaming;
        bool load_load_balancing;
        bool load_service_fairness;
        bool load_diagnostics_measurements;
        bool load_backhaul_measurements;
        bool load_front_measurements;
        bool load_health_check;
        bool load_monitor_on_vaps;
        bool certification_mode;
        bool persistent_db;
        int persistent_db_aging_interval;
        int roaming_5ghz_failed_attemps_threshold;
        int roaming_24ghz_failed_attemps_threshold;
        int roaming_11v_failed_attemps_threshold;
        int roaming_hysteresis_percent_bonus;
        int roaming_unconnected_client_rssi_compensation_db;
        int roaming_hop_percent_penalty;
        int roaming_band_pathloss_delta_db;
        int roaming_rssi_cutoff_db;
        int monitor_total_ch_load_notification_hi_th_percent;
        int monitor_total_ch_load_notification_lo_th_percent;
        int monitor_total_ch_load_notification_delta_th_percent;
        int monitor_min_active_clients;
        int monitor_active_client_th;
        int monitor_client_load_notification_delta_th_percent;
        int monitor_rx_rssi_notification_threshold_dbm;
        int monitor_rx_rssi_notification_delta_db;
        int monitor_ap_idle_threshold_B;
        int monitor_ap_active_threshold_B;
        int monitor_ap_idle_stable_time_sec;
        int monitor_disable_initiative_arp;

        int idle_steer_activity_check_timeout;
        int channel_selection_random_delay;
        int fail_safe_5G_frequency;
        int fail_safe_5G_bw;
        int fail_safe_5G_vht_frequency;
        int channel_selection_long_delay;
        int credentials_change_timeout_sec;
        int blacklist_channel_remove_timeout;
        int failed_roaming_counter_threshold;
        int roaming_sticky_client_rssi_threshold;
        int clients_persistent_db_max_size;
        size_t steer_history_persistent_db_max_size;
        int max_timelife_delay_minutes;
        int unfriendly_device_max_timelife_delay_minutes;
        unsigned int persistent_db_commit_changes_interval_seconds;
        std::chrono::seconds link_metrics_request_interval_seconds;
        std::chrono::seconds dhcp_monitor_interval_seconds;
        std::chrono::milliseconds steering_disassoc_timer_msec;
        int management_mode;
        bool unsuccessful_assoc_report_policy;
        int unsuccessful_assoc_max_reporting_rate;
    } sDbMasterConfig;

    typedef struct {
        // Features
        bool enable_dfs_reentry          = true;
        bool client_band_steering        = true;
        bool client_optimal_path_roaming = true;
        bool client_11k_roaming          = true;
        bool legacy_client_roaming       = true;

        bool ire_roaming = true;

        bool load_balancing = false;

        bool diagnostics_measurements = true;
        bool backhaul_measurements    = true;
        bool front_measurements       = true;
        bool monitor_on_vaps          = true;

        bool health_check = true;

        bool service_fairness = false;

        bool rdkb_extensions = false;

        // Params
        bool client_optimal_path_roaming_prefer_signal_strength = false;
    } sDbMasterSettings;

    /**
     * @brief Avaliable configuration parameters in NBAPI.
     *
     * This struct is subset of sDbMasterSettings.
     */
    typedef struct {
        bool client_band_steering;
        bool client_optimal_path_roaming;
        int roaming_hysteresis_percent_bonus;
        std::chrono::milliseconds steering_disassoc_timer_msec;
        std::chrono::seconds link_metrics_request_interval_seconds;
    } sDbNbapiConfig;

    typedef struct {
        uint64_t m_byte_sent            = 0;
        uint64_t m_byte_received        = 0;
        uint64_t m_packets_sent         = 0;
        uint64_t m_packets_received     = 0;
        uint32_t m_tx_packets_error     = 0;
        uint32_t m_rx_packets_error     = 0;
        uint32_t m_retransmission_count = 0;
    } sAssociatedStaTrafficStats;

    typedef struct {
        std::string dm_path; /**< data model path */
        sMacAddr original_bssid;
        sMacAddr target_bssid;
        std::string trigger_event;
        std::string steering_approach;
        std::chrono::seconds duration = {};
        std::string timestamp;
    } sStaSteeringEvent;

    std::unordered_map<sMacAddr, std::vector<sStaSteeringEvent>> m_stations_steering_events;

    beerocks::mac_map<Agent> m_agents;
    beerocks::mac_map<Station> m_stations;

    db(sDbMasterConfig &config_, beerocks::logging &logger_, const sMacAddr &local_bridge_mac,
       std::shared_ptr<beerocks::nbapi::Ambiorix> ambiorix_object)

        : config(config_), logger(logger_), m_local_bridge_mac(local_bridge_mac),
          m_ambiorix_datamodel(ambiorix_object)
    {
        settings.enable_dfs_reentry &= config_.load_dfs_reentry;
        settings.client_band_steering &= config_.load_client_band_steering;
        settings.client_optimal_path_roaming &= config_.load_client_optimal_path_roaming;
        settings.client_11k_roaming &= config_.load_client_11k_roaming;
        settings.legacy_client_roaming &= config_.load_legacy_client_roaming;
        settings.ire_roaming &= config_.load_ire_roaming;
        settings.load_balancing &= config_.load_load_balancing;
        settings.diagnostics_measurements &= config_.load_diagnostics_measurements;
        settings.backhaul_measurements &= config_.load_backhaul_measurements;
        settings.front_measurements &= config_.load_front_measurements;
        settings.monitor_on_vaps &= config_.load_monitor_on_vaps;
        settings.health_check &= config_.load_health_check;
        settings.service_fairness &= config_.load_service_fairness;
        settings.rdkb_extensions &= config_.load_rdkb_extensions;
    }
    ~db(){};

    //static

    static const int8_t TASK_ID_NOT_FOUND = -1;

    /**
     * @brief Get string representation of node type.
     *
     * @param type Type of a node.
     * @return std::string the string representation of the type.
     */
    static std::string type_to_string(beerocks::eType type);

    /**
     * @brief Get db entry from MAC address.
     *
     * @param mac MAC address of a client.
     * @return std::string the string representation of the MAC address with ':' replaced with '_' removed.
     * @return An empty string is returned on failure.
     */
    static std::string client_db_entry_from_mac(const sMacAddr &mac);

    /**
     * @brief Get client MAC address from db entry.
     *
     * @param db_entry Client entry name in persistent db.
     * @return sMacAddr MAC address of the client the db_entry is representing. On failure ZERO_MAC is returned.
     */
    static sMacAddr client_db_entry_to_mac(std::string db_entry);

    /**
     * @brief Get string representation of number of seconds in timestamp.
     *
     * @param timestamp A time-point.
     * @return std::string the string representation of the integer number of seconds in the timestamp.
     */
    static std::string
    timestamp_to_string_seconds(const std::chrono::system_clock::time_point timestamp);

    /**
     * @brief Translate an integer number of seconds to a timepoint.
     *
     * @param timestamp_sec Number of seconds in the timestamp.
     * @return std::chrono::system_clock::time_point a time-point representation of the number of seconds.
     */
    static std::chrono::system_clock::time_point timestamp_from_seconds(int timestamp_sec);

    /**
     * @brief Get index and instance path from full data model path. Simplifies ambiorix/nbapi calls.
     *
     * Data model levels are noted with '.', so method splits according to last dot.
     * Example: DM Path: Device.WiFi.DataElements.Network.Device.2.Interface.3
     * Returns: <instance, index> <Device.WiFi.DataElements.Network.Device.2.Interface, 3>
     *
     * @param dm_path Full data model path.
     * @return std::pair <std::string instance path, int index>
     */
    static std::pair<std::string, int> get_dm_index_from_path(const std::string &dm_path);

    /**
     * @brief Add plus one to value of specifed with param_name Data Model's parameter.
     *
     * @param obj_path Path to object in Data Model which holds parameter.
     * @param param_name Name of parameter, value of which will be increased by one.
     * Parameter type should be uint64_t.
     * @return true on success, false otherwise.
     */
    bool dm_uint64_param_one_up(const std::string &obj_path, const std::string &param_name);

    /**
     * @brief Get agent containing a specific radio
     *
     * If no radio with the given radio_uid exists, an error is logged (and nullptr returned).
     *
     * @param radio_uid Radio UID of the radio.
     * @return The Agent object, or nullptr if it doesn't exist.
     */
    std::shared_ptr<Agent> get_agent_by_radio_uid(const sMacAddr &radio_uid);

    /**
     * @brief Get radio on a specific agent
     *
     * If no agent with the given al_mac exists, an error is logged (and nullptr returned). If no
     * radio with the given UID exists on the agent, nullptr is returned without logging an error.
     *
     * @param al_mac AL-MAC address of the agent (usually source address of a CMDU).
     * @param radio_uid Radio UID of the radio.
     * @return The sRadio object, or nullptr if it doesn't exist.
     */
    std::shared_ptr<Agent::sRadio> get_radio(const sMacAddr &al_mac, const sMacAddr &radio_uid);

    /**
     * @brief Get radio with a specific Radio Unique Identifier (Radio UID)
     *
     * Searches all Agent objects for sRadio object with the given Radio UID.
     * If no radio with the given UID found, nullptr is returned and an error is logged.
     *
     * @param radio_uid Radio UID of the radio.
     * @return The sRadio object, or nullptr if it doesn't exist.
     */
    std::shared_ptr<Agent::sRadio> get_radio_by_uid(const sMacAddr &radio_uid);

    /**
     * @brief Finds and returns sBss with a specified BSSID.
     *
     * @param bssid BSSID of searched BSS.
     * @return The sBss object, or nullptr if it doesn't exist.
     */
    std::shared_ptr<Agent::sRadio::sBss> get_bss(const sMacAddr &bssid);

    /**
     * @brief Get radio containing a BSS with a specific BSSID
     * Searches all Agent objects for sRadio object containing a BSS with the given BSSID.
     * If no such radio found, nullptr is returned and an error is logged.
     *
     * @param bssid BSSID of one of BSSs of the radio.
     * @return The sRadio object, or nullptr if it doesn't exist.
     */
    std::shared_ptr<Agent::sRadio> get_radio_by_bssid(const sMacAddr &bssid);

    /**
     * @brief Get station with a specific MAC address.
     *
     * Searches all Station object to find one with the given MAC address.
     * If no station with the given MAC was found, nullptr is returned and an error is logged.
     *
     * @param mac MAC address of the station.
     * @return The Station object, or nullptr if it doesn't exist.
     */
    std::shared_ptr<Station> get_station(const sMacAddr &mac);

    //logger
    void set_log_level_state(const beerocks::eLogLevel &log_level, const bool &new_state);

    // General set/get
    bool has_node(const sMacAddr &mac);

    bool add_virtual_node(const sMacAddr &mac, const sMacAddr &real_node_mac);

    /**
     * @brief Gets the remaining timelife of a client
     *
     * @param client The pair of the client and its variables associated with a key.
     * @return Returns the remaining life duration of a client.
     */
    uint64_t get_client_remaining_sec(const std::pair<std::string, ValuesMap> &client);
    /**
     * @brief A wrapper to add_node (to nodelist)
     *
     * @param client_entry A special identifier of a client.
     * @param ValuesMap The client information: timestamp, friendly status.
     * @param [out] results An error results for the persistent function report.
     */
    void add_node_from_data(const std::string &client_entry, const ValuesMap &values_map,
                            std::pair<uint16_t, uint16_t> &results);

    /**
     * @brief add instance of data element 'radio'
     *
     * @param mac address of radio
     * @param mac address of device
     * @return data model path if radio instance was successfully added, empty string otherwise
     */
    std::string dm_add_radio_element(const sMacAddr &radio_mac, const sMacAddr &device_mac);

    /**
    * @brief Add instance of data element 'MeasurementReport'.
    *
    * @param beacon_response Measurement Report elements.
    * @return True on success, false otherwise.
    */
    bool dm_add_sta_beacon_measurement(const beerocks_message::sBeaconResponse11k &beacon_response);

    /**
    * @brief Calculate value for TR-181* declares Noise parameter of MultiAPSTA object as: An indicator
    * of the average radio noise plus interference power measured on
    * the uplink from the Associated Device (STA) to the Access Point (AP).
    * Encoded as defined for ANPI in [Section 11.11.9.4/802.11].

    * It's possible to get value for ANPI with RCPI and RSNI based on formuladescribed
    * in 9.4.2.41 RSNI element of [Section 9.4.2.41 RSNI element/802.11].
    * RSNI = (10 * log ((RCPI – ANPI) / ANPI) + 10) * 2;
    * From RSNI formula possible to get ANPI.
    * ANPI = RCPI/(1 + 10^((RSNI / 20.00) - 1))
    *
    * @param station Station object.
    * @param rcpi RCPI.
    * @param rsni RSNI.
    * @return True on success, false otherwise.
    */
    bool dm_set_multi_ap_sta_noise_param(Station &station, const uint8_t rcpi, const uint8_t rsni);

    /**
     * @brief add gateway node and Agent object.
     *
     * Adds a gateway node and an Agent object if they don't exist.
     *
     * @param mac AL MAC of the gateway.
     * @return the existing Agent if it was already there or the newly added Agent otherwise.
     */
    std::shared_ptr<Agent> add_node_gateway(const sMacAddr &mac);

    /**
     * @brief add IRE node and Agent object.
     *
     * Adds an IRE node and an Agent object if they don't exist.
     *
     * @param mac AL MAC of the gateway.
     * @param parent_mac MAC address of the parent node in the legacy node structure.
     * @return the existing Agent if it was already there or the newly added Agent otherwise.
     */
    std::shared_ptr<Agent>
    add_node_ire(const sMacAddr &mac,
                 const sMacAddr &parent_mac = beerocks::net::network_utils::ZERO_MAC);

    /**
     * @brief add wireless backhaul node and Station object.
     *
     * Adds a wireless backhaul node and a Station object if they don't exist.
     *
     * @param mac MAC address of the wireless backhaul station.
     * @param parent_mac MAC address of the parent node in the legacy node structure.
     * @return the existing Station if it was already there or the newly added Station otherwise.
     */
    std::shared_ptr<Station>
    add_node_wireless_backhaul(const sMacAddr &mac,
                               const sMacAddr &parent_mac = beerocks::net::network_utils::ZERO_MAC);
    bool
    add_node_wired_backhaul(const sMacAddr &mac,
                            const sMacAddr &parent_mac = beerocks::net::network_utils::ZERO_MAC);
    bool add_node_radio(const sMacAddr &mac,
                        const sMacAddr &parent_mac = beerocks::net::network_utils::ZERO_MAC);

    /**
     * @brief add client node and Station object.
     *
     * Adds a station node and a Station object if they don't exist.
     *
     * @param mac MAC address of the client.
     * @param parent_mac MAC address of the parent node in the legacy node structure.
     * @return the existing Station if it was already there or the newly added Station otherwise.
     */
    std::shared_ptr<Station>
    add_node_station(const sMacAddr &mac,
                     const sMacAddr &parent_mac = beerocks::net::network_utils::ZERO_MAC);

    bool remove_node(const sMacAddr &mac);

    /**
     * @brief Removes optional subobjects: HTCapabilities, VHTCapabilities,
     * HECapabilities for Capabilities data model.
     * Example of path to object: "Device.WiFi.DataElements.Network.Device.1.Radio.1.Capabilities".
     *
     * @param radio_mac Radio mac for finding path to appropriate 'Capabilities' data element.
     * @return True if subobject was successfuly removed, false otherwise.
     */
    bool clear_ap_capabilities(const sMacAddr &radio_mac);

    bool set_node_type(const std::string &mac, beerocks::eType type);
    beerocks::eType get_node_type(const std::string &mac);

    bool set_node_ipv4(const std::string &mac, const std::string &ipv4 = std::string());
    std::string get_node_ipv4(const std::string &mac);

    bool set_node_link_rssi(const std::string &mac, int8_t link_rssi);
    int8_t get_node_link_rssi(const std::string &mac);

    bool set_node_manufacturer(const std::string &mac, const std::string &manufacturer);
    bool set_agent_manufacturer(prplmesh::controller::db::Agent &agent,
                                const std::string &manufacturer);

    int get_node_channel(const std::string &mac);

    int get_hostap_operating_class(const sMacAddr &mac);

    bool set_node_vap_id(const std::string &mac, int8_t vap_id);
    int8_t get_node_vap_id(const std::string &mac);

    bool set_node_beacon_measurement_support_level(
        const std::string &mac,
        beerocks::eBeaconMeasurementSupportLevel support_beacon_measurement);
    beerocks::eBeaconMeasurementSupportLevel
    get_node_beacon_measurement_support_level(const std::string &mac);

    bool set_node_name(const std::string &mac, const std::string &name);

    bool set_node_state(const std::string &mac, beerocks::eNodeState state);
    beerocks::eNodeState get_node_state(const std::string &mac);

    std::chrono::steady_clock::time_point get_last_state_change(const std::string &mac);

    bool set_node_handoff_flag(Station &station, bool handoff);
    bool get_node_handoff_flag(const Station &station);

    bool update_node_last_seen(const std::string &mac);

    std::chrono::steady_clock::time_point get_node_last_seen(const std::string &mac);

    bool set_hostap_active(const sMacAddr &mac, bool active);
    bool is_hostap_active(const sMacAddr &mac);

    bool is_ap_out_of_band(const std::string &mac, const std::string &sta_mac);

    bool is_node_wireless(const std::string &mac);

    std::string node_to_string(const std::string &mac);

    /**
     * @brief Get the link metric database
     * @return reference to the map that holds link metric data of all agents.
     */
    std::unordered_map<sMacAddr, std::unordered_map<sMacAddr, son::node::link_metrics_data>> &
    get_link_metric_data_map();

    /**
     * @brief Get the ap metric database
     * @return reference to the map that holds ap metric data of all agents.
     */
    std::unordered_map<sMacAddr, son::node::ap_metrics_data> &get_ap_metric_data_map();

    /**
     * @brief Add Current Operating Class to the Device.WiFi.DataElements Data model.
     *        Data model path example: "Device.WiFi.DataElements.Network.Device.1.Radio.1.CurrentOperatingClasses".
     *
     * @param[in] radio_mac MAC address for Radio which reporting Operating Class.
     * @param[in] op_class Current operating class.
     * @param[in] op_channel Current channel number.
     * @param[in] tx_power Current Transmit power.
     * @return True if success otherwise false.
     */
    bool add_current_op_class(const sMacAddr &radio_mac, uint8_t op_class, uint8_t op_channel,
                              int8_t tx_power);

    /**
     * @brief Removes all CurrentOperatingClasses instances from the Data Model.
     *
     * @param[in] radio_mac MAC address for Radio which reporting Operating Class
     * @return true on success and false otherwise.
     */
    bool remove_current_op_classes(const sMacAddr &radio_mac);

    /**
     * @brief Removes all instances of hostap supported operating classes
	 * from the Data Model and database.
     * Path example: Device.WiFi.DataElements.Network.Device.1.Radio.1.Capabilities.OperatingClasses
     *
     * @param radio_mac MAC address for Radio which reporting Operating Class
     * @return true on success, false otherwise.
     */
    bool remove_hostap_supported_operating_classes(const sMacAddr &radio_mac);

    /**
     * @brief Adds Interface Object and updates Interface Data Model Object.
     *
     * If instance with @a interface_mac exists, updates it, otherwise add it.
     * Path example: Device.WiFi.DataElements.Network.Device.1.Interface.1
     *
     * @param device_mac device MAC address for node matching
     * @param interface_mac interface mac address
     * @param media_type Media type, as per IEEE1905.1 table 6-12
     * @param status current operational state of the interface
     * @param name per-device unique and unchanging name for the interface. if unavailable,
     * use MAC address or linux interface name
     * @return true on success, false otherwise.
     */
    bool add_interface(const sMacAddr &device_mac, const sMacAddr &interface_mac,
                       uint16_t media_type, const std::string &status = "Up",
                       const std::string &name = {});

    /**
     * @brief Gets Interface Node according to device and interface MAC addresses.
     *
     * @param device_mac device MAC address for node matching
     * @param interface_mac interface mac address for node matching
     * @return returns node shared pointer.
     */
    std::shared_ptr<prplmesh::controller::db::Interface>
    get_interface_node(const sMacAddr &device_mac, const sMacAddr &interface_mac);

    /**
     * @brief Adds interface instances to Device's Data Model.
     *
     * If instance with @a interface_mac exists, updates it, otherwise add it.
     * Path example: Device.WiFi.DataElements.Network.Device.1.Interface.1
     *
     * @param device_mac device MAC address for node matching
     * @param interface_mac interface mac address
     * @param media_type Media type, as per IEEE1905.1 table 6-12
     * @param status current operational state of the interface
     * @param name per-device unique and unchanging name for the interface. if unavailable,
     * use MAC address or linux interface name
     * @return true on success, false otherwise.
     */
    bool dm_add_interface_element(const sMacAddr &device_mac, const sMacAddr &interface_mac,
                                  uint16_t media_type, const std::string &status,
                                  const std::string &name);

    /**
     * @brief Returns ambiorix object.
     *
     * @return Instance of AmbiorixImpl class, or AmbiorixDummy (if dummy mode enabled).
     */
    std::shared_ptr<beerocks::nbapi::Ambiorix> get_ambiorix_obj();

    /**
     * @brief Removes the interface of given MAC of Interface Class and Device's Data Model.
     *
     * Searches index of m_interfaces vector and removes it.
     * After that data model remove method is called within (dm_remove_interface_element).
     *
     * @param device_mac device MAC address for node matching
     * @param interface_mac interface mac to be deleted
     * @return true on success, false otherwise.
     */
    bool remove_interface(const sMacAddr &device_mac, const sMacAddr &interface_mac);

    /**
     * @brief Removes the interface of given MAC from Device's Data Model.
     *
     * Searches index of Device.WiFi.DataElements.Network.Device.{i}.Interface.{i} according
     * to MACAddress attribute and removes it.
     * Path example: Device.WiFi.DataElements.Network.Device.1.Interface.1.MACAddress
     *
     * @param device_mac device MAC address for node matching
     * @param interface_mac interface mac to be deleted
     * @return true on success, false otherwise.
     */
    bool dm_remove_interface_element(const sMacAddr &device_mac, const sMacAddr &interface_mac);

    /**
     * @brief Updates the node interface mac list.
     * Removes unused intarfaces from Device's Data model.
     *
     * @param device_mac device MAC address for node matching
     * @param interface_macs Interface MAC addresses of the device
     * @return true on success, false otherwise.
     */
    bool dm_update_interface_elements(const sMacAddr &device_mac,
                                      const std::vector<sMacAddr> &interface_macs);

    /**
     * @brief Updates Tx Parameters of the Interface Stats.
     *
     * Path: Device.WiFi.DataElements.Network.Device.{i}.Interface.{i}.Stats
     *
     * @param device_mac device MAC address for node matching
     * @param interface_mac Interface MAC address of the device
     * @param packets_sent send packets counter
     * @param errors_sent send error counter
     * @return true on success, false otherwise.
     */
    bool dm_update_interface_tx_stats(const sMacAddr &device_mac, const sMacAddr &interface_mac,
                                      uint64_t packets_sent, uint32_t errors_sent);

    /**
     * @brief Updates Rx Parameters of the Interface Stats.
     *
     * Path: Device.WiFi.DataElements.Network.Device.{i}.Interface.{i}.Stats
     *
     * @param device_mac device MAC address for node matching
     * @param interface_mac Interface MAC address of the device
     * @param packets_received received packets counter
     * @param errors_received receive error counter
     * @return true on success, false otherwise.
     */
    bool dm_update_interface_rx_stats(const sMacAddr &device_mac, const sMacAddr &interface_mac,
                                      uint64_t packets_received, uint32_t errors_received);

    /**
     * @brief Adds to data model Scan Result and fill up data for its parameters.
     *
     * @param ruid Radio unique identifier.
     * @param operating_class Scanned operating class.
     * @param channel Scanned channel.
     * @param noise Channel noise.
     * @param utilization Channel utilization.
     * @param neighbors List of discovered neighbors.
     * @param ISO_8601_timestamp Channel scan report's timestamp.
     * @return True on success, false otherwise.
     */
    bool dm_add_scan_result(const sMacAddr &ruid, const uint8_t &operating_class,
                            const uint8_t &channel, const uint8_t noise, const uint8_t utilization,
                            const std::vector<wfa_map::cNeighbors> &neighbors,
                            const std::string &ISO_8601_timestamp);

    /** @brief Set 'Status' parameter of NBAPI 'SteerEvent' object.
     *
     * @param event_path Path to NBAPI 'SteerEvent' object.
     * @param status_code Status code of client steering.
     */
    void dm_set_status(const std::string &event_path, const uint8_t status_code);

    /**
     * @brief Adds NBAPI SteerEvent object.
     * Data model path example: "Device.WiFi.DataElements.SteerEvent.42"
     *
     * @return Path to object on success, empty string otherwise.
     */
    std::string dm_add_steer_event();

    /**
     * @brief Set values for parameters of NBAPI object MultiAPSTA.SteeringSummaryStats.
     *
     * @param station Station object.
     * @return True on success, false otherwise.
     */
    bool dm_restore_steering_summary_stats(Station &station);

    /**
     * @brief Update global steering summary statistics for one parameter
     *
     * @param param_name Name of parameter, value of which will be increased by one.
     */
    void dm_increment_steer_summary_stats(const std::string &param_name);

    /**
     * @brief Adds or updates instance of Neighbor inside Interface object.
     *
     * Path: Device.WiFi.DataElements.Network.Device.{i}.Interface.{i}.Neighbor.{i}
     *
     * @param device_mac device MAC address for node matching
     * @param interface_mac Interface MAC address of the device
     * @param neighbor_mac Neighbor MAC address is connected to Interface
     * @param is_IEEE1905 flag which identify neighbor is IEEE1905 device or not
     * @return true on success, false otherwise.
     */
    bool add_neighbor(const sMacAddr &device_mac, const sMacAddr &interface_mac,
                      const sMacAddr &neighbor_mac, bool is_IEEE1905);

    /**
     * @brief Adds or updates instance of Neighbor inside Interface Data Model.
     *
     * Path: Device.WiFi.DataElements.Network.Device.{i}.Interface.{i}.Neighbor.{i}
     *
     * @param interface Interface object that Neighbor relates to
     * @param neighbor Neighbor object is used to create/update data model of neighbor
     * @return true on success, false otherwise.
     */
    bool dm_add_interface_neighbor(
        const std::shared_ptr<prplmesh::controller::db::Interface> &interface,
        std::shared_ptr<prplmesh::controller::db::Interface::sNeighbor> &neighbor);

    /**
     * @brief Remove instance of Neighbors inside Interface Data Model.
     *
     * Path: Device.WiFi.DataElements.Network.Device.{i}.Interface.{i}.Neighbor.{i}
     *
     * @param dm_path datamodel path of neighbor
     * @return true on success, false otherwise.
     */
    bool dm_remove_interface_neighbor(const std::string &dm_path);

    /**
     * @brief Sets Extended Link Metrics for corresponding STA.
     *
     * Path: Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}
     *
     * @param sta_mac sta MAC address for node matching
     * @param metrics extended metrics of associated sta
     * @return true on success, false otherwise.
     */
    bool dm_set_sta_extended_link_metrics(
        const sMacAddr &sta_mac,
        const wfa_map::tlvAssociatedStaExtendedLinkMetrics::sMetrics &metrics);

    /**
     * @brief Sets Traffic Stats for corresponding STA.
     *
     * Path: Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}
     *
     * @param sta_mac sta MAC address for node matching
     * @param stats stats of associated sta traffic
     * @return true on success, false otherwise.
     */
    bool dm_set_sta_traffic_stats(const sMacAddr &sta_mac, db::sAssociatedStaTrafficStats &stats);

    /**
     * @brief Clears all stats for corresponding STA.
     *
     * Path: Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}
     *
     * @param sta_mac sta MAC address for node matching
     * @return true on success, false otherwise.
     */
    bool dm_clear_sta_stats(const sMacAddr &sta_mac);

    /**
     * @brief Remove STA from datamodel with given Station object.
     *
     * Path: Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}
     *
     * @param station Station object
     * @return true on success, false otherwise.
     */
    bool dm_remove_sta(Station &station);

    /**
     * @brief Adds FailedConnectionEventData NBAPI object each time
     * when station failed to associate. Set values for parameters of this object.
     *
     * Data model path example:
     * "Device.WiFi.DataElements.FailedConnectionEvent.FailedConnectionEventData"
     *
     * @param sta_mac Client mac address.
     * @param reason_code Reason code of clients failed association.
     * @param status_code Status code of clients failed association.
     * @return True on success, false otherwise.
     */
    bool dm_add_failed_connection_event(const sMacAddr &sta_mac, const uint16_t reason_code,
                                        const uint16_t status_code);

    /**
     * @brief Adds station capabilities sub-objects into data model
     * under instance of object AssociationEventData.
     *
     * @param assoc_event_path Path to instantiated AssociationEvent object.
     * Example of full path to object:
     * 'Device.WiFi.DataElements.AssociationEvent.AssociationEventData.1'.
     * @param sta_cap Structure with station HT Capabilities.
     * @return True on success, false otherwise.
     */
    bool dm_add_assoc_event_sta_caps(const std::string &assoc_event_path,
                                     const beerocks::message::sRadioCapabilities &sta_cap);

    /**
     * @brief Set STA DHCPv4 lease information for both node and datamodel.
     *
     * Path: Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}
     *
     * @param sta_mac sta MAC address for node matching
     * @param host_name sta host name
     * @param ipv4_address sta ipv4 address given by dhcp
     * @return true on success, false otherwise.
     */
    bool set_sta_dhcp_v4_lease(const sMacAddr &sta_mac, const std::string &host_name,
                               const std::string &ipv4_address);

    /**
     * @brief Set STA DHCPv6 lease information for both node and datamodel.
     *
     * Path: Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}
     *
     * @param sta_mac sta MAC address for node matching
     * @param host_name sta host name
     * @param ipv6_address sta ipv6 address given by dhcp
     * @return true on success, false otherwise.
     */
    bool set_sta_dhcp_v6_lease(const sMacAddr &sta_mac, const std::string &host_name,
                               const std::string &ipv6_address);

    //
    // DB node functions (get only)
    //
    int get_node_hierarchy(const std::string &mac);
    std::set<std::string> get_nodes(int type = -1);
    std::set<std::string> get_active_hostaps();
    std::vector<std::shared_ptr<Agent>> get_all_connected_agents();
    std::set<std::string> get_nodes_from_hierarchy(int hierarchy, int type = -1);
    std::shared_ptr<Agent> get_gw();
    std::set<std::string> get_node_subtree(const std::string &mac);
    std::string get_node_parent(const std::string &mac);

    std::string get_node_previous_parent(const std::string &mac);
    sMacAddr get_node_parent_ire(const std::string &mac);
    std::string get_node_parent_backhaul(const std::string &mac);
    std::set<std::string> get_node_siblings(const std::string &mac, int type = beerocks::TYPE_ANY);
    std::set<std::string> get_node_children(const std::string &mac, int type = beerocks::TYPE_ANY,
                                            int state = beerocks::STATE_ANY);
    std::list<sMacAddr> get_1905_1_neighbors(const sMacAddr &al_mac);

    //
    // Capabilities
    //

    /**
     * @brief Add optional sub-object of AP HE Capabilities data element,
     * set values for its parameters.
     * Example of full path to object:
     * "Device.WiFi.DataElements.Netwok.Device.1.Radio.1.Capabilities.HECapabilities"
     *
     * @param he_caps_tlv TLV with AP HE Capabilities included in
     * 'AP Capability Report' message
     * @return True if sub-object was successfully added
     * and values for its parameters set, false otherwise.
     */
    bool set_ap_he_capabilities(wfa_map::tlvApHeCapabilities &he_caps_tlv);

    /**
     * @brief add 'HTCapabilities' data element, set values to its parameters.
     * Example of full path to object:
     * "Device.WiFi.DataElements.Netwok.Device.1.Radio.1.Capabilities.HTCapabilities"
     *
     * @param radio_mac mac address of radio
     * @param flags structure with AP HT Capabilities
     * @return true on success, false otherwise
     */
    bool set_ap_ht_capabilities(const sMacAddr &radio_mac,
                                const wfa_map::tlvApHtCapabilities::sFlags &flags);

    /**
     * @brief Add 'VHTCapabilities' data element, set values to its parameters.
     * Example of full path to object:
     * "Device.WiFi.DataElements.Netwok.Device.1.Radio.1.Capabilities.VHTCapabilities"
     *
     * @param vht_caps_tlv TLV with AP VHT Capabilities included in
     * 'AP Capability Report' message.
     * @return True if sub-object was successfully added
     * and values for its parameters set, false otherwise.
     */
    bool set_ap_vht_capabilities(wfa_map::tlvApVhtCapabilities &vht_caps_tlv);

    /**
     * @brief Set values for estimated MAC data rate downlink and uplink
     * for STA.EstMACDataRateDownlink and STA.EstMACDataRateUplink data elements.
     * Example of full path to data element:
     * 'Device.WiFi.DataElements.Network.Device.1.Radio.2.BSS.3.STA.4.EstMACDataRateUplink'.
     * Set value for station SignalStrength data element.
     * 'Device.WiFi.DataElements.Network.Device.1.Radio.2.BSS.1.STA.4.SignalStrength'.
     *
     * @param sta_mac Station MAC address.
     * @param downlink_est_mac_data_rate Estimated MAC Data Rate in downlink (in Mb/s).
     * @param uplink_est_mac_data_rate Estimated MAC Data Rate in uplink (in Mb/s).
     * @param signal_strength Indicator of radio signal strength (RCPI)
     * of the uplink from the Non-AP STA - measured in dBm.
     * @return True on success, false otherwise.
     */
    bool dm_set_sta_link_metrics(const sMacAddr &sta_mac, uint32_t downlink_est_mac_data_rate,
                                 uint32_t uplink_est_mac_data_rate, uint8_t signal_strength);

    const beerocks::message::sRadioCapabilities *
    get_station_current_capabilities(const std::string &mac);

    const beerocks::message::sRadioCapabilities *
    get_station_capabilities(const std::string &client_mac, bool is_bandtype_5ghz);
    bool set_station_capabilities(const std::string &client_mac,
                                  const beerocks::message::sRadioCapabilities &sta_cap);

    bool set_hostap_ant_num(const sMacAddr &mac, beerocks::eWiFiAntNum ant_num);
    beerocks::eWiFiAntNum get_hostap_ant_num(const sMacAddr &mac);

    bool set_hostap_ant_gain(const sMacAddr &al_mac, const sMacAddr &mac, int ant_gain);
    int get_hostap_ant_gain(const sMacAddr &mac);

    bool set_hostap_tx_power(const sMacAddr &al_mac, const sMacAddr &mac, int tx_power);
    int get_hostap_tx_power(const sMacAddr &mac);

    bool set_hostap_supported_channels(const sMacAddr &mac,
                                       beerocks::message::sWifiChannel *supported_channels,
                                       int length);
    std::vector<beerocks::message::sWifiChannel> get_hostap_supported_channels(const sMacAddr &mac);
    std::string get_hostap_supported_channels_string(const sMacAddr &radio_mac);

    bool add_hostap_supported_operating_class(const sMacAddr &radio_mac, uint8_t operating_class,
                                              uint8_t tx_power,
                                              const std::vector<uint8_t> &non_operable_channels);

    bool set_hostap_band_capability(const sMacAddr &al_mac, const sMacAddr &mac,
                                    beerocks::eRadioBandCapability capability);
    beerocks::eRadioBandCapability get_hostap_band_capability(const sMacAddr &mac);

    bool capability_check(const std::string &mac, int channel);

    bool get_node_5ghz_support(
        const std::string &mac); // TODO: add a real learning algorithm for per-channel support
    bool get_node_24ghz_support(const std::string &mac);
    bool is_node_5ghz(const std::string &mac);
    bool is_node_24ghz(const std::string &mac);
    bool update_node_failed_5ghz_steer_attempt(const std::string &mac);
    bool update_node_failed_24ghz_steer_attempt(const std::string &mac);

    /**
     * @brief Checks if it's possible to initiate client steering.
     *
     * @param sta_mac Mac address of client fetched from BUS, which made steering reqeust.
     * @param bss_id Target BSSID.
     * @return True if it's possible to initiate client steering, false otherwise.
     */
    bool can_start_client_steering(const std::string &sta_mac, const std::string &bssid);

    void update_node_11v_responsiveness(Station &station, bool success);
    bool get_node_11v_capability(const Station &mac);

    bool set_hostap_iface_name(const sMacAddr &al_mac, const sMacAddr &mac,
                               const std::string &iface_name);
    std::string get_hostap_iface_name(const sMacAddr &mac);

    bool set_hostap_iface_type(const sMacAddr &al_mac, const sMacAddr &mac,
                               beerocks::eIfaceType iface_type);
    beerocks::eIfaceType get_hostap_iface_type(const sMacAddr &mac);

    bool set_hostap_vap_list(const sMacAddr &mac,
                             const std::unordered_map<int8_t, sVapElement> &vap_list);
    std::unordered_map<int8_t, sVapElement> &get_hostap_vap_list(const sMacAddr &mac);
    std::set<std::string> get_hostap_vaps_bssids(const std::string &mac);
    bool remove_vap(Agent::sRadio &radio, int vap_id);
    bool add_vap(const std::string &radio_mac, int vap_id, const std::string &bssid,
                 const std::string &ssid, bool backhaul);

    /** Update VAP information
     *
     * Add or update the VAP information for the given BSSID on the given radio. If the VAP exists
     * already, it is updated. If no VAP with the given BSSID exists, a new one is created with
     * a unique vap_id.
     *
     * For prplMesh agents, this function should be called after the VAPs were created (with
     * add_vap) so the vap_id is correct. For non-prplMesh agents, the vap_id doesn't matter.
     */
    bool update_vap(const sMacAddr &radio_mac, const sMacAddr &bssid, const std::string &ssid,
                    bool backhaul);

    std::string get_hostap_ssid(const sMacAddr &mac);
    /**
     * @brief checks if vap name is on the steer list.
     *
     * @param[in] bssid vap mac address.
     * @return true if vap name is on the steer list.
     */
    bool is_vap_on_steer_list(const sMacAddr &bssid);
    std::string get_hostap_vap_with_ssid(const sMacAddr &mac, const std::string &ssid);
    sMacAddr get_hostap_vap_mac(const sMacAddr &mac, int vap_id);
    std::string get_node_parent_radio(const std::string &mac);

    /**
     * @brief Get data model path of node
     *
     * @param[in] mac node mac address.
     * @return Data model path of node on success or empty string otherwise.
     */
    std::string get_node_data_model_path(const std::string &mac);
    std::string get_node_data_model_path(const sMacAddr &mac);

    int8_t get_hostap_vap_id(const sMacAddr &mac);

    bool set_node_backhaul_iface_type(const std::string &mac, beerocks::eIfaceType iface_type);
    beerocks::eIfaceType get_node_backhaul_iface_type(const std::string &mac);

    std::string get_5ghz_sibling_hostap(const std::string &mac);

    bool set_global_restricted_channels(const uint8_t *restricted_channels);
    std::vector<uint8_t> get_global_restricted_channels();
    bool set_hostap_conf_restricted_channels(const sMacAddr &hostap_mac,
                                             const uint8_t *restricted_channels);
    std::vector<uint8_t> get_hostap_conf_restricted_channels(const sMacAddr &hostap_mac);
    bool
    fill_radio_channel_scan_capabilites(const sMacAddr &radio_mac,
                                        wfa_map::cRadiosWithScanCapabilities &radio_capabilities);

    //
    // CS - DFS
    //
    bool set_hostap_activity_mode(const sMacAddr &mac, beerocks::eApActiveMode ap_activity_mode);
    beerocks::eApActiveMode get_hostap_activity_mode(const sMacAddr &mac);
    bool set_radar_hit_stats(const sMacAddr &mac, uint8_t channel, uint8_t bw, bool is_csa_entry);
    bool set_supported_channel_radar_affected(const sMacAddr &mac,
                                              const std::vector<uint8_t> &channels, bool affected);
    //bool get_supported_channel_all_availble(const std::string &mac );

    bool set_hostap_is_dfs(const sMacAddr &mac, bool enable);
    bool get_hostap_is_dfs(const sMacAddr &mac);

    bool set_hostap_cac_completed(const sMacAddr &mac, bool enable);
    bool get_hostap_cac_completed(const sMacAddr &mac);

    bool set_hostap_on_dfs_reentry(const sMacAddr &mac, bool enable);
    bool get_hostap_on_dfs_reentry(const sMacAddr &mac);

    bool set_hostap_dfs_reentry_clients(const sMacAddr &mac,
                                        const std::set<std::string> &dfs_reentry_clients);
    std::set<std::string> get_hostap_dfs_reentry_clients(const sMacAddr &mac);
    bool clear_hostap_dfs_reentry_clients(const sMacAddr &mac);

    //
    // Channel Scan
    //
    /**
     * @brief Set the channel scan is enabled flag
     *
     * @param mac:    MAC address of radio
     * @param enable: enable flag to be set
     * @return true on success
     * @return false on failure
     */
    bool set_channel_scan_is_enabled(const sMacAddr &mac, bool enable);

    /**
     * @brief Get the channel scan is enabled flag
     *
     * @param [out] mac: MAC address of radio
     * @return current channel scan enable flag
     */
    bool get_channel_scan_is_enabled(const sMacAddr &mac);

    /**
     * @brief Set the channel scan interval sec object
     *
     * @param mac
     * @param interval_sec
     * @return true
     * @return false
     */
    bool set_channel_scan_interval_sec(const sMacAddr &mac, int interval_sec);

    /**
     * @brief Get the channel scan interval sec object
     *
     * @param mac: MAC address of radio
     * @return value o interval sec object
     */
    int get_channel_scan_interval_sec(const sMacAddr &mac);

    /**
     * @brief Set the channel scan is pending object
     *
     * @param mac:              MAC address of radio
     * @param scan_in_progress: Flag of current channel scan
     * @return true on success
     * @return false on failure
     */
    bool set_channel_scan_is_pending(const sMacAddr &mac, bool scan_is_pending);

    /**
     * @brief Set the channel scan in progress object
     *
     * @param mac:              MAC address of radio
     * @param scan_in_progress: Flag of current channel scan
     * @param single_scan:      Indicated if to use single scan or continuous
     * @return true on success
     * @return false on failure
     */
    bool set_channel_scan_in_progress(const sMacAddr &mac, bool scan_in_progress, bool single_scan);

    /**
     * @brief Get the channel scan in progress object
     * In the case of single scan also check the scan is pending flag
     *
     * @param mac          MAC address of radio
     * @param single_scan: Indicated if to use single scan or continuous
     * @return Flag of current channel scan
     */
    bool get_channel_scan_in_progress(const sMacAddr &mac, bool single_scan);

    /**
     * @brief Set the channel scan results status object
     *
     * @param mac:         MAC address of radio
     * @param error_code:  Current status of channel scan results
     * @param single_scan: Indicated if to use single scan or continuous
     * @return true on success
     * @return false on failure
     */
    bool set_channel_scan_results_status(const sMacAddr &mac,
                                         beerocks::eChannelScanStatusCode error_code,
                                         bool single_scan);

    /**
     * @brief Get the channel scan results status object
     *
     * @param mac:         MAC address of radio
     * @param single_scan: Indicated if to use single scan or continuous
     * @return Current status of channel scan results
     */
    beerocks::eChannelScanStatusCode get_channel_scan_results_status(const sMacAddr &mac,
                                                                     bool single_scan);

    /**
     * @brief Set the channel scan dwell time msec object
     *
     * @param mac:             MAC address of radio
     * @param dwell_time_msec: Dwell time of channel scan
     * @param single_scan:     Indicated if to use single scan or continuous
     * @return true on success
     * @return false on failure
     */
    bool set_channel_scan_dwell_time_msec(const sMacAddr &mac, int dwell_time_msec,
                                          bool single_scan);

    /**
     * @brief Get the channel scan dwell time msec object
     *
     * @param mac          MAC address of radio
     * @param single_scan: Indicated if to use single scan or continuous
     * @return Dwell time of channel scan
     */
    int get_channel_scan_dwell_time_msec(const sMacAddr &mac, bool single_scan);

    /**
     * @brief Set the channel scan pool object
     *
     * @param mac:          MAC address of radio
     * @param channel_pool: Channel pool of channel scan
     * @param single_scan:  Indicated if to use single scan or continuous
     * @return true on success
     * @return false on failure
     */
    bool set_channel_scan_pool(const sMacAddr &mac, const std::unordered_set<uint8_t> &channel_pool,
                               bool single_scan);

    /**
     * @brief Validate the channel scan pool
     *
     * @param mac:          MAC address of radio
     * @param channel_pool: Channel pool of channel scan
     * @return true if pool is valid
     * @return false if pool is invalid
     */
    bool is_channel_scan_pool_supported(const sMacAddr &mac,
                                        const std::unordered_set<uint8_t> &channel_pool);

    /**
     * @brief Get the channel scan pool object
     *
     * @param mac:         MAC address of radio
     * @param single_scan: Indicated if to use single scan or continuous
     * @return Channel pool of channel scan
     */
    const std::unordered_set<uint8_t> &get_channel_scan_pool(const sMacAddr &mac, bool single_scan);

    /**
     * @brief Checks whather a given channel is in the currently set channel pool
     *
     * @param mac:         MAC address of radio
     * @param channel:     Given channel to be checked
     * @param single_scan: Indicated if to use single scan or continuous
     * @return true if given channel is in current channel pool
     * @return false if given channel isn't in current channel pool
     */
    bool is_channel_in_pool(const sMacAddr &mac, uint8_t channel, bool single_scan);

    /**
     * @brief Clears any existing results for the given channel scan
     *
     * @param mac:         MAC address of radio
     * @param single_scan: Indicated if to use single scan or continuous
     * @return true on success
     * @return false on failure
     */
    bool clear_channel_scan_results(const sMacAddr &mac, bool single_scan);

    /**
     * @brief Adds a new scan result to the current scan results
     *
     * @param mac:         MAC address of radio
     * @param scan_result: Scan result to be added to current scan results
     * @param single_scan: Indicated if to use single scan or continuous
     * @return true on success
     * @return false on failure
     */
    bool add_channel_scan_results(const sMacAddr &mac, const sChannelScanResults &scan_result,
                                  bool single_scan);

    /**
     * @brief Get the report records for a given radio using a given timestamp.
     *
     * @param mac MAC address of radio.
     * @param ISO_8601_timestamp Channel scan report's timestamp.
     * @param operating_class Operating class of the report.
     * @param channel Channel of the report.
     * @return True if record exists, false otherwise.
     */
    bool has_channel_report_record(const sMacAddr &mac, const std::string &ISO_8601_timestamp,
                                   const uint8_t operating_class, const uint8_t channel);

    /**
     * @brief Clear the channel scan report record for the given timestamp.
     *
     * @param mac MAC address of radio.
     * @param ISO_8601_timestamp Channel scan report's timestamp.
     * @return True on success, false otherwise.
     */
    bool clear_channel_report_record(const sMacAddr &mac, const std::string &ISO_8601_timestamp);

    /**
     * @brief Get the channel scan report for the given radio and timestamp
     *
     * @param mac MAC address of radio.
     * @param ISO_8601_timestamp Channel scan report's timestamp.
     * @param[out] report_index  Copy of the report index.
     * @return True on success, false otherwise.
     *
     */
    bool get_channel_report_record(const sMacAddr &mac, const std::string &ISO_8601_timestamp,
                                   node::radio::channel_scan_report_index &report_index);

    /**
     * @brief Get the channel pool containing all the supported channels.
     *
     * @param[out] channel_pool_set Set containing the current channel pool.
     * @param[in] radio_mac MAC address of radio.
     */
    bool get_pool_of_all_supported_channels(std::unordered_set<uint8_t> &channel_pool_set,
                                            const sMacAddr &radio_mac);

    /**
     * @brief
     *
     * @param RUID Radio UID
     * @param operating_class Operating class of report
     * @param channel channel of report
     * @param neighbors vactor containing the neighboring APs
     * @return true on success
     * @return false on failure
     */
    bool add_channel_report(const sMacAddr &RUID, const uint8_t &operating_class,
                            const uint8_t &channel,
                            const std::vector<wfa_map::cNeighbors> &neighbors, uint8_t avg_noise,
                            uint8_t avg_utilization, const std::string &ISO_8601_timestamp,
                            bool override_existing_data = true);

    /**
     * @brief Get the report records for a given radio using the scan's index.
     *
     * @param RUID MAC address of radio.
     * @param index Channel scan report's index, set of pair<uint8_t, uint8_t>
     * @return True if record exists, false otherwise.
     */
    const std::vector<sChannelScanResults>
    get_channel_scan_report(const sMacAddr &RUID,
                            const node::radio::channel_scan_report_index &index);

    /**
     * @brief Get the report records for a given radio using a given timestamp.
     *
     * @param RUID MAC address of radio.
     * @param ISO_8601_timestamp Channel scan report's timestamp.
     * @return True if record exists, false otherwise.
     */
    const std::vector<sChannelScanResults>
    get_channel_scan_report(const sMacAddr &RUID, const std::string &ISO_8601_timestamp);

    /**
     * @brief Get the report records for a given radio using its channel-list as the key.
     *
     * @param RUID MAC address of radio.
     * @param single_scan Indicated if to use single scan or continuous
     * @return True if record exists, false otherwise.
     */
    const std::vector<sChannelScanResults> get_channel_scan_report(const sMacAddr &RUID,
                                                                   bool single_scan);

    /**
     * @brief Get the channel scan results object
     *
     * @param mac: MAC address of radio
     * @param single_scan: Indicated if to use single scan or continuous
     * @return const std::list<sChannelScanResults>&
     */
    const std::list<sChannelScanResults> &get_channel_scan_results(const sMacAddr &mac,
                                                                   bool single_scan);

    //
    // Client Persistent Data
    //
    /**
     * @brief Check if client exists in persistent db.
     *
     * @param mac MAC address of a client.
     * @return true if client exists, false otherwise.
     */
    bool is_client_in_persistent_db(const sMacAddr &mac);

    /**
     * @brief Adds a client to the persistent db, if already exists, remove old entry and add a new one.
     *
     * @param mac MAC address of a client.
     * @param params An unordered map of key-value of client parameters and their values.
     * @return true on success, otherwise false.
     */
    bool add_client_to_persistent_db(const sMacAddr &mac, const ValuesMap &params = {});

    /**
     * @brief Adds a client to the persistent db, if already exists, remove old entry and add a new one.
     *
     * @param mac MAC address of a client.
     * @param params An unordered map of key-value of client parameters and their values.
     * @return True on success, otherwise false.
     */
    bool add_steer_event_to_persistent_db(const ValuesMap &params = {});

    /**
     * @brief Get from persistent db all steer history event and register them on a system bus.
     *
     * @return True on success, otherwise false.
     */
    bool restore_steer_history();

    /**
     * @brief Get the client's parameters last edit time.
     *
     * @param mac MAC address of a client.
     * @return Client persistent data last edit time (even if edit was done only to runtime-dbb and not saved to persistent db), or time_point::min() if not-configured or failure.
     */
    std::chrono::system_clock::time_point get_client_parameters_last_edit(const sMacAddr &mac);

    /**
     * @brief Set the client's time-life delay.
     *
     * @param client Station object representing a client.
     * @param time_life_delay_minutes Client-specific aging time.
     * @param save_to_persistent_db If set to true, update the persistent-db (write-through), default is true.
     * @return true on success, otherwise false.
     */
    bool set_client_time_life_delay(Station &client,
                                    const std::chrono::minutes &time_life_delay_minutes,
                                    bool save_to_persistent_db = true);

    /**
     * @brief Set the client's stay-on-initial-radio.
     *
     * @param client Station object representing a client.
     * @param stay_on_initial_radio Enable client stay on the radio it initially connected to.
     * @param save_to_persistent_db If set to true, update the persistent-db (write-through), default is true.
     * @return true on success, otherwise false.
     */
    bool set_client_stay_on_initial_radio(Station &client, bool stay_on_initial_radio,
                                          bool save_to_persistent_db = true);

    /**
     * @brief Get the client's stay-on-initial-radio.
     *
     * @param mac MAC address of a client.
     * @return Enable client stay on the radio it initially connected to.
     */
    eTriStateBool get_client_stay_on_initial_radio(const sMacAddr &mac);

    /**
     * @brief Set the client's initial-radio.
     *
     * @param client Station object representing a client.
     * @param initial_radio_mac The MAC address of the radio that the client has initially connected to.
     * @param save_to_persistent_db If set to true, update the persistent-db (write-through), default is true.
     * @return true on success, otherwise false.
     */
    bool set_client_initial_radio(Station &client, const sMacAddr &initial_radio_mac,
                                  bool save_to_persistent_db = true);

    /**
     * @brief Set the client's selected-bands.
     *
     * @param client Station object representing a client.
     * @param selected_bands Client selected band/bands. Possible values are bitwise options of eClientSelectedBands.
     * @param save_to_persistent_db If set to true, update the persistent-db (write-through), default is true.
     * @return true on success, otherwise false.
     */
    bool set_client_selected_bands(Station &client, int8_t selected_bands,
                                   bool save_to_persistent_db = true);

    /**
     * @brief Set the client's unfriendly status.
     *
     * @param client Station object representing a client.
     * @param is_unfriendly Whether a client is unfriendly or not.
     * @param save_to_persistent_db If set to true, update the persistent-db (write-through), default is true.
     * @return true on success, otherwise false.
     */
    bool set_client_is_unfriendly(Station &client, bool is_unfriendly,
                                  bool save_to_persistent_db = true);

    /**
     * @brief Check if the radio's band is on one of the selected bands.
     *
     * @param client Client's mac address.
     * @param hostap MAC address of a radio.
     * @return true on success, otherwise false.
     */
    bool is_hostap_on_client_selected_bands(const sMacAddr &client, const sMacAddr &hostap);

    /**
     * @brief Clear client's persistent information.
     *
     * @param mac MAC address of a client.
     * @return true on success, otherwise false.
     */
    bool clear_client_persistent_db(const sMacAddr &mac);

    /**
     * @brief Update client's persistent information with the runtime information.
     *
     * @param client Station object representing a client.
     * @return true on success, otherwise false.
     */
    bool update_client_persistent_db(Station &client);

    /**
     * @brief Load all clients from persistent db.
     * Creates nodes for the clients in runtime-db and set persistent parameters values accordingly.
     * Aged Clients and Clients with invalid data are filtered-out and removed from persistent-DB.
     *
     * @return true on success, otherwise false.
     */
    bool load_persistent_db_clients();

    /**
     * @brief Get the clients with persistent data configured object
     *
     * @return std::deque<sMacAddr> containing mac addresses of clients with configured persistent data
     */
    std::deque<sMacAddr> get_clients_with_persistent_data_configured();

    //
    // CLI
    //
    void add_cli_socket(int sd);
    void remove_cli_socket(int sd);
    bool get_cli_debug_enable(int sd);
    int get_cli_socket_at(int idx);
    void set_slave_stop_on_failure_attempts(int attempts);
    int get_slave_stop_on_failure_attempts();

    //
    // BML
    //
    void add_bml_socket(int sd);
    void remove_bml_socket(int sd);
    bool get_bml_nw_map_update_enable(int sd);
    bool set_bml_nw_map_update_enable(int sd, bool update_enable);
    bool get_bml_stats_update_enable(int sd);
    bool set_bml_stats_update_enable(int sd, bool update_enable);
    bool get_bml_events_update_enable(int sd);
    bool set_bml_events_update_enable(int sd, bool update_enable);
    bool get_bml_topology_update_enable(int sd);
    bool set_bml_topology_update_enable(int sd, bool update_enable);
    int get_bml_socket_at(int idx);
    bool is_bml_listener_exist();

    void set_vap_list(std::shared_ptr<vaps_list_t> vaps_list);
    const std::shared_ptr<vaps_list_t> get_vap_list();
    void clear_vap_list();

    //
    // Measurements
    //

    bool set_hostap_stats_info(const sMacAddr &mac, const beerocks_message::sApStatsParams *params);
    void clear_hostap_stats_info(const sMacAddr &al_mac, const sMacAddr &mac);

    /**
     * @brief Notify about client disconnection.
     * @param mac String with STA mac address.
     * @param reason_code Reason code of clients failed association/connection.
     */
    bool notify_disconnection(const std::string &mac, const uint16_t reason_code);

    /**
     * @brief Update the node stats info
     *
     * @param[in] mac MAC address of the given node
     * @param[in] params pointer to the incoming parameters
     *
     * @return true on success, otherwise false.
     */
    bool set_node_stats_info(const sMacAddr &mac, const beerocks_message::sStaStatsParams *params);

    /**
     * @brief Clear any existing node stats info
     *
     * @param[in] mac MAC address of the given node
     */
    void clear_node_stats_info(const sMacAddr &mac);

    /**
     * @brief Set virtual AP metrics info
     *
     * @param[in] bssid vap mac address.
     * @param[in] uc_tx_bytes unicast send bytes
     * @param[in] uc_rx_bytes unicast received bytes
     * @param[in] mc_tx_bytes multicast send bytes
     * @param[in] mc_rx_bytes multicast received bytes
     * @param[in] bc_tx_bytes broadcast send bytes
     * @param[in] bc_rx_bytes broadcast received bytes
     * @return true on success, otherwise false.
     */
    bool set_vap_stats_info(const sMacAddr &bssid, uint64_t uc_tx_bytes, uint64_t uc_rx_bytes,
                            uint64_t mc_tx_bytes, uint64_t mc_rx_bytes, uint64_t bc_tx_bytes,
                            uint64_t bc_rx_bytes);

    bool commit_persistent_db_changes();
    bool is_commit_to_persistent_db_required();

    int get_hostap_stats_measurement_duration(const sMacAddr &mac);
    std::chrono::steady_clock::time_point get_hostap_stats_info_timestamp(const sMacAddr &mac);

    uint32_t get_node_rx_bytes(const std::string &mac);
    uint32_t get_node_tx_bytes(const std::string &mac);

    double get_node_rx_bitrate(const std::string &mac);
    double get_node_tx_bitrate(const std::string &mac);

    bool set_node_rx_phy_rate_100kb(const std::string &mac, uint16_t rx_phy_rate_100kb);
    bool set_node_tx_phy_rate_100kb(const std::string &mac, uint16_t tx_phy_rate_100kb);

    uint16_t get_node_rx_phy_rate_100kb(const std::string &mac);
    uint16_t get_node_tx_phy_rate_100kb(const std::string &mac);

    int get_hostap_channel_load_percent(const sMacAddr &mac);

    uint32_t get_hostap_total_sta_rx_bytes(const sMacAddr &mac);
    uint32_t get_hostap_total_sta_tx_bytes(const sMacAddr &mac);

    int get_hostap_total_client_tx_load_percent(const sMacAddr &mac);
    int get_hostap_total_client_rx_load_percent(const sMacAddr &mac);

    int get_node_rx_load_percent(const std::string &mac);
    int get_node_tx_load_percent(const std::string &mac);

    int8_t get_load_rx_rssi(const std::string &sta_mac);
    uint16_t get_load_rx_phy_rate_100kb(const std::string &sta_mac);
    uint16_t get_load_tx_phy_rate_100kb(const std::string &sta_mac);

    bool set_measurement_delay(const std::string &mac, int measurement_delay);
    int get_measurement_delay(const std::string &mac);

    bool set_measurement_sent_timestamp(const std::string &mac);

    int get_measurement_recv_delta(const std::string &mac);
    bool set_measurement_recv_delta(const std::string &mac);

    int get_measurement_window_size(const std::string &mac);
    bool set_measurement_window_size(const std::string &mac, int window_size);

    bool set_node_channel_bw(const sMacAddr &mac, int channel, beerocks::eWiFiBandwidth bw,
                             bool channel_ext_above_secondary, int8_t channel_ext_above_primary,
                             uint16_t vht_center_frequency);
    bool update_node_bw(const sMacAddr &mac, beerocks::eWiFiBandwidth bw);
    beerocks::eWiFiBandwidth get_node_bw(const std::string &mac);
    bool get_hostap_channel_ext_above_primary(const sMacAddr &hostap_mac);
    bool get_node_channel_ext_above_secondary(const std::string &mac);
    uint16_t get_hostap_vht_center_frequency(const sMacAddr &mac);

    /**
     * @brief Store BSS information in the bss_infos_new list.
     *
     * @param bss_info Structure with BSS information.
     */
    void add_bss_info_configuration_new(const wireless_utils::sBssInfoConf &bss_info);
    void add_bss_info_configuration(const sMacAddr &al_mac,
                                    const wireless_utils::sBssInfoConf &bss_info);
    /**
     * @brief Store BSS information in the bss_infos_global list.
     *
     * @param bss_info Structure with BSS information.
     */
    void add_bss_info_configuration(const wireless_utils::sBssInfoConf &bss_info);
    std::list<wireless_utils::sBssInfoConf> &get_bss_info_configuration(const sMacAddr &al_mac);

    /**
     * @brief Return bss_infos_global list with BSS information.
     */
    std::list<wireless_utils::sBssInfoConf> &get_bss_info_configuration();
    void clear_bss_info_configuration();
    void clear_bss_info_configuration(const sMacAddr &al_mac);

    /**
     * @brief Store traffic separation policy for agent.
     *
     * @param[in] al_mac AL MAC address of agent.
     * @param[in] config Traffic separation policy configuration.
     */
    void
    add_traffic_separataion_configuration(const sMacAddr &al_mac,
                                          const wireless_utils::sTrafficSeparationSsid &config);
    /**
     * @brief Store default 802.1Q settings for agent.
     *
     * @param[in] al_mac AL MAC address of agent.
     * @param[in] config Default 802.1Q setting configuration.
     */
    void add_default_8021q_settings(const sMacAddr &al_mac,
                                    const wireless_utils::s8021QSettings &config);

    /**
     * @brief Get traffic separation policy for agent.
     *
     * @param[in] al_mac AL MAC address of agent.
     *
     * @return List of policies for the AL mac. If not found, return empty list.
     */
    const std::list<wireless_utils::sTrafficSeparationSsid>
    get_traffic_separataion_configuration(const sMacAddr &al_mac);

    /**
     * @brief Get default 802.1Q settings for agent.
     *
     * @param[in] al_mac AL MAC address of agent.
     *
     * @return Default 8021Q settings for the AL mac. If not found, return empty struct.
     */
    wireless_utils::s8021QSettings get_default_8021q_setting(const sMacAddr &al_mac);

    /**
     * @brief Clear all known traffic separation configurations.
     */
    void clear_traffic_separation_configurations();

    /**
     * @brief Clear traffic separation configuration for an agent.
     *
     * @param[in] al_mac AL MAC address of agent.
     */
    void clear_traffic_separation_configurations(const sMacAddr &al_mac);

    /**
     * @brief Clear all known default 802.1Q settings.
     */
    void clear_default_8021q_settings();

    /**
     * @brief Clear traffic separation configuration for an agent.
     *
     * @param[in] al_mac AL MAC address of agent.
     */
    void clear_default_8021q_settings(const sMacAddr &al_mac);

    /**
     * @brief Disable periodic link metrics requests by setting interval to zero.
     */
    void disable_periodic_link_metrics_requests();

    /**
     * @brief Set radio utilization value in Device.WiFi.DataElements Data Model.
     *
     * Data model path example: "Device.WiFi.DataElements.Network.Device.1.Radio.1.Utilization"
     *
     * @param[in] bssid BSSID for specific radio.
     * @param[in] utilization Radio utilization value.
     * @return true on success, otherwise false.
     */
    bool set_radio_utilization(const sMacAddr &bssid, uint8_t utilization);

    /**
     * @brief Set radio metrics values in Device.WiFi.DataElements Data Model.
     *
     * Objects are Noise, Transmit, ReceiveSelf and ReceiveOther.
     * Data model path example: "Device.WiFi.DataElements.Network.Device.1.Radio.1.Noise"
     *
     * @param[in] uid uid for specific radio.
     * @param[in] noise Noise value [0, 220].
     * @param[in] transmit Transmit value.
     * @param[in] receive_self ReceiveSelf value.
     * @param[in] receive_other ReceiveOther value.
     * @return true on success, otherwise false.
     */
    bool set_radio_metrics(const sMacAddr &uid, uint8_t noise, uint8_t transmit,
                           uint8_t receive_self, uint8_t receive_other);

    /**
     * @brief Set estimated service parameters in Device.WiFi.DataElements Data Model.
     *
     * Data model path example: "Device.WiFi.DataElements.Network.Device.1.Radio.1.BSS.1.EstServiceParametersBE"
     *
     * @param[in] bssid BSSID.
     * @param[in] param_name Estimated service parameters name.
     * @param[in] esp_vslue Estimated service parameters value.
     * @return True on success, otherwise false.
     */
    bool set_estimated_service_param(const sMacAddr &bssid, const std::string &param_name,
                                     uint32_t esp_value);

    /**
     * @brief Updates master configuration if a setting is changed through NBAPI.
     *
     * Data model path : "Device.WiFi.DataElements.Configuration" defined in controller.odl
     *
     * @param nbapi_config Settings read from datamodel with change action.
     * @return true on success, otherwise false.
     */
    bool update_master_configuration(const sDbNbapiConfig &nbapi_config);

    /**
     * @brief Recalculate single value of attribute to Byte units according to its unit.
     *
     * If attribute unit is BYTES, method changes nothing.
     * According to attributes unit type, method recalculates bytes and assign on it again.
     *
     * @param[in] byte_counter_units attribute unit
     * @param[in] bytes Number of bytes to recalculate
     * @return Recalculate value of the bytes
     */
    uint64_t recalculate_attr_to_byte_units(
        wfa_map::tlvProfile2ApCapability::eByteCounterUnits byte_counter_units, uint64_t bytes);

    /**
     * @brief Clears CAC Status Report data model.
     *
     * Remove all indexes in CACAvailableChannel and updates timestamp.
     *
     * Data model path : "Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.CACStatus" defined in radio.odl
     *
     * @param[in] radio radio db object
     * @return true on success, otherwise false.
     */
    bool dm_clear_cac_status_report(std::shared_ptr<Agent::sRadio> radio);

    /**
     * @brief Adds instance for CACStatus.CACAvailableChannel and fullfills it.
     *
     * Data model path : "Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.CACStatus.CACAvailableChannel.{i}"
     *
     * @param[in] radio radio db object
     * @param[in] operating_class operating class
     * @param[in] channel channel number
     * @return true on success, otherwise false.
     */
    bool dm_add_cac_status_available_channel(std::shared_ptr<Agent::sRadio> radio,
                                             uint8_t operating_class, uint8_t channel);

    /**
     * @brief Removes excessive NBAPI objects from system bus, if amount of them succeed the limit.
     *
     * @param paths Queue with paths to NBAPI objects of particular type.
     * @param limit The maximum allowed amount of those objects.
     * @return True on success, false otherwise.
     */
    bool dm_check_objects_limit(std::queue<std::string> &paths, uint8_t limit);

    /**
     * @brief Updates CollectionInterval of the all Devices (Agents).
     *
     * The interval between the collection of consecutive measurements of the most frequently
     * updated Data Element from this device (milliseconds).
     *
     * @param interval interval in milliseconds
     * @return True on success, false otherwise.
     */
    bool dm_update_collection_intervals(std::chrono::milliseconds interval);

    /**
     * @brief Updates last contact time the agent.
     *
     * Each time Multi-AP control message is arrived, last contact time is updated.
     *
     * @param agent_mac agent AL-MAC address
     * @return True on success, false otherwise.
     */
    bool update_last_contact_time(const sMacAddr &agent_mac);

    /**
     * @brief Sets agents (Device) datamodel ManufacturerOUI object.
     *
     * This OUI is retrieved from agents AL-MAC first three bytes.
     *
     * @param agent database object
     * @return True on success, false otherwise.
     */
    bool dm_set_agent_oui(std::shared_ptr<Agent> agent);

    /**
     * @brief Adds station steering event to database map and also for data model.
     *
     * Map stores events with maximum number MAX_EVENT_HISTORY_SIZE per station.
     *
     * @param sta_mac station mac address
     * @param event station steering event
     * @return True on success, false otherwise.
     */
    bool add_sta_steering_event(const sMacAddr &sta_mac, sStaSteeringEvent &event);

    /**
     * @brief Restores station steering event from database to add to new data model path of station.
     *
     * When station is steered/disassociated all data models are removed for that specific station.
     * To recover old steering history, this method reads it from database and add old ones to data model.
     *
     * @param station Station object
     * @return True on success, false otherwise.
     */
    bool dm_restore_sta_steering_event(const Station &station);

    /**
     * @brief Sets multi ap backhaul datamodel of devices.
     *
     * Controller does not have any Backhaul, so it left empty as standard requested.
     *
     * BackhaulMACAddress -> Parent Backhaul MAC Address (Parent's BH BSS, or ETH MAC)
     * BackhaulDeviceID -> Parent Device ID (AL_MAC)
     * MACAddress -> Current Device's Backhaul Interface MAC (BH STA or ETH MAC)
     *
     * DM path : "Device.WiFi.DataElements.Network.Device.{i}.MultiAPDevice.Backhaul"
     *
     * @param agent agent whose multi ap backhaul object is set
     * @param parent_bssid parent BSSID of the backhaul STA interface
     * @param backhaul_mac backhaul interface mac
     * @param interface_type backhaul link/interface type
     * @return True on success, false otherwise.
     */
    bool dm_set_device_multi_ap_backhaul(const Agent &agent, const sMacAddr &parent_bssid,
                                         const sMacAddr &backhaul_mac,
                                         const beerocks::eIfaceType &interface_type);

    /**
     * @brief Sets Device datamodel board info parameters.
     *
     * DM path : "Device.WiFi.DataElements.Network.Device.{i}"
     *
     * @param agent agent whose Device object is set
     * @param device_info struct with values of board info parameters
     * @return True on success, false otherwise.
     */
    bool dm_set_device_board_info(const Agent &agent,
                                  const beerocks::bpl::sDeviceInfo &device_info);

    /**
     * @brief Adds to data model an instance of object AssociationEventData.
     *
     * This object describes an event generated when a STA associates to a BSS.
     * Example of full path to object:
     * 'Device.WiFi.DataElements.AssociationEvent.AssociationEventData.1'.
     *
     * @param bssid BSS mac address.
     * @param client_mac Client mac address.
     * @param assoc_ts Timesamp in Data Model time format of station association.
     * @return Path to object on success, empty sring otherwise.
     */
    std::string dm_add_association_event(const sMacAddr &bssid, const sMacAddr &client_mac,
                                         const std::string &assoc_ts = {});

    //
    // tasks
    //
    bool assign_load_balancer_task_id(const std::string &mac, int new_task_id);
    int get_load_balancer_task_id(const std::string &mac);

    bool assign_network_optimization_task_id(int new_task_id);
    int get_network_optimization_task_id();

    bool assign_bml_task_id(int new_task_id);
    int get_bml_task_id();

    bool assign_rdkb_wlan_task_id(int new_task_id);
    int get_rdkb_wlan_task_id();

    bool assign_channel_selection_task_id(int new_task_id);
    int get_channel_selection_task_id();

    bool assign_dynamic_channel_selection_task_id(const sMacAddr &mac, int new_task_id);
    int get_dynamic_channel_selection_task_id(const sMacAddr &mac);

    bool assign_dynamic_channel_selection_r2_task_id(int new_task_id);
    int get_dynamic_channel_selection_r2_task_id();

    bool assign_persistent_db_aging_operation_id(int new_operation_id);
    int get_persistent_db_aging_operation_id();

    bool assign_persistent_db_data_commit_operation_id(int new_operation_id);
    int get_persistent_db_data_commit_operation_id();

    bool assign_dhcp_task_id(int new_task_id);
    int get_dhcp_task_id();

    bool assign_agent_monitoring_task_id(int new_task_id);
    int get_agent_monitoring_task_id();

    bool assign_statistics_polling_task_id(int new_task_id);
    int get_statistics_polling_task_id();

    void lock();
    void unlock();

    //
    // settings
    //
    std::string settings_vendor() { return config.vendor; }
    std::string settings_model() { return config.model; }

    // Features:
    void settings_dfs_reentry(bool en)
    {
        settings.enable_dfs_reentry = en && config.load_dfs_reentry;
    }
    bool settings_dfs_reentry() { return settings.enable_dfs_reentry; }
    void settings_client_band_steering(bool en)
    {
        settings.client_band_steering = en && config.load_client_band_steering;
    }
    bool settings_client_band_steering() { return settings.client_band_steering; }
    void settings_client_optimal_path_roaming(bool en)
    {
        settings.client_optimal_path_roaming = en && config.load_client_optimal_path_roaming;
    }
    bool settings_client_optimal_path_roaming() { return settings.client_optimal_path_roaming; }
    void settings_legacy_client_roaming(bool en)
    {
        settings.legacy_client_roaming = en && config.load_legacy_client_roaming;
    }
    bool settings_legacy_client_roaming() { return settings.legacy_client_roaming; }
    void settings_client_11k_roaming(bool en)
    {
        settings.client_11k_roaming = en && config.load_client_11k_roaming;
    }
    bool settings_client_11k_roaming() { return settings.client_11k_roaming; }

    void settings_ire_roaming(bool en) { settings.ire_roaming = en && config.load_ire_roaming; }
    bool settings_ire_roaming() { return settings.ire_roaming; }

    void settings_load_balancing(bool en)
    {
        settings.load_balancing = en && config.load_load_balancing;
    }
    bool settings_load_balancing() { return settings.load_balancing; }

    void settings_diagnostics_measurements(bool en)
    {
        settings.diagnostics_measurements = en && config.load_diagnostics_measurements;
    }
    bool settings_diagnostics_measurements() { return settings.diagnostics_measurements; }
    void settings_backhaul_measurements(bool en)
    {
        settings.backhaul_measurements = en && config.load_backhaul_measurements;
    }
    bool settings_backhaul_measurements() { return settings.backhaul_measurements; }
    void settings_front_measurements(bool en)
    {
        settings.front_measurements = en && config.load_front_measurements;
    }
    bool settings_front_measurements() { return settings.front_measurements; }
    void settings_monitor_on_vaps(bool en)
    {
        settings.monitor_on_vaps = en && config.load_monitor_on_vaps;
    }
    bool settings_monitor_on_vaps() { return settings.monitor_on_vaps; }

    void settings_health_check(bool en) { settings.health_check = en && config.load_health_check; }
    bool settings_health_check() { return settings.health_check; }

    void settings_service_fairness(bool en)
    {
        settings.service_fairness = en && config.load_service_fairness;
    }
    bool settings_service_fairness() { return settings.service_fairness; }
    void settings_rdkb_extensions(bool en)
    {
        settings.rdkb_extensions = en && config.load_rdkb_extensions;
    }
    bool settings_rdkb_extensions() { return settings.rdkb_extensions; }

    // Params
    void setting_certification_mode(bool en) { config.certification_mode = en; }

    bool setting_certification_mode() { return config.certification_mode; }

    void settings_client_optimal_path_roaming_prefer_signal_strength(bool en)
    {
        settings.client_optimal_path_roaming_prefer_signal_strength = en;
    }
    bool settings_client_optimal_path_roaming_prefer_signal_strength()
    {
        return settings.client_optimal_path_roaming_prefer_signal_strength;
    }

    bool is_prplmesh(const sMacAddr &mac);
    void set_prplmesh(const sMacAddr &mac);

    //
    // Controller context
    //
    void set_controller_ctx(Controller *ctx) { m_controller_ctx = ctx; }
    Controller *get_controller_ctx() { return m_controller_ctx; }

    const sMacAddr &get_local_bridge_mac() { return m_local_bridge_mac; }

    //
    // vars
    //
    sDbMasterConfig &config;

private:
    /**
     * @brief Adds node to the database.
     *
     * @param mac MAC address of the node.
     * @param parent_mac
     * @param type The type of node used for node-type verification.
     * @return std::shared_ptr<node> pointer to the node on success, nullptr otherwise.
     */
    bool add_node(const sMacAddr &mac,
                  const sMacAddr &parent_mac = beerocks::net::network_utils::ZERO_MAC,
                  beerocks::eType type       = beerocks::TYPE_CLIENT);
    std::shared_ptr<node> get_node(const std::string &key); //key can be <mac> or <al_mac>_<ruid>
    std::shared_ptr<node> get_node(const sMacAddr &mac);

    /**
     * @brief Returns the node object after verifing node type.
     *
     * if node is found but type is not requested type a nullptr is returned.
     *
     * @param mac MAC address of the node.
     * @param type The type of node used for node-type verification.
     * @return std::shared_ptr<node> pointer to the node on success, nullptr otherwise.
     */
    std::shared_ptr<node> get_node_verify_type(const sMacAddr &mac, beerocks::eType type);
    std::shared_ptr<node::radio> get_hostap(const sMacAddr &radio_uid);
    int get_node_hierarchy(std::shared_ptr<node> n);
    std::set<std::shared_ptr<node>> get_node_subtree(std::shared_ptr<node> n);
    void adjust_subtree_hierarchy(std::shared_ptr<node> n);
    void adjust_subtree_hierarchy(std::set<std::shared_ptr<node>> subtree, int offset);
    std::set<std::shared_ptr<node>> get_node_children(std::shared_ptr<node> n,
                                                      int type               = beerocks::TYPE_ANY,
                                                      int state              = beerocks::STATE_ANY,
                                                      std::string parent_mac = std::string());

    void rewind();
    bool get_next_node(std::shared_ptr<node> &n);

    /**
     * @brief Updates the client values in the persistent db.
     *
     * @param mac MAC address of a client.
     * @param values_map A map of client params and their values.
     * @return true on success, otherwise false.
     */
    bool update_client_entry_in_persistent_db(const sMacAddr &mac, const ValuesMap &values_map);

    /**
     * @brief Sets the node params (runtime db) from a param-value map.
     *
     * @param mac MAC address of node to be updated.
     * @param values_map A map of client params and their values.
     * @return true on success, otherwise false.
     */
    bool set_node_params_from_map(const sMacAddr &mac, const ValuesMap &values_map);

    /**
     * @brief Adds a client entry to persistent_db with configured parameters and increments clients counter.
     *
     * @param entry_name Client entry name in persistent db.
     * @param values_map A map of client params and their values.
     * @return true on success, otherwise false.
     */
    bool add_client_entry_and_update_counter(const std::string &entry_name,
                                             const ValuesMap &values_map);

    /**
     * @brief Removes a client entry from persistent_db and decrements clients counter.
     *
     * @param entry_name Client entry name in persistent db.
     * @return true on success, otherwise false.
     */
    bool remove_client_entry_and_update_counter(const std::string &entry_name);

    /**
     * @brief Removes client with least timelife remaining from persistent db (with preference to disconnected clients).
     *
     * @param[in] client_to_skip A client mac that should not be selected as cadidate. This is to prevent currently added node as candidate.
     * @return true on success, otherwise false.
     */
    bool remove_candidate_client(sMacAddr client_to_skip = beerocks::net::network_utils::ZERO_MAC);

    /**
     * @brief Returns the preferred client to be removed.
     *
     * Preference is determined as follows:
     * - Prefer disconnected clients over connected ones.
     * - According to above, the client with least time left before aging.

     * @param[in] client_to_skip A client mac that should not be selected as cadidate. This is to prevent currently added node as candidate.
     * @return sMacAddr mac of candidate client to be removed - if not found, string_utils::ZERO_MAC is returned.
     */
    sMacAddr get_candidate_client_for_removal(
        sMacAddr client_to_skip = beerocks::net::network_utils::ZERO_MAC);

    /**
     * @brief Adds instance to the datamodel for the unique MAC
     *
     * @param[in] mac Mac address for the new device
     * @return Path of device instance on success or empty string otherwise.
     */
    std::string dm_add_device_element(const sMacAddr &mac);

    /**
     * @brief Add station 'HECapabilities' data element, set values to its parameters.
     *
     * Example of full path to object:
     * "Device.WiFi.DataElements.Netwok.Device.1.Radio.1.Capabilities.BSS.1.STA.1.HECapabilities".
     *
     * @param path_to_sta Path to station.
     * Example of full path to station:
     * "Device.WiFi.DataElements.Netwok.Device.1.Radio.1.Capabilities.BSS.1.STA.1.
     * @param sta_cap Structure with station HE Capabilities.
     * @return True on success, false otherwise.
     */
    bool dm_set_sta_he_capabilities(const std::string &path_to_sta,
                                    const beerocks::message::sRadioCapabilities &sta_cap);

    /**
     * @brief Add station 'HTCapabilities' data element, set values to its parameters.
     *
     * Example of full path to object:
     * "Device.WiFi.DataElements.Netwok.Device.1.Radio.1.Capabilities.BSS.1.STA.1.HTCapabilities".
     *
     * @param path_to_sta Path to station.
     * Example of full path to station:
     * "Device.WiFi.DataElements.Netwok.Device.1.Radio.1.Capabilities.BSS.1.STA.1.
     * @param sta_cap Structure with station HT Capabilities.
     * @return True on success, false otherwise.
     */
    bool dm_set_sta_ht_capabilities(const std::string &path_to_sta,
                                    const beerocks::message::sRadioCapabilities &sta_cap);

    /**
     * @brief Add station 'VHTCapabilities' data element, set values to its parameters.
     *
     * Example of full path to VHTCapabilities object:
     * "Device.WiFi.DataElements.Netwok.Device.1.Radio.1.Capabilities.BSS.1.STA.1.VHTCapabilities".
     *
     * @param path_to_sta Path to station.
     * Example of full path to the station:
     * "Device.WiFi.DataElements.Netwok.Device.1.Radio.1.Capabilities.BSS.1.STA.1."
     * @param sta_cap Structure with station capabilities.
     * @return True on success, false otherwise.
     */
    bool dm_set_sta_vht_capabilities(const std::string &path_to_obj,
                                     const beerocks::message::sRadioCapabilities &sta_cap);

    /**
     * @brief Sets value for parameters of optional sub-object STA HTCapabilities.
     *
     * @param path_to_event Path to event which contains STA HTCapabilities sub-object.
     * @param sta_cap Structure with station HT Capabilities.
     * @return True on success, false otherwise.
     */
    bool dm_set_assoc_event_sta_ht_cap(const std::string &path_to_event,
                                       const beerocks::message::sRadioCapabilities &sta_cap);

    /**
     * @brief Sets value for parameters of optional sub-object STA VHTCapabilities.
     *
     * @param path_to_event Path to event which contains STA VHTCapabilities sub-object.
     * @param sta_cap Structure with station VHT Capabilities.
     * @return True on success, false otherwise.
     */
    bool dm_set_assoc_event_sta_vht_cap(const std::string &path_to_event,
                                        const beerocks::message::sRadioCapabilities &sta_cap);

    /**
     * @brief Adds STA instance to the datamodel.
     *
     * Data model path example: "Device.WiFi.DataElements.Network.Device.1.Radio.1.BSS.2.STA.3"
     *
     * @param bssid BSS mac address.
     * @param station Station object
     * @return True on success, false otherwise.
     */
    bool dm_add_sta_element(const sMacAddr &bssid, Station &station);

    /**
     * @brief Prepares path to the BSS data element with correct index (i).
     *
     * Data model path example: "Device.WiFi.DataElements.Network.Device.1.Radio.1.BSS.2."
     *
     * @param bssid BSSID.
     * @return Path to bss, empty string otherwise.
     */
    std::string dm_get_path_to_bss(const sMacAddr &bssid);

    /**
     * @brief Set clients (device) multi ap capabilities
     *
     * Data model path example: "Device.WiFi.DataElements.Network.Device.1.MultiAPCapabilities.{capabilities}"
     *
     * @param device_mac mac address of device
     * @return True on success, false otherwise.
     */
    bool dm_set_device_multi_ap_capabilities(const std::string &device_mac);

    /**
     * @brief Add instance of 'OperatingClasses' data element, set values for its parameters.
     *
     * Data model path example: "Device.WiFi.DataElements.Network.Device.1.Radio.1.Capabilities.OperatingClasses.1."
     *
     * @param radio_mac mac address of radio which reporting operating class.
     * @param max_tx_power max transmit power.
     * @param op_class operating class.
     * @param non_operable_channels list of non oparable channels.
     * @return true on success, false otherwise.
     */
    bool dm_add_ap_operating_classes(const std::string &radio_mac, uint8_t max_tx_power,
                                     uint8_t op_class,
                                     const std::vector<uint8_t> &non_operable_channels);

    /**
     * @brief Add or update instance of "BSS" data element, set values for its parameters.
     *
     * Example: "Device.WiFi.DataElements.Network.Device.1.Radio.1.BSS.1"
     *
     * @param radio_mac mac address of radio on which BSSID exists.
     * @param bssid BSSID of the BSS.
     * @param ssid SSID of the BSS. If empty, BSS is considered disabled.
     */
    bool dm_set_radio_bss(const sMacAddr &radio_mac, const sMacAddr &bssid,
                          const std::string &ssid);

    /**
     * @brief Set data model path member of a node
     *
     * @param mac mac address of node
     * @param data_model_path data model path
     * @return true on success, false otherwise.
     */
    bool set_node_data_model_path(const sMacAddr &mac, const std::string &data_model_path);

    int network_optimization_task_id           = -1;
    int channel_selection_task_id              = -1;
    int dynamic_channel_selection_r2_task_id   = -1;
    int bml_task_id                            = -1;
    int rdkb_wlan_task_id                      = -1;
    int config_update_task_id                  = -1;
    int persistent_db_aging_operation_id       = -1;
    int persistent_db_data_commit_operation_id = -1;
    int dhcp_task_id                           = -1;
    int agent_monitoring_task_id               = -1;
    int statistics_polling_task_id             = -1;

    std::shared_ptr<node> last_accessed_node;
    std::string last_accessed_node_mac;

    std::mutex db_mutex;

    std::unordered_map<std::string, std::shared_ptr<node>> nodes[beerocks::HIERARCHY_MAX];

    /**
    *  @brief This variable indicates that data is awaiting to be commited over to the persistentDB.
    */
    bool persistent_db_changes_made = false;

    int slaves_stop_on_failure_attempts = 0;

    /**
     * @brief some operations on unordered_map can cause iterators to be invalidated use the following with caution.
     */
    int current_hierarchy = 0;
    std::unordered_map<std::string, std::shared_ptr<node>>::iterator db_it =
        std::unordered_map<std::string, std::shared_ptr<node>>::iterator();

    std::vector<int> cli_debug_sockets;
    std::vector<sBmlListener> bml_listeners_sockets;

    beerocks::logging &logger;

    sDbMasterSettings settings;
    std::vector<uint8_t> global_restricted_channels;
    friend class network_map;

    std::shared_ptr<vaps_list_t> m_vap_list;

    /**
     * @brief This map holds link metric "data struct" per reporting Agent sMacAddr .
     * "data struct" holds map of the actual link_metrics_data vector (tx/rx) per reported Agent sMacAddr.
     * Map is Used in TYPE_GW/TYPE_IRE nodes.
     * Map created empty in all other nodes.
     */
    //TODO: This map should be moved to the agent nodes instead of being a separate map.
    std::unordered_map<sMacAddr, std::unordered_map<sMacAddr, son::node::link_metrics_data>>
        m_link_metric_data;

    /**
     * @brief This map holds ap metric data per reporting Agent sMacAddr .
     * Map is Used in TYPE_GW/TYPE_IRE nodes.
     * Map created empty in all other nodes.
     */
    //TODO: This map should be moved to the BSS nodes (which currently don't exist) instead of being a separate map.
    std::unordered_map<sMacAddr, son::node::ap_metrics_data> m_ap_metric_data;

    // certification
    std::shared_ptr<uint8_t> certification_tx_buffer;
    std::unordered_map<sMacAddr, std::list<wireless_utils::sBssInfoConf>> bss_infos; // key=al_mac
    std::list<wireless_utils::sBssInfoConf> bss_infos_global;
    std::list<wireless_utils::sBssInfoConf> bss_infos_new;

    /**
     * @brief This map holds traffic separation policy per Agent sMacAddr.
     */
    std::unordered_map<sMacAddr, std::list<wireless_utils::sTrafficSeparationSsid>>
        traffic_separation_policy_configurations; // key=al_mac

    /**
     * @brief This map holds default 802.1Q settings per Agent sMacAddr.
     */
    std::unordered_map<sMacAddr, wireless_utils::s8021QSettings>
        default_8021q_settings; // key=al_mac

    Controller *m_controller_ctx = nullptr;
    const sMacAddr m_local_bridge_mac;

    int m_persistent_db_clients_count = 0;

    /**
     * @brief Queue with name of clients steer
     * history entries in persistent database.
     */
    std::queue<std::string> m_steer_history;

    std::shared_ptr<beerocks::nbapi::Ambiorix> m_ambiorix_datamodel;

    /**
     * @brief Maximum amount of events registered on the system bus NBAPI
     */
    const uint8_t MAX_EVENT_HISTORY_SIZE = 24;

    /*
    *  The queue with paths of NBAPI SteerEvent NBAPI objects.
    */
    std::queue<std::string> m_steer_events;

    /**
     * @brief The queue with paths of NBAPI disassociation events.
     */
    std::queue<std::string> m_disassoc_events;

    /**
     * @brief The queue with paths of NBAPI association events.
     */
    std::queue<std::string> m_assoc_events;

    /**
     * @brief Maximum amount of NBAPI ScanResults registered on the system bus.
     */
    const uint8_t MAX_SCAN_RESULT_HISTORY_SIZE = 5;

    /**
     * @brief The queue with paths of NBAPI ScanResults.
     */
    std::queue<std::string> m_scan_results;

    /*
    * key = Client mac.
    * value = Latest dialog token from sta beacon measurement.
    */
    std::unordered_map<sMacAddr, uint8_t> m_dialog_tokens;
};

} // namespace son

#endif
