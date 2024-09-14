/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */
#include "son_slave_thread.h"

#include "agent_db.h"

#include "cac_status_database.h"
#include "gate/1905_beacon_query_to_vs.h"
#include "gate/vs_beacon_response_to_1905.h"
#include "tasks/ap_autoconfiguration_task.h"
#include "tasks/service_prioritization_task.h"
#include <bcl/beerocks_cmdu_client_factory_factory.h>
#include <bcl/beerocks_cmdu_server_factory.h>
#include <bcl/beerocks_timer_factory_impl.h>
#include <bcl/beerocks_timer_manager_impl.h>
#include <bcl/beerocks_utils.h>
#include <bcl/network/network_utils.h>
#include <btl/broker_client_factory_factory.h>

#include <beerocks/tlvf/beerocks_message.h>
#include <beerocks/tlvf/beerocks_message_1905_vs.h>
#include <beerocks/tlvf/beerocks_message_apmanager.h>
#include <beerocks/tlvf/beerocks_message_backhaul.h>
#include <beerocks/tlvf/beerocks_message_control.h>
#include <beerocks/tlvf/beerocks_message_monitor.h>
#include <beerocks/tlvf/beerocks_message_platform.h>
#include <easylogging++.h>
#include <mapf/common/utils.h>
#include <tlvf/AttrList.h>
#include <tlvf/ieee_1905_1/tlvAlMacAddress.h>
#include <tlvf/wfa_map/tlvApMetricQuery.h>
#include <tlvf/wfa_map/tlvAssociatedStaTrafficStats.h>
#include <tlvf/wfa_map/tlvBeaconMetricsResponse.h>
#include <tlvf/wfa_map/tlvChannelPreference.h>
#include <tlvf/wfa_map/tlvChannelSelectionResponse.h>
#include <tlvf/wfa_map/tlvClientAssociationControlRequest.h>
#include <tlvf/wfa_map/tlvClientAssociationEvent.h>
#include <tlvf/wfa_map/tlvHigherLayerData.h>
#include <tlvf/wfa_map/tlvOperatingChannelReport.h>
#include <tlvf/wfa_map/tlvProfile2CacCompletionReport.h>
#include <tlvf/wfa_map/tlvProfile2CacStatusReport.h>
#include <tlvf/wfa_map/tlvProfile2ReasonCode.h>
#include <tlvf/wfa_map/tlvProfile2SteeringRequest.h>
#include <tlvf/wfa_map/tlvStaMacAddressType.h>
#include <tlvf/wfa_map/tlvSteeringBTMReport.h>
#include <tlvf/wfa_map/tlvSteeringRequest.h>
#include <tlvf/wfa_map/tlvTransmitPowerLimit.h>

#include "gate/1905_beacon_query_to_vs.h"
#include "gate/vs_beacon_response_to_1905.h"
#include "traffic_separation.h"


// BPL Error Codes
#include <bpl/bpl_cfg.h>
#include <bpl/bpl_err.h>

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Constatnts /////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

constexpr int SLAVE_INIT_DELAY_SEC                                    = 4;
constexpr int WAIT_FOR_PLATFORM_MANAGER_REGISTER_RESPONSE_TIMEOUT_SEC = 600;
constexpr int MONITOR_HEARTBEAT_TIMEOUT_SEC                           = 10;
constexpr int MONITOR_HEARTBEAT_RETRIES                               = 10;
constexpr int AP_MANAGER_HEARTBEAT_TIMEOUT_SEC                        = 10;
constexpr int AP_MANAGER_HEARTBEAT_RETRIES                            = 10;

//////////////////////////////////////////////////////////////////////////////
/////////////////////////// Local Module Functions ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Implementation ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

using namespace beerocks;
using namespace net;
using namespace son;

slave_thread::slave_thread(sAgentConfig conf, beerocks::logging &logger_)
    : cmdu_tx(m_tx_buffer, sizeof(m_tx_buffer)), config(conf), logger(logger_)
{
    thread_name = BEEROCKS_AGENT;

    // Set configuration on Agent database.
    auto db                                  = AgentDB::get();
    db->bridge.iface_name                    = conf.bridge_iface;
    db->backhaul.preferred_bssid             = tlvf::mac_from_string(conf.backhaul_preferred_bssid);
    db->device_conf.stop_on_failure_attempts = conf.stop_on_failure_attempts;

    for (const auto &radio_map_element : config.radios) {

        const auto &fronthaul_iface = radio_map_element.first;
        const auto &radio_conf      = radio_map_element.second;

        auto &radio_manager                    = m_radio_managers[fronthaul_iface];
        radio_manager.stop_on_failure_attempts = db->device_conf.stop_on_failure_attempts;

        auto radio = db->add_radio(fronthaul_iface, radio_conf.backhaul_wireless_iface);
        if (!radio) {
            m_constructor_failed = true;
            // No need to print here anything, 'add_radio()' does it internally
            return;
        }

        radio->sta_iface_filter_low = radio_conf.backhaul_wireless_iface_filter_low;
    }
}

slave_thread::~slave_thread()
{
    LOG(DEBUG) << "destructor - agent_reset()";
    stop_slave_thread();
}

bool slave_thread::thread_init()
{
    /** Logger Initialization **/
    logger.set_thread_name(logger.get_module_name());
    logger.attach_current_thread_to_logger_id();

    if (m_constructor_failed) {
        LOG(ERROR) << "Not initalizing the Agent. There was an error in the constructor";
        return false;
    }

    LOG(INFO) << "Agent Info:";
    for (const auto &radio_map_element : config.radios) {

        const auto &fronthaul_iface = radio_map_element.first;
        const auto &radio_conf      = radio_map_element.second;
        LOG(INFO) << "fronthaul_iface=" << fronthaul_iface;
        LOG(INFO) << "fronthaul_iface_type=" << radio_conf.hostap_iface_type;

        if (radio_conf.hostap_iface_type == beerocks::IFACE_TYPE_UNSUPPORTED) {
            LOG(ERROR) << "hostap_iface_type '" << radio_conf.hostap_iface_type << "' UNSUPPORTED!";
            return false;
        }
    }
    /**  Broker Client  **/

    // Create broker client factory to create broker clients when requested
    std::string broker_uds_path = config.temp_path + std::string(BEEROCKS_BROKER_UDS);
    m_broker_client_factory =
        beerocks::btl::create_broker_client_factory(broker_uds_path, m_event_loop);
    LOG_IF(!m_broker_client_factory, FATAL) << "Unable to create broker client factory!";

    // Create an instance of a broker client connected to the broker server that is running in the
    // transport process
    m_broker_client = m_broker_client_factory->create_instance();
    LOG_IF(!m_broker_client, FATAL) << "Failed to create instance of broker client";

    beerocks::btl::BrokerClient::EventHandlers broker_client_handlers;
    // Install a CMDU-received event handler for CMDU messages received from the transport process.
    // These messages are actually been sent by a remote process and the broker server running in
    // the transport process just forwards them to the broker client.
    broker_client_handlers.on_cmdu_received = [&](uint32_t iface_index, const sMacAddr &dst_mac,
                                                  const sMacAddr &src_mac,
                                                  ieee1905_1::CmduMessageRx &cmdu_rx) {
        handle_cmdu_from_broker(iface_index, dst_mac, src_mac, cmdu_rx);
    };

    // Install a connection-closed event handler.
    // Currently there is no recovery mechanism if connection with broker server gets interrupted
    // (something that happens if the transport process dies). Just log a message and exit
    broker_client_handlers.on_connection_closed = [&]() {
        LOG(ERROR) << "Broker client got disconnected!";
        return false;
    };

    m_broker_client->set_handlers(broker_client_handlers);

    // Subscribe for the reception of CMDU messages that this process is interested in
    if (!m_broker_client->subscribe(std::set<ieee1905_1::eMessageType>{
            ieee1905_1::eMessageType::TOPOLOGY_DISCOVERY_MESSAGE,
            ieee1905_1::eMessageType::AP_AUTOCONFIGURATION_RESPONSE_MESSAGE,
            ieee1905_1::eMessageType::AP_AUTOCONFIGURATION_WSC_MESSAGE,
            ieee1905_1::eMessageType::AP_AUTOCONFIGURATION_RENEW_MESSAGE,
            ieee1905_1::eMessageType::MULTI_AP_POLICY_CONFIG_REQUEST_MESSAGE,
            ieee1905_1::eMessageType::SERVICE_PRIORITIZATION_REQUEST_MESSAGE,
        })) {
        LOG(FATAL) << "Failed subscribing to the Bus";
    }

    /** CMDU Server **/

    // Create UDS address where the server socket will listen for incoming connection requests.
    std::string agent_server_uds_path = config.temp_path + std::string(BEEROCKS_AGENT_UDS);
    m_cmdu_server_uds_address = beerocks::net::UdsAddress::create_instance(agent_server_uds_path);
    LOG_IF(!m_cmdu_server_uds_address, FATAL)
        << "Unable to create UDS server address for backhaul manager!";

    // Create server to exchange CMDU messages with clients connected through a UDS socket
    m_cmdu_server =
        beerocks::CmduServerFactory::create_instance(m_cmdu_server_uds_address, m_event_loop);
    LOG_IF(!m_cmdu_server, FATAL) << "Unable to create CMDU server for backhaul manager!";

    beerocks::CmduServer::EventHandlers cmdu_server_handlers{
        .on_client_connected    = nullptr,
        .on_client_disconnected = [&](int fd) { handle_client_disconnected(fd); },
        .on_cmdu_received       = [&](int fd, uint32_t iface_index, const sMacAddr &dst_mac,
                                const sMacAddr &src_mac,
                                ieee1905_1::CmduMessageRx &cmdu_rx) { handle_cmdu(fd, cmdu_rx); },
    };
    m_cmdu_server->set_handlers(cmdu_server_handlers);

    /** Platform Manager Client Factory **/

    // Create UDS address where the server socket will listen for incoming connection requests.
    std::string platform_manager_uds_path = config.temp_path + std::string(BEEROCKS_PLATFORM_UDS);

    // Create CMDU client factory to create CMDU clients connected to CMDU server running in
    // platform manager when requested
    m_platform_manager_cmdu_client_factory =
        std::move(beerocks::create_cmdu_client_factory(platform_manager_uds_path, m_event_loop));
    LOG_IF(!m_platform_manager_cmdu_client_factory, FATAL)
        << "Unable to create CMDU client factory!";

    /** Backhaul Manager Client Factory **/

    // Create UDS address where the server socket will listen for incoming connection requests.
    std::string backhaul_manager_uds_path = config.temp_path + std::string(BEEROCKS_BACKHAUL_UDS);

    // Create CMDU client factory to create CMDU clients connected to CMDU server running in
    // platform manager when requested
    m_backhaul_manager_cmdu_client_factory =
        std::move(beerocks::create_cmdu_client_factory(backhaul_manager_uds_path, m_event_loop));
    LOG_IF(!m_backhaul_manager_cmdu_client_factory, FATAL)
        << "Unable to create CMDU client factory!";

    /** FSM Timer **/

    // Create timer factory to create instances of timers.
    auto timer_factory = std::make_shared<beerocks::TimerFactoryImpl>();
    LOG_IF(!timer_factory, FATAL) << "Unable to create timer factory!";

    // Create timer manager to help using application timers.
    m_timer_manager = std::make_shared<beerocks::TimerManagerImpl>(timer_factory, m_event_loop);
    LOG_IF(!m_timer_manager, FATAL) << "Unable to create timer manager!";

    // Create a timer to run the FSM periodically
    constexpr auto fsm_timer_period = std::chrono::milliseconds(200);
    m_fsm_timer = m_timer_manager->add_timer("Agent FSM", fsm_timer_period, fsm_timer_period,
                                             [&](int fd, beerocks::EventLoop &loop) {
                                                 fsm_all();
                                                 return true;
                                             });
    LOG_IF(m_fsm_timer == beerocks::net::FileDescriptor::invalid_descriptor, FATAL)
        << "Failed to create the FSM timer";

    LOG(DEBUG) << "FSM timer created with fd=" << m_fsm_timer;

    // Create a timer to run internal tasks periodically
    constexpr auto tasks_timer_period = std::chrono::milliseconds(500);

    m_tasks_timer = m_timer_manager->add_timer(
        "Agent Tasks", tasks_timer_period, tasks_timer_period,
        [&](int fd, beerocks::EventLoop &loop) {
            // Allow tasks to execute up to 80% of the timer period
            m_task_pool.run_tasks(int(double(tasks_timer_period.count()) * 0.8));
            return true;
        });

    if (m_tasks_timer == beerocks::net::FileDescriptor::invalid_descriptor) {
        LOG(FATAL) << "Failed to create the tasks timer";
        return false;
    }

    m_task_pool.add_task(std::make_shared<ApAutoConfigurationTask>(*this, cmdu_tx));
    m_task_pool.add_task(std::make_shared<ServicePrioritizationTask>(*this, cmdu_tx));

    m_agent_state = STATE_INIT;
    LOG(DEBUG) << "Agent Started";

    return true;
}

void slave_thread::stop_slave_thread()
{
    agent_reset();
    should_stop = true;
}

void slave_thread::agent_reset()
{
    // If already during reset, return.
    if (m_agent_state < eSlaveState::STATE_LOAD_PLATFORM_CONFIGURATION) {
        return;
    }

    m_agent_resets_counter++;
    LOG(DEBUG) << "agent_reset() #" << m_agent_resets_counter;

    m_radio_managers.do_on_each_radio_manager([&](const sManagedRadio &radio_manager,
                                                  const std::string &fronthaul_iface) {
        auto db = AgentDB::get();

        if (db->device_conf.stop_on_failure_attempts && !radio_manager.stop_on_failure_attempts) {
            LOG(ERROR) << "Reached to max stop on failure attempts!";
            m_stopped = true;
        }

        // If stopped, or backhaul disconnected close the fronthaul.
        if (m_stopped || m_is_backhaul_disconnected) {
            fronthaul_stop(fronthaul_iface);
        }
        return true;
    });

    // If stopped, move to STATE_STOPPED.
    if (m_stopped) {
        LOG(DEBUG) << "goto STATE_STOPPED";
        m_agent_state = STATE_STOPPED;
        platform_notify_error(beerocks::bpl::eErrorCode::SLAVE_STOPPED, "");
        return;
    }

    if (m_is_backhaul_disconnected) {
        m_agent_state_timer_sec =
            std::chrono::steady_clock::now() + std::chrono::seconds(SLAVE_INIT_DELAY_SEC);
        LOG(DEBUG) << "goto STATE_WAIT_BEFORE_INIT";
        m_agent_state = STATE_WAIT_BEFORE_INIT;
        return;
    }

    LOG(DEBUG) << "goto STATE_INIT";
    m_agent_state = STATE_INIT;
}

bool slave_thread::read_platform_configuration()
{
    auto db = AgentDB::get();

    char security_type[beerocks::message::WIFI_SECURITY_TYPE_MAX_LENGTH];
    if (bpl::cfg_get_beerocks_credentials(BPL_RADIO_FRONT, db->device_conf.front_radio.ssid,
                                          db->device_conf.front_radio.pass, security_type) < 0) {
        LOG(ERROR) << "Failed reading front Wi-Fi credentials!";
        return false;
    }

    const auto platform_to_bwl_security = [](const std::string &sec) -> bwl::WiFiSec {
        if (!sec.compare("None")) {
            return bwl::WiFiSec::None;
        } else if (!sec.compare("WEP-64")) {
            return bwl::WiFiSec::WEP_64;
        } else if (!sec.compare("WEP-128")) {
            return bwl::WiFiSec::WEP_128;
        } else if (!sec.compare("WPA-Personal")) {
            return bwl::WiFiSec::WPA_PSK;
        } else if (!sec.compare("WPA2-Personal")) {
            return bwl::WiFiSec::WPA2_PSK;
        } else if (!sec.compare("WPA-WPA2-Personal")) {
            return bwl::WiFiSec::WPA_WPA2_PSK;
        } else {
            return bwl::WiFiSec::Invalid;
        }
    };

    db->device_conf.front_radio.security_type = platform_to_bwl_security(security_type);

    LOG(DEBUG) << "Front Credentials:"
               << " ssid=" << db->device_conf.front_radio.ssid
               << " sec=" << db->device_conf.front_radio.security_type << " pass=***";

    char ssid[beerocks::message::WIFI_SSID_MAX_LENGTH];
    char pass[beerocks::message::WIFI_PASS_MAX_LENGTH];
    if (bpl::cfg_get_beerocks_credentials(BPL_RADIO_BACK, ssid, pass, security_type) < 0) {
        LOG(ERROR) << "Failed reading Wi-Fi back credentials!";
        return false;
    }
    db->device_conf.back_radio.ssid = std::string(ssid, beerocks::message::WIFI_SSID_MAX_LENGTH);
    db->device_conf.back_radio.pass = std::string(pass, beerocks::message::WIFI_PASS_MAX_LENGTH);
    db->device_conf.back_radio.security_type = platform_to_bwl_security(security_type);

    int mem_only_psk = bpl::cfg_get_security_policy();
    if (mem_only_psk < 0) {
        LOG(ERROR) << "Failed reading Wi-Fi security policy!";
        return false;
    }

    db->device_conf.back_radio.mem_only_psk = bool(mem_only_psk);

    LOG(DEBUG) << "Back Credentials:"
               << " ssid=" << db->device_conf.back_radio.ssid
               << " sec=" << db->device_conf.back_radio.security_type
               << " mem_only_psk=" << db->device_conf.back_radio.mem_only_psk << " pass=***";

    for (const auto &radio_manager : m_radio_managers.get()) {
        const auto &fronthaul_iface = radio_manager.first;
        bpl::BPL_WLAN_PARAMS params;
        if (bpl::cfg_get_wifi_params(fronthaul_iface.c_str(), &params) < 0) {
            LOG(ERROR) << "Failed reading '" << fronthaul_iface << "' parameters!";
            return false;
        }

        db->device_conf.front_radio.config[fronthaul_iface].band_enabled       = params.enabled;
        db->device_conf.front_radio.config[fronthaul_iface].configured_channel = params.channel;
        db->device_conf.front_radio.config[fronthaul_iface].sub_band_dfs = params.sub_band_dfs;

        CountryCode current_country;
        current_country[0] = params.country_code[0];
        current_country[1] = params.country_code[1];

        bool db_country_code_empty =
            (db->device_conf.country_code[0] == 0) && (db->device_conf.country_code[1] == 0);

        if (current_country != db->device_conf.country_code && !db_country_code_empty) {
            LOG(ERROR) << "strangely enough this agent exists in more than one country: "
                       << current_country[0] << current_country[1] << " and "
                       << db->device_conf.country_code[0] << db->device_conf.country_code[1];
        }
        // take the latest
        db->device_conf.country_code = current_country;

        LOG(DEBUG) << "wlan settings " << fronthaul_iface << ":";
        LOG(DEBUG) << "band_enabled=" << params.enabled;
        LOG(DEBUG) << "channel=" << params.channel;
        LOG(DEBUG) << "sub_band_dfs=" << params.sub_band_dfs;
        LOG(DEBUG) << "country-code="
                   << (!db_country_code_empty ? std::string(&db->device_conf.country_code[0], 2)
                                              : "(not set)");

        LOG(DEBUG) << "iface=" << fronthaul_iface << " added to wlan params change check";
    }

    const int back_vaps_buff_len =
        BPL_BACK_VAPS_GROUPS * BPL_BACK_VAPS_IN_GROUP * BPL_MAC_ADDR_OCTETS_LEN;
    char back_vaps[back_vaps_buff_len];

    int temp_int;

    if ((temp_int = bpl::cfg_get_rdkb_extensions()) < 0) {
        LOG(ERROR) << "Failed reading 'rdkb_extensions'";
        return false;
    }
    db->device_conf.rdkb_extensions_enabled = static_cast<bool>(temp_int);

    if (!bpl::cfg_get_band_steering(db->device_conf.client_band_steering_enabled)) {
        LOG(DEBUG) << "Failed to read cfg_get_band_steering, setting to default value: "
                   << beerocks::bpl::DEFAULT_BAND_STEERING;

        db->device_conf.client_band_steering_enabled = beerocks::bpl::DEFAULT_BAND_STEERING;
    }

    if (!beerocks::bpl::cfg_get_client_roaming(
            db->device_conf.client_optimal_path_roaming_enabled)) {
        LOG(DEBUG) << "Failed to read cfg_get_client_roaming, setting to default value: "
                   << beerocks::bpl::DEFAULT_CLIENT_ROAMING;

        db->device_conf.client_optimal_path_roaming_enabled = beerocks::bpl::DEFAULT_CLIENT_ROAMING;
    }

    if ((temp_int = bpl::cfg_is_master()) < 0) {
        LOG(ERROR) << "Failed reading 'local_controller'";
        return false;
    }
    db->device_conf.local_controller = temp_int;
    if ((temp_int = bpl::cfg_get_operating_mode()) < 0) {
        LOG(ERROR) << "Failed reading 'operating_mode'";
        return false;
    }
    db->device_conf.operating_mode = uint8_t(temp_int);

    if ((temp_int = bpl::cfg_get_certification_mode()) < 0) {
        LOG(ERROR) << "Failed reading 'certification_mode'";
        return false;
    }
    db->device_conf.certification_mode = temp_int;

    if ((temp_int = bpl::cfg_get_stop_on_failure_attempts()) < 0) {
        LOG(ERROR) << "Failed reading 'stop_on_failure_attempts'";
        return false;
    }
    db->device_conf.stop_on_failure_attempts = temp_int;

    int backhaul_max_vaps;
    int backhaul_network_enabled;
    int backhaul_preferred_radio_band;
    if (bpl::cfg_get_backhaul_params(&backhaul_max_vaps, &backhaul_network_enabled,
                                     &backhaul_preferred_radio_band) < 0) {
        LOG(ERROR) << "Failed reading 'backhaul_max_vaps, backhaul_network_enabled, "
                      "backhaul_preferred_radio_band'!";
    }
    db->device_conf.back_radio.backhaul_max_vaps = static_cast<uint8_t>(backhaul_max_vaps);
    db->device_conf.back_radio.backhaul_network_enabled =
        static_cast<bool>(backhaul_network_enabled);

    const auto bpl_band_to_freq_type = [](int bpl_band) -> beerocks::eFreqType {
        if (bpl_band == BPL_RADIO_BAND_2G) {
            return beerocks::eFreqType::FREQ_24G;
        } else if (bpl_band == BPL_RADIO_BAND_5G) {
            return beerocks::eFreqType::FREQ_5G;
        } else if (bpl_band == BPL_RADIO_BAND_AUTO) {
            return beerocks::eFreqType::FREQ_AUTO;
        } else {
            return beerocks::eFreqType::FREQ_UNKNOWN;
        }
    };
    db->device_conf.back_radio.backhaul_preferred_radio_band =
        bpl_band_to_freq_type(backhaul_preferred_radio_band);

    if (bpl::cfg_get_backhaul_vaps(back_vaps, back_vaps_buff_len) < 0) {
        LOG(ERROR) << "Failed reading beerocks backhaul_vaps parameters!";
        return false;
    }

    if (!bpl::cfg_get_zwdfs_enable(db->device_conf.zwdfs_enable)) {
        LOG(WARNING) << "cfg_get_zwdfs_enable() failed!, using default configuration, zwdfs is "
                     << (db->device_conf.zwdfs_enable ? std::string("enabled.")
                                                      : std::string("disabled."));
    }

    if (!bpl::cfg_get_best_channel_rank_threshold(db->device_conf.best_channel_rank_threshold)) {
        LOG(WARNING) << "cfg_get_best_channel_rank_threshold() failed!"
                     << " using default configuration ";
    }

    if (!bpl::get_max_prioritization_rules(db->device_conf.max_prioritization_rules)) {
        LOG(WARNING) << "get_max_prioritization_rules() failed!"
                     << " using default configuration ";
    }

    // Set local_gw flag
    db->device_conf.local_gw = (db->device_conf.operating_mode == BPL_OPER_MODE_GATEWAY ||
                                db->device_conf.operating_mode == BPL_OPER_MODE_GATEWAY_WISP);

    db->device_conf.client_optimal_path_roaming_prefer_signal_strength_enabled =
        0; // TODO add platform DB flag
    db->device_conf.client_11k_roaming_enabled =
        (db->device_conf.client_optimal_path_roaming_enabled ||
         db->device_conf.client_band_steering_enabled);

    db->device_conf.load_balancing_enabled   = 0; // for v1.3 TODO read from CAL DB
    db->device_conf.service_fairness_enabled = 0; // for v1.3 TODO read from CAL DB

    std::vector<std::string> lan_iface_list;
    if (beerocks::bpl::bpl_get_lan_interfaces(lan_iface_list)) {

        db->ethernet.lan.clear();

        std::string iface_mac;
        for (const auto &lan_iface : lan_iface_list) {

            if (beerocks::net::network_utils::linux_iface_get_mac(lan_iface, iface_mac)) {
                db->ethernet.lan.emplace_back(lan_iface, tlvf::mac_from_string(iface_mac));
            }
        }
    }
    LOG(DEBUG) << "client_band_steering_enabled: " << db->device_conf.client_band_steering_enabled;
    LOG(DEBUG) << "client_optimal_path_roaming_enabled: "
               << db->device_conf.client_optimal_path_roaming_enabled;
    LOG(DEBUG) << "client_optimal_path_roaming_prefer_signal_strength_enabled: "
               << db->device_conf.client_optimal_path_roaming_prefer_signal_strength_enabled;
    LOG(DEBUG) << "local_gw: " << db->device_conf.local_gw;
    LOG(DEBUG) << "local_controller: " << db->device_conf.local_controller;
    LOG(DEBUG) << "backhaul_preferred_radio_band: "
               << db->device_conf.back_radio.backhaul_preferred_radio_band;
    LOG(DEBUG) << "rdkb_extensions: " << db->device_conf.rdkb_extensions_enabled;
    LOG(DEBUG) << "zwdfs_enable: " << db->device_conf.zwdfs_enable;
    LOG(DEBUG) << "best_channel_rank_threshold: " << db->device_conf.best_channel_rank_threshold;
    LOG(DEBUG) << "max_prioritization_rules: " << db->device_conf.max_prioritization_rules;

    return true;
}

void slave_thread::platform_notify_error(beerocks::bpl::eErrorCode code,
                                         const std::string &error_data)
{
    if (!m_platform_manager_client) {
        LOG(ERROR) << "Invalid Platform Manager socket!";
        return;
    }

    auto error =
        message_com::create_vs_message<beerocks_message::cACTION_PLATFORM_ERROR_NOTIFICATION>(
            cmdu_tx);

    if (error == nullptr) {
        LOG(ERROR) << "Failed building message!";
        return;
    }

    error->code() = uint32_t(code);
    string_utils::copy_string(error->data(0), error_data.c_str(),
                              message::PLATFORM_ERROR_DATA_SIZE);

    // Send the message
    m_platform_manager_client->send_cmdu(cmdu_tx);
}

void slave_thread::on_thread_stop() { stop_slave_thread(); }

void slave_thread::handle_client_disconnected(int fd)
{

    auto handle_disconnect = [&](const std::string &fronthaul_iface) {
        auto &radio_manager = m_radio_managers[fronthaul_iface];

        bool found_fd = false;
        if (fd == radio_manager.ap_manager_fd) {
            LOG(DEBUG) << "AP Manager " << fronthaul_iface << " disconnected";
            radio_manager.ap_manager_fd = net::FileDescriptor::invalid_descriptor;
            if (radio_manager.monitor_fd != net::FileDescriptor::invalid_descriptor) {
                m_cmdu_server->disconnect(radio_manager.monitor_fd);
            }
            found_fd = true;
        } else if (fd == radio_manager.monitor_fd) {
            LOG(DEBUG) << "Monitor " << fronthaul_iface << " disconnected";
            radio_manager.monitor_fd = net::FileDescriptor::invalid_descriptor;
            if (radio_manager.ap_manager_fd != net::FileDescriptor::invalid_descriptor) {
                m_cmdu_server->disconnect(radio_manager.ap_manager_fd);
            }
            found_fd = true;
        }
        if (found_fd) {
            LOG(DEBUG) << "agent_reset!";
            radio_manager.fronthaul_started = false;
            agent_reset();
            return true;
        }
        return false;
    };

    for (auto &radio_manager_map_element : m_radio_managers.get()) {
        const auto &fronthaul_iface = radio_manager_map_element.first;
        if (handle_disconnect(fronthaul_iface)) {
            return;
        }
    }

    const auto &zwdfs_radio_manager = m_radio_managers.get_zwdfs();
    if (zwdfs_radio_manager) {
        if (handle_disconnect(zwdfs_radio_manager->first)) {
            return;
        }
    }
    LOG(INFO) << "Uknown client socket disconnected!";
}

bool slave_thread::fsm_all()
{
    auto radio_fsm = [&](const sManagedRadio &radio_manager, const std::string &fronthaul_iface) {
        if (!monitor_heartbeat_check(fronthaul_iface) ||
            !ap_manager_heartbeat_check(fronthaul_iface)) {
            fronthaul_reset(radio_manager);
        }
        return true;
    };

    // Running the FSM only if not on STATE_OPERATIONAL, to perform this state only once.
    // The state will execute once from the state STATE_WAIT_FOR_AUTO_CONFIGURATION_COMPLETE once
    // after the Agent is configured.
    if (m_agent_state != eSlaveState::STATE_OPERATIONAL) {
        if (!agent_fsm()) {
            return false;
        }
    } else {
        if (!m_radio_managers.do_on_each_radio_manager(radio_fsm)) {
            return false;
        }
    }

    return true;
}

bool slave_thread::handle_cmdu(int fd, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    // Find for each radio sockets the message has received.
    std::string fronthaul_iface;
    for (auto &radio_manager_map_element : m_radio_managers.get()) {
        auto &radio_manager = radio_manager_map_element.second;
        if (fd == radio_manager.ap_manager_fd || fd == radio_manager.monitor_fd) {
            fronthaul_iface = radio_manager_map_element.first;
        }
    }

    if (cmdu_rx.getMessageType() == ieee1905_1::eMessageType::VENDOR_SPECIFIC_MESSAGE) {

        const auto &zwdfs_radio_manager = m_radio_managers.get_zwdfs();
        if (fronthaul_iface.empty() && zwdfs_radio_manager) {
            fronthaul_iface = zwdfs_radio_manager->first;
        }

        auto beerocks_header = message_com::parse_intel_vs_message(cmdu_rx);

        if (!beerocks_header) {
            LOG(ERROR) << "Not a vendor specific message";
            return false;
        }

        switch (beerocks_header->action()) {
        case beerocks_message::ACTION_CONTROL: {
            return handle_cmdu_control_message(fd, beerocks_header);
        } break;
        case beerocks_message::ACTION_BACKHAUL: {
            return handle_cmdu_backhaul_manager_message(fd, beerocks_header);
        } break;
        case beerocks_message::ACTION_PLATFORM: {
            return handle_cmdu_platform_manager_message(fd, beerocks_header);
        } break;
        case beerocks_message::ACTION_APMANAGER: {
            return handle_cmdu_ap_manager_message(fronthaul_iface, fd, beerocks_header);
        } break;
        case beerocks_message::ACTION_MONITOR: {
            return handle_cmdu_monitor_message(fronthaul_iface, fd, beerocks_header);
        } break;
        default: {
            LOG(ERROR) << "Unknown message, action: " << int(beerocks_header->action());
        }
        }
    } else if (!fronthaul_iface.empty() && fd == m_radio_managers[fronthaul_iface].ap_manager_fd) {
        // Handle IEEE 1905.1 messages from the AP Manager
        return handle_cmdu_ap_manager_ieee1905_1_message(fronthaul_iface, fd, cmdu_rx);
    } else if (!fronthaul_iface.empty() && fd == m_radio_managers[fronthaul_iface].monitor_fd) {
        // Handle IEEE 1905.1 messages from the Monitor
        return handle_cmdu_monitor_ieee1905_1_message(fronthaul_iface, fd, cmdu_rx);
    } else {
        // Handle IEEE 1905.1 messages from the Controller
        return handle_cmdu_control_ieee1905_1_message(fd, cmdu_rx);
    }
    return true;
}

bool slave_thread::handle_cmdu_from_broker(uint32_t iface_index, const sMacAddr &dst_mac,
                                           const sMacAddr &src_mac,
                                           ieee1905_1::CmduMessageRx &cmdu_rx)
{
    {
        auto db = AgentDB::get();
        // Filter messages which are not destined to this agent
        if (dst_mac != beerocks::net::network_utils::MULTICAST_1905_MAC_ADDR &&
            dst_mac != db->bridge.mac) {
            LOG(DEBUG) << "handle_cmdu() - dropping msg, dst_mac=" << dst_mac
                       << ", local_bridge_mac=" << db->bridge.mac;
            return true;
        }
    }

    switch (cmdu_rx.getMessageType()) {
    case ieee1905_1::eMessageType::VENDOR_SPECIFIC_MESSAGE: {
        auto beerocks_header = message_com::parse_intel_vs_message(cmdu_rx);

        if (!beerocks_header) {
            LOG(ERROR) << "Not a vendor specific message";
            return false;
        }

        if (beerocks_header->action() != beerocks_message::ACTION_CONTROL) {
            LOG(ERROR) << "Unknown message, action: " << std::hex << int(beerocks_header->action());
            return false;
        }

        m_task_pool.handle_cmdu(cmdu_rx, iface_index, dst_mac, src_mac,
                                beerocks::net::FileDescriptor::invalid_descriptor, beerocks_header);
        return true;
    }
    default: {
        m_task_pool.handle_cmdu(cmdu_rx, iface_index, dst_mac, src_mac,
                                beerocks::net::FileDescriptor::invalid_descriptor);
    }
    }
    return true;
}

////////////////////////////////////////////////////////////////////////
////////////////////////// HANDLE CMDU ACTIONS /////////////////////////
////////////////////////////////////////////////////////////////////////

bool slave_thread::handle_cmdu_control_ieee1905_1_message(int fd,
                                                          ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto cmdu_message_type = cmdu_rx.getMessageType();

    switch (cmdu_message_type) {
    case ieee1905_1::eMessageType::ACK_MESSAGE:
        return handle_ack_message(fd, cmdu_rx);
    case ieee1905_1::eMessageType::AP_METRICS_QUERY_MESSAGE:
        return handle_ap_metrics_query(fd, cmdu_rx);
    case ieee1905_1::eMessageType::BEACON_METRICS_QUERY_MESSAGE:
        return handle_beacon_metrics_query(fd, cmdu_rx);
    case ieee1905_1::eMessageType::CHANNEL_PREFERENCE_QUERY_MESSAGE:
        return handle_channel_preference_query(fd, cmdu_rx);
    case ieee1905_1::eMessageType::CHANNEL_SELECTION_REQUEST_MESSAGE:
        return handle_channel_selection_request(fd, cmdu_rx);
    case ieee1905_1::eMessageType::CLIENT_ASSOCIATION_CONTROL_REQUEST_MESSAGE:
        return handle_client_association_request(fd, cmdu_rx);
    case ieee1905_1::eMessageType::CLIENT_STEERING_REQUEST_MESSAGE:
        return handle_client_steering_request(fd, cmdu_rx);
    case ieee1905_1::eMessageType::HIGHER_LAYER_DATA_MESSAGE:
        return handle_1905_higher_layer_data_message(fd, cmdu_rx);
    default:
        LOG(ERROR) << "Unknown CMDU message type: " << std::hex << int(cmdu_message_type);
        return false;
    }

    return true;
}

bool slave_thread::handle_cmdu_ap_manager_ieee1905_1_message(const std::string &fronthaul_iface,
                                                             int fd,
                                                             ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto cmdu_message_type = cmdu_rx.getMessageType();
    switch (cmdu_message_type) {
    // Forward unhandled messages to the backhaul manager (probably headed to the controller)
    default:
        const auto mid = cmdu_rx.getMessageId();
        LOG(DEBUG) << "Forwarding ieee1905 message " << int(cmdu_message_type)
                   << " to backhaul_manager, mid = " << std::hex << int(mid);

        if (!m_backhaul_manager_client->forward_cmdu(cmdu_rx)) {
            LOG(ERROR) << "Failed forwarding message 0x" << std::hex << int(cmdu_message_type)
                       << " to backhaul_manager";

            return false;
        }
    }

    return true;
}

bool slave_thread::handle_cmdu_monitor_ieee1905_1_message(const std::string &fronthaul_iface,
                                                          int fd,
                                                          ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto cmdu_message_type = cmdu_rx.getMessageType();
    switch (cmdu_message_type) {
    case ieee1905_1::eMessageType::AP_METRICS_RESPONSE_MESSAGE:
        return handle_monitor_ap_metrics_response(fronthaul_iface, fd, cmdu_rx);
    default:
        LOG(ERROR) << "Unknown CMDU message type: " << std::hex << int(cmdu_message_type);
        return false;
    }
}

bool slave_thread::handle_cmdu_control_message(int fd,
                                               std::shared_ptr<beerocks_header> beerocks_header)
{
    if (beerocks_header->actionhdr()->direction() == beerocks::BEEROCKS_DIRECTION_CONTROLLER) {
        return true;
    }

    // Scope this code block to prevent shadowing of "db" and "radio" variables internally on the
    // switch case.
    std::string fronthaul_iface;
    {
        auto db    = AgentDB::get();
        auto radio = db->get_radio_by_mac(beerocks_header->actionhdr()->radio_mac(),
                                          AgentDB::eMacType::RADIO);
        if (!radio) {
            LOG(DEBUG) << "Radio " << beerocks_header->actionhdr()->radio_mac()
                       << " does not exist on the db";
            return false;
        }

        fronthaul_iface = radio->front.iface_name;

        // ZWDFS Radio should ignore messages from the Controller
        if (radio->front.zwdfs) {
            return true;
        }
    }

    auto &radio_manager = m_radio_managers[fronthaul_iface];

    if (m_agent_state == STATE_STOPPED) {
        return true;
    }

    switch (beerocks_header->action_op()) {
    case beerocks_message::ACTION_CONTROL_ARP_QUERY_REQUEST: {
        LOG(TRACE) << "ACTION_CONTROL_ARP_QUERY_REQUEST";
        auto request_in =
            beerocks_header->addClass<beerocks_message::cACTION_CONTROL_ARP_QUERY_REQUEST>();
        if (request_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_ARP_QUERY_REQUEST failed";
            return false;
        }
        auto request_out =
            message_com::create_vs_message<beerocks_message::cACTION_PLATFORM_ARP_QUERY_REQUEST>(
                cmdu_tx, beerocks_header->id());
        if (request_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        // notify platform manager
        request_out->params() = request_in->params();
        m_platform_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_REQUEST: {
        LOG(DEBUG) << "received ACTION_CONTROL_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_REQUEST";

        auto request_in = beerocks_header->addClass<
            beerocks_message::cACTION_CONTROL_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_REQUEST>();
        if (request_in == nullptr) {
            LOG(ERROR)
                << "addClass cACTION_CONTROL_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_REQUEST failed";
            return false;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_REQUEST>(
            cmdu_tx);
        if (request_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        LOG(DEBUG) << "send ACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_REQUEST";
        request_out->params() = request_in->params();
        send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_CHANNEL_SWITCH_ACS_START: {
        LOG(DEBUG) << "received ACTION_CONTROL_HOSTAP_CHANNEL_SWITCH_ACS_START";
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_HOSTAP_CHANNEL_SWITCH_ACS_START>();
        if (request_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_HOSTAP_CHANNEL_SWITCH_ACS_START failed";
            return false;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START>(
            cmdu_tx, beerocks_header->id());
        if (request_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        LOG(DEBUG) << "send cACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START";
        request_out->cs_params() = request_in->cs_params();
        send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_CONTROL_CLIENT_START_MONITORING_REQUEST: {
        LOG(DEBUG) << "received ACTION_CONTROL_CLIENT_START_MONITORING_REQUEST";
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CLIENT_START_MONITORING_REQUEST>();
        if (request_in == nullptr) {
            LOG(ERROR) << "addClass ACTION_CONTROL_CLIENT_START_MONITORING_REQUEST failed";
            return false;
        }

        std::string client_mac = tlvf::mac_to_string(request_in->params().mac);
        std::string client_bridge_4addr_mac =
            tlvf::mac_to_string(request_in->params().bridge_4addr_mac);
        std::string client_ip = network_utils::ipv4_to_string(request_in->params().ipv4);

        LOG(DEBUG) << "START_MONITORING_REQUEST: mac=" << client_mac << " ip=" << client_ip
                   << " bridge_4addr_mac=" << client_bridge_4addr_mac;

        //notify monitor
        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CLIENT_START_MONITORING_REQUEST>(
            cmdu_tx, beerocks_header->id());
        if (request_out == nullptr) {
            LOG(ERROR) << "Failed building ACTION_MONITOR_CLIENT_START_MONITORING_REQUEST message!";
            return false;
        }
        request_out->params() = request_in->params();
        send_cmdu(radio_manager.monitor_fd, cmdu_tx);
        break;
    }

    case beerocks_message::ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST: {
        LOG(DEBUG) << "received ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST";

        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST>();
        if (request_in == nullptr) {
            LOG(ERROR) << "addClass ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST failed";
            return false;
        }

        auto db = AgentDB::get();
        if (request_in->params().cross && (request_in->params().ipv4.oct[0] == 0) &&
            db->backhaul.connection_type == AgentDB::sBackhaul::eConnectionType::Wireless) {
            auto request_out = message_com::create_vs_message<
                beerocks_message::cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST>(
                cmdu_tx, beerocks_header->id());
            if (request_out == nullptr) {
                LOG(ERROR) << "Failed building ACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST "
                              "message!";
                return false;
            }

            request_out->params() = request_in->params();
            m_backhaul_manager_client->send_cmdu(cmdu_tx);
        } else if (request_in->params().cross &&
                   (request_in->params().ipv4.oct[0] ==
                    0)) { // unconnected client cross --> send to ap_manager
            auto request_out = message_com::create_vs_message<
                beerocks_message::cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_REQUEST>(
                cmdu_tx, beerocks_header->id());
            if (request_out == nullptr) {
                LOG(ERROR) << "Failed building ACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_REQUEST "
                              "message!";
                return false;
            }
            request_out->params() = request_in->params();
            send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
        } else {
            auto request_out = message_com::create_vs_message<
                beerocks_message::cACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_REQUEST>(
                cmdu_tx, beerocks_header->id());
            if (request_out == nullptr) {
                LOG(ERROR)
                    << "Failed building ACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_REQUEST message!";
                return false;
            }
            request_out->params() = request_in->params();
            send_cmdu(radio_manager.monitor_fd, cmdu_tx);
        }

        LOG(INFO) << "rx_rssi measurement request for client mac=" << request_in->params().mac
                  << " ip=" << network_utils::ipv4_to_string(request_in->params().ipv4)
                  << " channel=" << int(request_in->params().channel) << " bandwidth="
                  << utils::convert_bandwidth_to_int(
                         (beerocks::eWiFiBandwidth)request_in->params().bandwidth)
                  << " cross=" << int(request_in->params().cross)
                  << " id=" << int(beerocks_header->id());
        break;
    }
    case beerocks_message::ACTION_CONTROL_CLIENT_DISCONNECT_REQUEST: {
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CLIENT_DISCONNECT_REQUEST>();
        if (request_in == nullptr) {
            LOG(ERROR) << "addClass ACTION_CONTROL_CLIENT_DISCONNECT_REQUEST failed";
            return false;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_CLIENT_DISCONNECT_REQUEST>(cmdu_tx,
                                                                           beerocks_header->id());
        if (request_out == nullptr) {
            LOG(ERROR) << "Failed building ACTION_APMANAGER_CLIENT_DISCONNECT_REQUEST message!";
            return false;
        }

        request_out->mac()    = request_in->mac();
        request_out->vap_id() = request_in->vap_id();
        request_out->type()   = request_in->type();
        request_out->reason() = request_in->reason();

        send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_CONTROL_CLIENT_NEW_IP_ADDRESS_NOTIFICATION: {
        LOG(DEBUG) << "received ACTION_CONTROL_CLIENT_NEW_IP_ADDRESS_NOTIFICATION";
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CLIENT_NEW_IP_ADDRESS_NOTIFICATION>();
        if (!notification_in) {
            LOG(ERROR) << "addClass ACTION_CONTROL_CLIENT_NEW_IP_ADDRESS_NOTIFICATION failed";
            return false;
        }

        // Notify monitor
        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CLIENT_NEW_IP_ADDRESS_NOTIFICATION>(cmdu_tx);
        if (!notification_out) {
            LOG(ERROR)
                << "Failed building cACTION_MONITOR_CLIENT_NEW_IP_ADDRESS_NOTIFICATION message!";
            return false;
        }

        notification_out->mac()  = notification_in->mac();
        notification_out->ipv4() = notification_in->ipv4();
        send_cmdu(radio_manager.monitor_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_CONTROL_CHANGE_MODULE_LOGGING_LEVEL: {
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CHANGE_MODULE_LOGGING_LEVEL>();
        if (request_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_CHANGE_MODULE_LOGGING_LEVEL failed";
            return false;
        }
        bool all = false;
        if (request_in->params().module_name == beerocks::BEEROCKS_PROCESS_ALL) {
            all = true;
        }
        if (all || request_in->params().module_name == beerocks::BEEROCKS_PROCESS_SLAVE) {
            logger.set_log_level_state((eLogLevel)request_in->params().log_level,
                                       request_in->params().enable);
        }
        if (all || request_in->params().module_name == beerocks::BEEROCKS_PROCESS_MONITOR) {
            auto request_out = message_com::create_vs_message<
                beerocks_message::cACTION_MONITOR_CHANGE_MODULE_LOGGING_LEVEL>(cmdu_tx);
            if (request_out == nullptr) {
                LOG(ERROR) << "Failed building message!";
                return false;
            }
            request_out->params() = request_in->params();
            send_cmdu(radio_manager.monitor_fd, cmdu_tx);
        }
        if (all || request_in->params().module_name == beerocks::BEEROCKS_PROCESS_PLATFORM) {
            auto request_out = message_com::create_vs_message<
                beerocks_message::cACTION_PLATFORM_CHANGE_MODULE_LOGGING_LEVEL>(cmdu_tx);
            if (request_out == nullptr) {
                LOG(ERROR) << "Failed building message!";
                return false;
            }
            request_out->params() = request_in->params();
            m_platform_manager_client->send_cmdu(cmdu_tx);
        }
        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_STATS_MEASUREMENT_REQUEST: {
        if (radio_manager.monitor_fd == beerocks::net::FileDescriptor::invalid_descriptor) {
            break;
        }
        // LOG(TRACE) << "received ACTION_CONTROL_HOSTAP_STATS_MEASUREMENT_REQUEST"; // floods the log
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_HOSTAP_STATS_MEASUREMENT_REQUEST>();
        if (request_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_HOSTAP_STATS_MEASUREMENT_REQUEST failed";
            return false;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_HOSTAP_STATS_MEASUREMENT_REQUEST>(
            cmdu_tx, beerocks_header->id());
        if (request_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        request_out->sync() = request_in->sync();
        send_cmdu(radio_manager.monitor_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_SET_NEIGHBOR_11K_REQUEST: {
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_HOSTAP_SET_NEIGHBOR_11K_REQUEST>();
        if (request_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_HOSTAP_SET_NEIGHBOR_11K_REQUEST failed";
            return false;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_SET_NEIGHBOR_11K_REQUEST>(
            cmdu_tx, beerocks_header->id());
        if (request_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        request_out->params() = request_in->params();
        send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_REMOVE_NEIGHBOR_11K_REQUEST: {
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_HOSTAP_REMOVE_NEIGHBOR_11K_REQUEST>();
        if (request_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_HOSTAP_REMOVE_NEIGHBOR_11K_REQUEST failed";
            return false;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_REMOVE_NEIGHBOR_11K_REQUEST>(
            cmdu_tx, beerocks_header->id());
        if (request_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        request_out->params() = request_in->params();
        send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_CONTROL_CLIENT_BEACON_11K_REQUEST: {
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CLIENT_BEACON_11K_REQUEST>();
        if (request_in == nullptr) {
            LOG(ERROR) << "addClass ACTION_CONTROL_CLIENT_BEACON_11K_REQUEST failed";
            return false;
        }

        auto db = AgentDB::get();

        //LOG(DEBUG) << "ACTION_CONTROL_CLIENT_BEACON_11K_REQUEST";
        // override ssid in case of:
        if (request_in->params().use_optional_ssid &&
            std::string((char *)request_in->params().ssid).empty()) {
            //LOG(DEBUG) << "ssid field is empty! using slave ssid -> " << config.ssid;
            string_utils::copy_string(request_in->params().ssid, db->device_conf.front_radio.ssid,
                                      message::WIFI_SSID_MAX_LENGTH);
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CLIENT_BEACON_11K_REQUEST>(cmdu_tx,
                                                                         beerocks_header->id());
        if (request_out == nullptr) {
            LOG(ERROR) << "Failed building ACTION_MONITOR_CLIENT_BEACON_11K_REQUEST message!";
            return false;
        }
        request_out->params() = request_in->params();

        send_cmdu(radio_manager.monitor_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST: {
        auto request_in = beerocks_header->addClass<
            beerocks_message::cACTION_CONTROL_HOSTAP_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST>();
        if (request_in == nullptr) {
            LOG(ERROR)
                << "addClass cACTION_CONTROL_HOSTAP_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST failed";
            return false;
        }
        auto db = AgentDB::get();

        db->device_conf.stop_on_failure_attempts = request_in->attempts();
        radio_manager.stop_on_failure_attempts   = db->device_conf.stop_on_failure_attempts;
        LOG(DEBUG) << "stop_on_failure_attempts new value: "
                   << db->device_conf.stop_on_failure_attempts;

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST>(cmdu_tx);
        if (request_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        request_out->attempts() = request_in->attempts();
        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_CONTROL_STEERING_CLIENT_SET_GROUP_REQUEST: {
        LOG(TRACE) << "ACTION_CONTROL_STEERING_CLIENT_SET_GROUP_REQUEST";
        auto update =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_STEERING_CLIENT_SET_GROUP_REQUEST>();
        if (update == nullptr) {
            LOG(ERROR) << "addClass failed";
            return false;
        }

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_STEERING_CLIENT_SET_GROUP_REQUEST>(
            cmdu_tx, beerocks_header->id());

        if (notification_out == nullptr) {
            LOG(ERROR)
                << "Failed building cACTION_MONITOR_STEERING_CLIENT_SET_GROUP_REQUEST message!";
            break;
        }
        notification_out->params() = update->params();

        LOG(DEBUG) << std::endl
                   << "remove = " << int(update->params().remove) << std::endl
                   << "steeringGroupIndex = " << update->params().steeringGroupIndex << std::endl
                   << "bssid = " << update->params().cfg.bssid << std::endl
                   << "utilCheckIntervalSec = " << update->params().cfg.utilCheckIntervalSec
                   << std::endl
                   << "utilAvgCount = " << update->params().cfg.utilAvgCount << std::endl
                   << "inactCheckIntervalSec = " << update->params().cfg.inactCheckIntervalSec
                   << std::endl
                   << "inactCheckThresholdSec = " << update->params().cfg.inactCheckThresholdSec
                   << std::endl;

        send_cmdu(radio_manager.monitor_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_CONTROL_STEERING_CLIENT_SET_REQUEST: {
        auto update =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_STEERING_CLIENT_SET_REQUEST>();
        if (update == nullptr) {
            LOG(ERROR) << "addClass failed";
            return false;
        }
        LOG(TRACE) << "ACTION_CONTROL_STEERING_CLIENT_SET_REQUEST for BSSID "
                   << update->params().bssid;

        // send to Monitor
        auto notification_mon_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_STEERING_CLIENT_SET_REQUEST>(cmdu_tx,
                                                                           beerocks_header->id());

        if (notification_mon_out == nullptr) {
            LOG(ERROR) << "Failed building cACTION_MONITOR_STEERING_CLIENT_SET_REQUEST message!";
            break;
        }

        notification_mon_out->params() = update->params();

        send_cmdu(radio_manager.monitor_fd, cmdu_tx);

        // send to AP MANAGER
        auto notification_ap_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_STEERING_CLIENT_SET_REQUEST>(cmdu_tx,
                                                                             beerocks_header->id());

        if (notification_ap_out == nullptr) {
            LOG(ERROR) << "Failed building cACTION_APMANAGER_STEERING_CLIENT_SET_REQUEST message!";
            break;
        }

        notification_ap_out->params() = update->params();

        LOG(DEBUG) << std::endl
                   << "remove = " << notification_ap_out->params().remove << std::endl
                   << "steeringGroupIndex = " << notification_ap_out->params().steeringGroupIndex
                   << std::endl
                   << "client_mac = " << notification_ap_out->params().client_mac << std::endl
                   << "bssid = " << update->params().bssid << std::endl
                   << "config.snrProbeHWM = " << notification_ap_out->params().config.snrProbeHWM
                   << std::endl
                   << "config.snrProbeLWM = " << notification_ap_out->params().config.snrProbeLWM
                   << std::endl
                   << "config.snrAuthHWM = " << notification_ap_out->params().config.snrAuthHWM
                   << std::endl
                   << "config.snrAuthLWM = " << notification_ap_out->params().config.snrAuthLWM
                   << std::endl
                   << "config.snrInactXing = " << notification_ap_out->params().config.snrInactXing
                   << std::endl
                   << "config.snrHighXing = " << notification_ap_out->params().config.snrHighXing
                   << std::endl
                   << "config.snrLowXing = " << notification_ap_out->params().config.snrLowXing
                   << std::endl
                   << "config.authRejectReason = "
                   << notification_ap_out->params().config.authRejectReason << std::endl;

        send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);

        break;
    }
    case beerocks_message::ACTION_CONTROL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST: {
        LOG(TRACE) << "ACTION_CONTROL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST";
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST>();
        if (!request_in) {
            LOG(ERROR) << "addClass cACTION_CONTROL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST failed";
            return false;
        }

        auto db = AgentDB::get();

        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            return false;
        }

        bool radio_5g = wireless_utils::is_frequency_band_5ghz(radio->freq_type);

        // If received scan request and ZWDFS CAC is about to finish refuse to start the
        // background scan only on the 5G radio.
        LOG(DEBUG) << "zwdfs_cac_remaining_time_sec=" << db->statuses.zwdfs_cac_remaining_time_sec;
        if (radio_5g && db->statuses.zwdfs_cac_remaining_time_sec > 0) {
            constexpr uint8_t ETSI_CAC_TIME_SEC = 72; // ETSI CAC time sec (60) * factor of 1.2
            float dwell_time_sec                = request_in->scan_params().dwell_time_ms / 1000.0;
            auto number_of_channel_to_scan      = request_in->scan_params().channel_pool_size;

            constexpr float SCAN_TIME_FACTOR = 89.1;
            // scan time factor (89.1) is calculated in this way:
            // factor * (scan_break_time / slice_size + 1) = 89.1
            // when: factor=1.1, scan_break_time=1600ms, slice_size=20ms
            auto total_scan_time = number_of_channel_to_scan * dwell_time_sec * SCAN_TIME_FACTOR;
            LOG(DEBUG) << "total_scan_time=" << total_scan_time
                       << " on number_of_channels=" << number_of_channel_to_scan;

            if (db->statuses.zwdfs_cac_remaining_time_sec < ETSI_CAC_TIME_SEC &&
                db->statuses.zwdfs_cac_remaining_time_sec < total_scan_time) {
                LOG(DEBUG) << "Refuse DCS scan";
                auto notification = message_com::create_vs_message<
                    beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_ABORTED_NOTIFICATION>(cmdu_tx);
                if (!notification) {
                    LOG(ERROR)
                        << "Failed building cACTION_MONITOR_CHANNEL_SCAN_ABORTED_NOTIFICATION msg";
                    return false;
                }

                send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
                break;
            }
        }

        radio->statuses.channel_scan_in_progress = true;

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST>(cmdu_tx);
        if (!request_out) {
            LOG(ERROR)
                << "Failed building cACTION_MONITOR_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST message!";
            return false;
        }

        request_out->scan_params() = request_in->scan_params();

        LOG(DEBUG) << "send cACTION_MONITOR_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST";
        send_cmdu(radio_manager.monitor_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_CONTROL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST: {
        LOG(TRACE) << "ACTION_CONTROL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST";
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST>();
        if (!request_in) {
            LOG(ERROR) << "addClass cACTION_CONTROL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST failed";
            return false;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_DUMP_RESULTS_REQUEST>(cmdu_tx);
        if (!request_out) {
            LOG(ERROR)
                << "Failed building cACTION_MONITOR_CHANNEL_SCAN_DUMP_RESULTS_REQUEST message!";
            return false;
        }

        LOG(DEBUG) << "send cACTION_MONITOR_CHANNEL_SCAN_DUMP_RESULTS_REQUEST";
        send_cmdu(radio_manager.monitor_fd, cmdu_tx);
        break;
    }
    default: {
        LOG(ERROR) << "Unknown CONTROL message, action_op: " << int(beerocks_header->action_op());
        return false;
    }
    }

    return true;
}

bool slave_thread::handle_cmdu_backhaul_manager_message(
    int fd, std::shared_ptr<beerocks_header> beerocks_header)
{
    if (!m_backhaul_manager_client) {
        LOG(ERROR) << "backhaul_socket == nullptr";
        return true;
    }

    switch (beerocks_header->action_op()) {
    case beerocks_message::ACTION_BACKHAUL_REGISTER_RESPONSE: {
        LOG(DEBUG) << "received ACTION_BACKHAUL_REGISTER_RESPONSE";
        if (m_agent_state == STATE_WAIT_FOR_BACKHAUL_MANAGER_REGISTER_RESPONSE) {
            auto response =
                beerocks_header->addClass<beerocks_message::cACTION_BACKHAUL_REGISTER_RESPONSE>();
            if (!response) {
                LOG(ERROR) << "Failed building message!";
                return false;
            }
            LOG(DEBUG) << "goto STATE_JOIN_INIT";
            m_agent_state = STATE_JOIN_INIT;
        } else {
            LOG(ERROR) << "slave_state != STATE_WAIT_FOR_BACKHAUL_MANAGER_REGISTER_RESPONSE";
        }
        break;
    }

    case beerocks_message::ACTION_BACKHAUL_ENABLE_APS_REQUEST: {
        auto notification_in =
            beerocks_header->addClass<beerocks_message::cACTION_BACKHAUL_ENABLE_APS_REQUEST>();
        if (!notification_in) {
            LOG(ERROR) << "Failed building cACTION_BACKHAUL_ENABLE_APS_REQUEST message!";
            return false;
        }

        auto front_iface_str = notification_in->iface();
        LOG(DEBUG) << "Received ACTION_BACKHAUL_ENABLE_APS_REQUEST iface=" << front_iface_str;

        auto &radio_manager = m_radio_managers[front_iface_str];

        auto notification_out =
            message_com::create_vs_message<beerocks_message::cACTION_APMANAGER_ENABLE_APS_REQUEST>(
                cmdu_tx);
        if (!notification_out) {
            LOG(ERROR) << "Failed building ACTION_APMANAGER_ENABLE_APS_REQUEST message!";
            return false;
        }

        notification_out->channel()        = notification_in->channel();
        notification_out->bandwidth()      = notification_in->bandwidth();
        notification_out->center_channel() = notification_in->center_channel();
        LOG(DEBUG) << "Sending ACTION_APMANAGER_ENABLE_APS_REQUEST";
        send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);

        radio_manager.configuration_in_progress = true;

        break;
    }

    case beerocks_message::ACTION_BACKHAUL_CONNECTED_NOTIFICATION: {

        auto notification =
            beerocks_header->addClass<beerocks_message::cACTION_BACKHAUL_CONNECTED_NOTIFICATION>();
        if (!notification) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        LOG(DEBUG) << "received ACTION_BACKHAUL_CONNECTED_NOTIFICATION";

        if (m_agent_state != STATE_WAIT_FOR_BACKHAUL_MANAGER_CONNECTED_NOTIFICATION) {
            LOG(WARNING) << "Unexpected Backhaul connected notification, Agent state="
                         << m_agent_state;
        }

        LOG(DEBUG) << "goto STATE_BACKHAUL_MANAGER_CONNECTED";
        m_agent_state = STATE_BACKHAUL_MANAGER_CONNECTED;

        break;
    }
    case beerocks_message::ACTION_BACKHAUL_DISCONNECTED_NOTIFICATION: {

        if (m_agent_state <= STATE_JOIN_INIT) {
            break;
        }

        LOG(DEBUG) << "ACTION_BACKHAUL_DISCONNECTED_NOTIFICATION";

        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_BACKHAUL_DISCONNECTED_NOTIFICATION>();
        if (!notification) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        m_is_backhaul_disconnected = true;

        m_stopped |= bool(notification->stopped());

        agent_reset();
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_APPLY_VLAN_POLICY_REQUEST: {
        LOG(DEBUG) << "received ACTION_BACKHAUL_APPLY_VLAN_POLICY_REQUEST";
        TrafficSeparation::apply_traffic_separation();
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE: {
        LOG(DEBUG) << "ACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE";

        auto response_in = beerocks_header->addClass<
            beerocks_message::cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE>();
        if (!response_in) {
            LOG(ERROR)
                << "Failed building ACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE message!";
            return false;
        }

        LOG(DEBUG) << "ACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE mac="
                   << response_in->params().result.mac
                   << " rx_rssi=" << int(response_in->params().rx_rssi)
                   << " id=" << int(beerocks_header->id());

        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE>(
            cmdu_tx, beerocks_header->id());

        if (response_out == nullptr) {
            LOG(ERROR)
                << "Failed building ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE message!";
            break;
        }

        response_out->params()            = response_in->params();
        response_out->params().src_module = beerocks::BEEROCKS_ENTITY_BACKHAUL_MANAGER;

        auto db = AgentDB::get();
        // The Agent send the request message to the Controller only if the backhaul link is
        // wireless. The Controller expects a response from the backhaul manager radio.
        send_cmdu_to_controller(db->backhaul.selected_iface_name, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE: {
        LOG(DEBUG) << "ACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE";
        auto response_in = beerocks_header->addClass<
            beerocks_message::cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE>();
        if (!response_in) {
            LOG(ERROR) << "Failed building ACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE "
                          "message!";
            return false;
        }

        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE>(
            cmdu_tx, beerocks_header->id());

        if (!response_out) {
            LOG(ERROR) << "Failed building ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE "
                          "message!";
            break;
        }
        response_out->mac() = response_in->mac();
        auto db             = AgentDB::get();
        // The Agent send the request message to the Controller only if the backhaul link is
        // wireless. The Controller expects a response from the backhaul manager radio.
        send_cmdu_to_controller(db->backhaul.selected_iface_name, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST: {
        LOG(DEBUG) << "ACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST";
        auto &radio_mac = beerocks_header->actionhdr()->radio_mac();
        auto db         = AgentDB::get();
        auto radio      = db->get_radio_by_mac(radio_mac, AgentDB::eMacType::RADIO);
        if (!radio) {
            break;
        }
        auto &radio_manager = m_radio_managers[radio->front.iface_name];

        if (radio_manager.monitor_fd == beerocks::net::FileDescriptor::invalid_descriptor) {
            LOG(ERROR) << "monitor_fd is invalid";
            return false;
        }
        auto request_in = beerocks_header->addClass<
            beerocks_message::cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST>();
        if (!request_in) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST failed";
            return false;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CLIENT_ASSOCIATED_STA_LINK_METRIC_REQUEST>(
            cmdu_tx, beerocks_header->id());
        if (!request_out) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        request_out->sync()    = request_in->sync();
        request_out->sta_mac() = request_in->sta_mac();
        LOG(DEBUG) << "send ACTION_MONITOR_CLIENT_ASSOCIATED_STA_LINK_METRIC_REQUEST";
        send_cmdu(radio_manager.monitor_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_START_WPS_PBC_REQUEST: {
        auto request_in =
            beerocks_header->addClass<beerocks_message::cACTION_BACKHAUL_START_WPS_PBC_REQUEST>();
        if (!request_in) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_START_WPS_PBC_REQUEST failed";
            return false;
        }
        std::string iface = request_in->iface();

        LOG(DEBUG) << "ACTION_BACKHAUL_START_WPS_PBC_REQUEST iface=" << iface;

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_START_WPS_PBC_REQUEST>(cmdu_tx);

        if (!request_out) {
            LOG(ERROR) << "Failed building message cACTION_APMANAGER_START_WPS_PBC_REQUEST!";
            return false;
        }
        auto &radio_manager = m_radio_managers[iface];
        send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST: {
        auto &radio_mac = beerocks_header->actionhdr()->radio_mac();
        auto db         = AgentDB::get();
        auto radio      = db->get_radio_by_mac(radio_mac, AgentDB::eMacType::RADIO);
        if (!radio) {
            break;
        }
        auto &radio_manager = m_radio_managers[radio->front.iface_name];

        if (radio_manager.ap_manager_fd == beerocks::net::FileDescriptor::invalid_descriptor) {
            LOG(ERROR) << "ap_manager_fd is invalid";
            return false;
        }
        LOG(DEBUG) << "ACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST";
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST>();
        if (!request_in) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST failed";
            return false;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_SET_ASSOC_DISALLOW_REQUEST>(cmdu_tx);
        if (!request_out) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        request_out->enable() = request_in->enable();
        request_out->bssid()  = request_in->bssid();
        LOG(DEBUG) << "send ACTION_APMANAGER_SET_ASSOC_DISALLOW_REQUEST";
        send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_CHANNELS_LIST_REQUEST: {
        auto &radio_mac = beerocks_header->actionhdr()->radio_mac();
        auto db         = AgentDB::get();
        auto radio      = db->get_radio_by_mac(radio_mac, AgentDB::eMacType::RADIO);
        if (!radio) {
            break;
        }
        auto &radio_manager = m_radio_managers[radio->front.iface_name];

        auto request_in =
            beerocks_header->addClass<beerocks_message::cACTION_BACKHAUL_CHANNELS_LIST_REQUEST>();
        if (!request_in) {
            LOG(ERROR) << "Failed building cACTION_BACKHAUL_CHANNELS_LIST_REQUEST "
                          "message!";
            return false;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_CHANNELS_LIST_REQUEST>(cmdu_tx);

        if (!request_out) {
            LOG(ERROR) << "Failed building "
                          "cACTION_APMANAGER_CHANNELS_LIST_REQUEST "
                          "message!";
            return false;
        }
        send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START: {
        auto &radio_mac = beerocks_header->actionhdr()->radio_mac();
        auto db         = AgentDB::get();
        auto radio      = db->get_radio_by_mac(radio_mac, AgentDB::eMacType::RADIO);
        if (!radio) {
            break;
        }
        auto &radio_manager = m_radio_managers[radio->front.iface_name];

        LOG(DEBUG) << "received ACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START";
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START>();
        if (!request_in) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START failed";
            return false;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START>(cmdu_tx);
        if (!request_out) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        LOG(DEBUG) << "send cACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START";
        request_out->cs_params() = request_in->cs_params();
        send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
        break;
    }

    case beerocks_message::ACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST: {
        auto &radio_mac = beerocks_header->actionhdr()->radio_mac();
        auto db         = AgentDB::get();
        auto radio      = db->get_radio_by_mac(radio_mac, AgentDB::eMacType::RADIO);
        if (!radio) {
            break;
        }
        auto &radio_manager = m_radio_managers[radio->front.iface_name];

        LOG(DEBUG) << "received ACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST";
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST>();
        if (!request_in) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST failed";
            return false;
        }

        // we are about to (re)configure
        radio_manager.configuration_in_progress = true;

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST>(cmdu_tx);
        if (!request_out) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        LOG(DEBUG) << "send cACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST";
        request_out->cs_params() = request_in->cs_params();
        send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
        break;
    }

    case beerocks_message::ACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST: {
        LOG(TRACE) << "Received ACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST";

        if (!m_radio_managers.get_zwdfs()) {
            LOG(ERROR) << "ZWDFS radio context is not initialized";
            auto response = message_com::create_vs_message<
                beerocks_message::cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE>(
                cmdu_tx);

            if (!response) {
                LOG(ERROR) << "Failed building "
                              "cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE "
                              "message!";
                return false;
            }

            // Return the response through the first radio context.
            m_backhaul_manager_client->send_cmdu(cmdu_tx);
        }
        auto request_in = beerocks_header->addClass<
            beerocks_message::cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST>();
        if (!request_in) {
            LOG(ERROR)
                << "Failed building cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST "
                   "message!";
            return false;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST>(cmdu_tx);

        if (!request_out) {
            LOG(ERROR) << "Failed building "
                          "cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST "
                          "message!";
            return false;
        }
        request_out->channel()          = request_in->channel();
        request_out->bandwidth()        = request_in->bandwidth();
        request_out->ant_switch_on()    = request_in->ant_switch_on();
        request_out->center_frequency() = request_in->center_frequency();
        send_cmdu(m_radio_managers.get_zwdfs()->second.ap_manager_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_RADIO_DISABLE_REQUEST: {
        auto request_in =
            beerocks_header->addClass<beerocks_message::cACTION_BACKHAUL_RADIO_DISABLE_REQUEST>();
        if (!request_in) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_RADIO_DISABLE_REQUEST failed";
            return false;
        }
        std::string iface = request_in->iface();

        LOG(DEBUG) << "ACTION_BACKHAUL_RADIO_DISABLE_REQUEST iface=" << iface;

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_RADIO_DISABLE_REQUEST>(cmdu_tx);

        if (!request_out) {
            LOG(ERROR) << "Failed building message cACTION_APMANAGER_RADIO_DISABLE_REQUEST!";
            return false;
        }
        auto &radio_manager                     = m_radio_managers[iface];
        radio_manager.configuration_in_progress = true;
        send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST: {
        LOG(DEBUG) << "ACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST";

        ///////////////////////////////////////////////////////////////////
        // Short term solution
        // In non-EasyMesh mode, never modify hostapd configuration
        // and in this case VAPs credentials
        //
        // Long term solution
        // All EasyMesh VAPs will be stored in the platform DB.
        // All other VAPs are manual, AKA should not be modified by prplMesh
        ////////////////////////////////////////////////////////////////////
        auto db = AgentDB::get();
        if (db->device_conf.management_mode == BPL_MGMT_MODE_NOT_MULTIAP) {
            LOG(WARNING) << "non-EasyMesh mode - skip updating VAP credentials";
            break;
        }

        for (const auto &radio_manager_element : m_radio_managers.get()) {
            // Tear down all VAPS in the radio by sending an update request with an empty
            // configuration.
            auto request_out = message_com::create_vs_message<
                beerocks_message::cACTION_APMANAGER_WIFI_CREDENTIALS_UPDATE_REQUEST>(cmdu_tx);
            if (!request_out) {
                LOG(ERROR)
                    << "Failed building message cACTION_APMANAGER_WIFI_CREDENTIALS_UPDATE_REQUEST!";
                return false;
            }

            auto &radio_manager = radio_manager_element.second;
            send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
        }

        break;
    }
    case beerocks_message::ACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST: {
        auto &radio_mac = beerocks_header->actionhdr()->radio_mac();
        auto db         = AgentDB::get();
        auto radio      = db->get_radio_by_mac(radio_mac, AgentDB::eMacType::RADIO);
        if (!radio) {
            break;
        }
        auto &radio_manager = m_radio_managers[radio->front.iface_name];

        LOG(TRACE) << "ACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST";
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST>();
        if (!request_in) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST failed";
            return false;
        }

        bool radio_5g = wireless_utils::is_frequency_band_5ghz(radio->freq_type);

        // If received scan request and ZWDFS CAC is about to finish refuse to start the
        // background scan only on the 5G radio.
        LOG(DEBUG) << "zwdfs_cac_remaining_time_sec=" << db->statuses.zwdfs_cac_remaining_time_sec;
        if (radio_5g && db->statuses.zwdfs_cac_remaining_time_sec > 0) {
            constexpr uint8_t ETSI_CAC_TIME_SEC = 72; // ETSI CAC time sec (60) * factor of 1.2
            float dwell_time_sec                = request_in->scan_params().dwell_time_ms / 1000.0;
            auto number_of_channel_to_scan      = request_in->scan_params().channel_pool_size;

            constexpr float SCAN_TIME_FACTOR = 89.1;
            // scan time factor (89.1) is calculated in this way:
            // factor * (scan_break_time / slice_size + 1) = 89.1
            // when: factor=1.1, scan_break_time=1600ms, slice_size=20ms
            auto total_scan_time = number_of_channel_to_scan * dwell_time_sec * SCAN_TIME_FACTOR;
            LOG(DEBUG) << "total_scan_time=" << total_scan_time
                       << " on number_of_channels=" << number_of_channel_to_scan;

            if (db->statuses.zwdfs_cac_remaining_time_sec < ETSI_CAC_TIME_SEC &&
                db->statuses.zwdfs_cac_remaining_time_sec < total_scan_time) {
                LOG(DEBUG) << "Refuse DCS scan";
                auto notification = message_com::create_vs_message<
                    beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_ABORTED_NOTIFICATION>(cmdu_tx);
                if (!notification) {
                    LOG(ERROR)
                        << "Failed building cACTION_MONITOR_CHANNEL_SCAN_ABORTED_NOTIFICATION msg";
                    return false;
                }

                send_cmdu_to_controller(radio->front.iface_name, cmdu_tx);
                break;
            }
        }

        radio->statuses.channel_scan_in_progress = true;

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST>(cmdu_tx);
        if (!request_out) {
            LOG(ERROR)
                << "Failed building cACTION_MONITOR_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST message!";
            return false;
        }

        request_out->scan_params() = request_in->scan_params();

        LOG(DEBUG) << "send cACTION_MONITOR_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST";
        send_cmdu(radio_manager.monitor_fd, cmdu_tx);
        break;
    }

    case beerocks_message::ACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST: {
        LOG(TRACE) << "ACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST";
        auto &radio_mac = beerocks_header->actionhdr()->radio_mac();
        auto db         = AgentDB::get();
        auto radio      = db->get_radio_by_mac(radio_mac, AgentDB::eMacType::RADIO);
        if (!radio) {
            break;
        }
        auto &radio_manager = m_radio_managers[radio->front.iface_name];

        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST>();
        if (!request_in) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST failed";
            return false;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_DUMP_RESULTS_REQUEST>(cmdu_tx);
        if (!request_out) {
            LOG(ERROR)
                << "Failed building cACTION_MONITOR_CHANNEL_SCAN_DUMP_RESULTS_REQUEST message!";
            return false;
        }

        LOG(DEBUG) << "send cACTION_MONITOR_CHANNEL_SCAN_DUMP_RESULTS_REQUEST";
        send_cmdu(radio_manager.monitor_fd, cmdu_tx);
        break;
    }

    case beerocks_message::ACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST: {
        auto &radio_mac = beerocks_header->actionhdr()->radio_mac();
        auto db         = AgentDB::get();
        auto radio      = db->get_radio_by_mac(radio_mac, AgentDB::eMacType::RADIO);
        if (!radio) {
            break;
        }
        auto &radio_manager = m_radio_managers[radio->front.iface_name];

        LOG(TRACE) << "ACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST";
        auto request_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST>();
        if (!request_in) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST failed";
            return false;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_ABORT_REQUEST>(cmdu_tx);
        if (!request_out) {
            LOG(ERROR) << "Failed building cACTION_MONITOR_CHANNEL_SCAN_ABORT_REQUEST message!";
            return false;
        }

        LOG(DEBUG) << "send cACTION_MONITOR_CHANNEL_SCAN_ABORT_REQUEST";
        send_cmdu(radio_manager.monitor_fd, cmdu_tx);
        break;
    }
    default: {
        LOG(ERROR) << "Unknown BACKHAUL_MANAGER message, action_op: "
                   << int(beerocks_header->action_op());
        return false;
    }
    }

    return true;
}

bool slave_thread::handle_cmdu_platform_manager_message(
    int fd, std::shared_ptr<beerocks_header> beerocks_header)
{
    switch (beerocks_header->action_op()) {
    case beerocks_message::ACTION_PLATFORM_SON_SLAVE_REGISTER_RESPONSE: {
        LOG(TRACE) << "ACTION_PLATFORM_SON_SLAVE_REGISTER_RESPONSE";
        if (m_agent_state == STATE_WAIT_FOR_PLATFORM_MANAGER_REGISTER_RESPONSE) {
            auto response =
                beerocks_header
                    ->addClass<beerocks_message::cACTION_PLATFORM_SON_SLAVE_REGISTER_RESPONSE>();
            if (!response) {
                LOG(ERROR) << "addClass cACTION_PLATFORM_SON_SLAVE_REGISTER_RESPONSE failed";
                return false;
            }

            /**
             * On GW platform the ethernet interface which is used for backhaul connection must be
             * empty since the GW doesn't need wired backhaul connection. Since it is being set on
             * the constructor from the agent configuration file, clear it here when we know if the
             * agent runs on a GW.
             */
            auto db = AgentDB::get();
            if (db->device_conf.local_gw) {
                db->ethernet.wan.iface_name.clear();
                db->ethernet.wan.mac = network_utils::ZERO_MAC;
            }

            // Rest the stop_on_failure_attempts counter
            m_radio_managers.do_on_each_radio_manager(
                [&](sManagedRadio &radio_manager, const std::string &fronthaul_iface) -> bool {
                    radio_manager.stop_on_failure_attempts =
                        db->device_conf.stop_on_failure_attempts;
                    return true;
                });

            LOG(TRACE) << "goto STATE_CONNECT_TO_BACKHAUL_MANAGER";
            m_agent_state = STATE_CONNECT_TO_BACKHAUL_MANAGER;
        } else {
            LOG(ERROR) << "slave_state != STATE_WAIT_FOR_PLATFORM_MANAGER_REGISTER_RESPONSE";
        }
        break;
    }
    case beerocks_message::ACTION_PLATFORM_ARP_MONITOR_NOTIFICATION: {
        // LOG(TRACE) << "ACTION_PLATFORM_ARP_MONITOR_NOTIFICATION";
        if (!link_to_controller()) {
            return true;
        }
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_PLATFORM_ARP_MONITOR_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_PLATFORM_ARP_MONITOR_NOTIFICATION failed";
            return false;
        }

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CLIENT_ARP_MONITOR_NOTIFICATION>(cmdu_tx);
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        notification_out->params() = notification_in->params();
        send_cmdu_to_controller({}, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_PLATFORM_WLAN_PARAMS_CHANGED_NOTIFICATION: {
        LOG(TRACE) << "ACTION_PLATFORM_WLAN_PARAMS_CHANGED_NOTIFICATION";

        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_PLATFORM_WLAN_PARAMS_CHANGED_NOTIFICATION>();
        if (!notification) {
            LOG(ERROR) << "addClass cACTION_PLATFORM_WLAN_PARAMS_CHANGED_NOTIFICATION failed";
            return false;
        }

        auto fronthaul_iface = notification->iface_name();

        // slave only reacts to band_enabled change
        auto db = AgentDB::get();
        if (db->device_conf.front_radio.config.at(fronthaul_iface).band_enabled !=
            notification->wlan_settings().band_enabled) {
            LOG(DEBUG) << "band_enabled changed - performing agent_reset";
            agent_reset();
        }
        break;
    }
    case beerocks_message::ACTION_PLATFORM_DHCP_MONITOR_NOTIFICATION: {
        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_PLATFORM_DHCP_MONITOR_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass ACTION_PLATFORM_DHCP_MONITOR_NOTIFICATION failed";
            return false;
        }

        if (notification->op() == beerocks_message::eDHCPOp_Add ||
            notification->op() == beerocks_message::eDHCPOp_Old) {
            std::string client_mac = tlvf::mac_to_string(notification->mac());
            std::string client_ip  = network_utils::ipv4_to_string(notification->ipv4());

            LOG(DEBUG) << "ACTION_DHCP_LEASE_ADDED_NOTIFICATION mac " << client_mac
                       << " ip = " << client_ip << " name="
                       << std::string(notification->hostname(message::NODE_NAME_LENGTH));

            // notify master
            auto master_notification = message_com::create_vs_message<
                beerocks_message::cACTION_CONTROL_CLIENT_DHCP_COMPLETE_NOTIFICATION>(cmdu_tx);
            if (!master_notification) {
                LOG(ERROR) << "Failed building message!";
                return false;
            }

            master_notification->mac()  = notification->mac();
            master_notification->ipv4() = notification->ipv4();
            string_utils::copy_string(master_notification->name(message::NODE_NAME_LENGTH),
                                      notification->hostname(message::NODE_NAME_LENGTH),
                                      message::NODE_NAME_LENGTH);
            send_cmdu_to_controller({}, cmdu_tx);

        } else {
            LOG(DEBUG) << "ACTION_PLATFORM_DHCP_MONITOR_NOTIFICATION op " << notification->op()
                       << " mac " << notification->mac()
                       << " ip = " << network_utils::ipv4_to_string(notification->ipv4());
        }
        break;
    }
    case beerocks_message::ACTION_PLATFORM_ARP_QUERY_RESPONSE: {
        LOG(TRACE) << "ACTION_PLATFORM_ARP_QUERY_RESPONSE";
        auto response =
            beerocks_header->addClass<beerocks_message::cACTION_PLATFORM_ARP_QUERY_RESPONSE>();
        if (!response) {
            LOG(ERROR) << "addClass cACTION_PLATFORM_ARP_QUERY_RESPONSE failed";
            return false;
        }

        auto response_out =
            message_com::create_vs_message<beerocks_message::cACTION_CONTROL_ARP_QUERY_RESPONSE>(
                cmdu_tx, beerocks_header->id());
        if (response_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        response_out->params() = response->params();
        send_cmdu_to_controller({}, cmdu_tx);
        break;
    }

    default: {
        LOG(ERROR) << "Unknown PLATFORM_MANAGER message, action_op: "
                   << int(beerocks_header->action_op());
        return false;
    }
    }

    return true;
}

bool slave_thread::handle_cmdu_ap_manager_message(const std::string &fronthaul_iface, int fd,
                                                  std::shared_ptr<beerocks_header> beerocks_header)
{
    if (beerocks_header->action_op() == beerocks_message::ACTION_APMANAGER_UP_NOTIFICATION) {
        auto notification =
            beerocks_header->addClass<beerocks_message::cACTION_APMANAGER_UP_NOTIFICATION>();
        if (!notification) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_UP_NOTIFICATION failed";
            return false;
        }

        auto iface          = notification->iface_name();
        auto &radio_manager = m_radio_managers[iface];
        LOG(INFO) << "Received ACTION_APMANAGER_UP_NOTIFICATION from fronthaul " << iface;
        if (radio_manager.ap_manager_fd != beerocks::net::FileDescriptor::invalid_descriptor) {
            LOG(FATAL) << "AP manager opened new socket altough there is already open socket to it";
        }

        radio_manager.ap_manager_fd = fd;

        static const std::string client_name("ap manager ");
        m_cmdu_server->set_client_name(fd, client_name + iface);

        auto config_msg =
            message_com::create_vs_message<beerocks_message::cACTION_APMANAGER_CONFIGURE>(cmdu_tx);
        if (!config_msg) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        auto db               = AgentDB::get();
        config_msg->channel() = db->device_conf.front_radio.config.at(iface).configured_channel;

        return send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
    }

    if (fronthaul_iface.empty()) {
        LOG(FATAL) << "Received message from unknown socket, fronthaul_iface is empty. action_op: "
                   << beerocks_header->action_op() << ", incoming fd=" << fd;
    }

    auto &radio_manager = m_radio_managers[fronthaul_iface];

    if (beerocks_header->action_op() == beerocks_message::ACTION_APMANAGER_HEARTBEAT_NOTIFICATION) {
        radio_manager.ap_manager_last_seen       = std::chrono::steady_clock::now();
        radio_manager.ap_manager_retries_counter = 0;
        return true;
    }

    switch (beerocks_header->action_op()) {
    case beerocks_message::ACTION_APMANAGER_JOINED_NOTIFICATION: {
        LOG(INFO) << "received ACTION_APMANAGER_JOINED_NOTIFICATION " << fronthaul_iface;
        auto notification =
            beerocks_header->addClass<beerocks_message::cACTION_APMANAGER_JOINED_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_JOINED_NOTIFICATION failed";
            return false;
        }
        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            return false;
        }

        radio->front.iface_mac    = notification->params().iface_mac;
        radio->number_of_antennas = notification->params().ant_num;
        radio->antenna_gain_dB    = notification->params().ant_gain;
        radio->tx_power_dB        = notification->params().tx_power;
        radio->freq_type          = notification->params().frequency_band;
        radio->max_supported_bw   = notification->params().max_bandwidth;

        radio->ht_supported  = notification->params().ht_supported;
        radio->ht_capability = notification->params().ht_capability;
        std::copy_n(notification->params().ht_mcs_set, beerocks::message::HT_MCS_SET_SIZE,
                    radio->ht_mcs_set.begin());

        radio->vht_supported  = notification->params().vht_supported;
        radio->vht_capability = notification->params().vht_capability;
        std::copy_n(notification->params().vht_mcs_set, beerocks::message::VHT_MCS_SET_SIZE,
                    radio->vht_mcs_set.begin());

        radio->he_supported  = notification->params().he_supported;
        radio->he_capability = notification->params().he_capability;
        std::copy_n(notification->params().he_mcs_set, beerocks::message::HE_MCS_SET_SIZE,
                    radio->he_mcs_set.begin());

        save_channel_params_to_db(fronthaul_iface, notification->cs_params());

        radio->front.zwdfs                 = notification->params().zwdfs;
        radio->front.hybrid_mode_supported = notification->params().hybrid_mode_supported;
        LOG(DEBUG) << "ZWDFS AP: " << radio->front.zwdfs;

        fill_channel_list_to_agent_db(fronthaul_iface, notification->channel_list());

        update_vaps_info(fronthaul_iface, notification->vap_list().vaps);

        // cac
        save_cac_capabilities_params_to_db(fronthaul_iface);

        if (radio->chipset_vendor.empty()) {
            beerocks::bpl::get_ruid_chipset_vendor(radio->front.iface_mac, radio->chipset_vendor);
        }

        if (radio->front.zwdfs) {
            auto request = message_com::create_vs_message<
                beerocks_message::cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED>(cmdu_tx);

            if (!request) {
                LOG(ERROR) << "Failed building message!";
                break;
            }
            request->set_front_iface_name(fronthaul_iface);
            LOG(DEBUG) << "send ACTION_BACKHAUL_ZWDFS_RADIO_DETECTED for mac " << fronthaul_iface;
            m_backhaul_manager_client->send_cmdu(cmdu_tx);

            db->remove_radio_from_radios_list(fronthaul_iface);

            auto &radio_managers = m_radio_managers.get();
            auto radio_ctx_iter  = radio_managers.find(fronthaul_iface);

            // If getting here, the code below must be the last in this function, so the deleted
            // radio context will not be accessed.
            m_radio_managers.set_zwdfs(radio_ctx_iter);
            return true;
        }
        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE: {
        auto response_in = beerocks_header->addClass<
            beerocks_message::cACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE>();
        if (response_in == nullptr) {
            LOG(ERROR) << "addClass "
                          "cACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE "
                          "failed";
            return false;
        }
        LOG(INFO) << "received ACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE";

        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE>(
            cmdu_tx);
        if (response_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        response_out->success() = response_in->success();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_AP_DISABLED_NOTIFICATION: {
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_HOSTAP_AP_DISABLED_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_HOSTAP_AP_DISABLED_NOTIFICATION failed";
            return false;
        }
        LOG(INFO) << "received ACTION_APMANAGER_HOSTAP_AP_DISABLED_NOTIFICATION on vap_id="
                  << int(notification_in->vap_id());
        if (notification_in->vap_id() == beerocks::IFACE_RADIO_ID) {

            auto notification_out = message_com::create_vs_message<
                beerocks_message::cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION>(cmdu_tx);

            notification_out->set_iface(fronthaul_iface);
            m_backhaul_manager_client->send_cmdu(cmdu_tx);

            LOG(WARNING) << __FUNCTION__ << "AP_Disabled on radio, slave reset";
            if (radio_manager.configuration_in_progress) {
                LOG(INFO) << "configuration in progress, ignoring";
                break;
            }
            fronthaul_reset(radio_manager);
        } else {
            auto notification_out = message_com::create_vs_message<
                beerocks_message::cACTION_CONTROL_HOSTAP_AP_DISABLED_NOTIFICATION>(cmdu_tx);
            if (notification_out == nullptr) {
                LOG(ERROR) << "Failed building message!";
                return false;
            }

            notification_out->vap_id() = notification_in->vap_id();
            send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        }
        break;
    }
    case beerocks_message::ACTION_APMANAGER_ENABLE_APS_RESPONSE: {
        radio_manager.configuration_in_progress = false;
        LOG(INFO) << "received ACTION_APMANAGER_ENABLE_APS_RESPONSE";

        auto response =
            beerocks_header->addClass<beerocks_message::cACTION_APMANAGER_ENABLE_APS_RESPONSE>();
        if (!response) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_ENABLE_APS_RESPONSE failed";
            return false;
        }

        if (!response->success()) {
            LOG(ERROR) << "failed to enable APs";
            fronthaul_reset(radio_manager);
        }

        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_AP_ENABLED_NOTIFICATION: {
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_HOSTAP_AP_ENABLED_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_HOSTAP_AP_ENABLED_NOTIFICATION failed";
            return false;
        }
        LOG(INFO) << "received ACTION_APMANAGER_HOSTAP_AP_ENABLED_NOTIFICATION vap_id="
                  << int(notification_in->vap_id());

        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            return false;
        }

        const auto &vap_info = notification_in->vap_info();
        auto bssid =
            std::find_if(radio->front.bssids.begin(), radio->front.bssids.end(),
                         [&vap_info](const beerocks::AgentDB::sRadio::sFront::sBssid &bssid) {
                             return bssid.mac == vap_info.mac;
                         });
        if (bssid == radio->front.bssids.end()) {
            LOG(ERROR) << "Radio does not contain BSSID: " << vap_info.mac;
            return false;
        }

        // Update VAP info (BSSID) in the AgentDB
        bssid->ssid          = vap_info.ssid;
        bssid->fronthaul_bss = vap_info.fronthaul_vap;
        bssid->backhaul_bss  = vap_info.backhaul_vap;
        if (vap_info.backhaul_vap) {
            bssid->backhaul_bss_disallow_profile1_agent_association =
                vap_info.profile1_backhaul_sta_association_disallowed;
            bssid->backhaul_bss_disallow_profile2_agent_association =
                vap_info.profile2_backhaul_sta_association_disallowed;
        }

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_HOSTAP_AP_ENABLED_NOTIFICATION>(cmdu_tx);
        if (!notification_out) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        notification_out->vap_id()   = notification_in->vap_id();
        notification_out->vap_info() = notification_in->vap_info();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION: {
        auto notification_in = beerocks_header->addClass<
            beerocks_message::cACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION failed";
            return false;
        }

        LOG(INFO) << "received ACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION "
                  << fronthaul_iface;

        update_vaps_info(fronthaul_iface, notification_in->params().vaps);

        TrafficSeparation::apply_traffic_separation(fronthaul_iface);

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION>(cmdu_tx);
        if (!notification_out) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        notification_out->params() = notification_in->params();
        LOG(TRACE) << "send ACTION_CONTROL_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION "
                   << fronthaul_iface;
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);

        // This probably changed the "AP Operational BSS" list in topology, so send a notification
        if (!cmdu_tx.create(0, ieee1905_1::eMessageType::TOPOLOGY_NOTIFICATION_MESSAGE)) {
            LOG(ERROR) << "cmdu creation of type TOPOLOGY_NOTIFICATION_MESSAGE, has failed";
            return false;
        }

        auto tlvAlMacAddress = cmdu_tx.addClass<ieee1905_1::tlvAlMacAddress>();
        if (!tlvAlMacAddress) {
            LOG(ERROR) << "addClass ieee1905_1::tlvAlMacAddress failed";
            return false;
        }
        auto db                = AgentDB::get();
        tlvAlMacAddress->mac() = db->bridge.mac;
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_ACS_NOTIFICATION: {
        LOG(INFO) << "ACTION_APMANAGER_HOSTAP_ACS_NOTIFICATION";
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_HOSTAP_ACS_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass ACTION_APMANAGER_HOSTAP_ACS_NOTIFICATION failed";
            return false;
        }

        save_channel_params_to_db(fronthaul_iface, notification_in->cs_params());

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_HOSTAP_ACS_NOTIFICATION>(cmdu_tx,
                                                                       beerocks_header->id());
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        notification_out->cs_params() = notification_in->cs_params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        send_operating_channel_report(fronthaul_iface);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_CSA_NOTIFICATION: {
        LOG(INFO) << "ACTION_APMANAGER_HOSTAP_CSA_NOTIFICATION";

        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_HOSTAP_CSA_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_HOSTAP_CSA_ERROR_NOTIFICATION failed";
            return false;
        }

        save_channel_params_to_db(fronthaul_iface, notification_in->cs_params());

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_HOSTAP_CSA_NOTIFICATION>(cmdu_tx,
                                                                       beerocks_header->id());
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        notification_out->cs_params() = notification_in->cs_params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        send_operating_channel_report(fronthaul_iface);

        auto notification_out_bhm = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_HOSTAP_CSA_NOTIFICATION>(cmdu_tx);
        if (!notification_out_bhm) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        notification_out_bhm->cs_params() = notification_in->cs_params();

        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            break;
        }
        auto action_header         = message_com::get_beerocks_header(cmdu_tx)->actionhdr();
        action_header->radio_mac() = radio->front.iface_mac;

        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_CSA_ERROR_NOTIFICATION: {
        LOG(INFO) << "received ACTION_APMANAGER_HOSTAP_CSA_ERROR_NOTIFICATION";
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_HOSTAP_CSA_ERROR_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_HOSTAP_CSA_ERROR_NOTIFICATION failed";
            return false;
        }

        save_channel_params_to_db(fronthaul_iface, notification_in->cs_params());

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_HOSTAP_CSA_ERROR_NOTIFICATION>(cmdu_tx,
                                                                             beerocks_header->id());
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        notification_out->cs_params() = notification_in->cs_params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        send_operating_channel_report(fronthaul_iface);

        auto notification_out_bhm = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_HOSTAP_CSA_ERROR_NOTIFICATION>(cmdu_tx);
        if (!notification_out_bhm) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        notification_out_bhm->cs_params() = notification_in->cs_params();

        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            break;
        }
        auto action_header         = message_com::get_beerocks_header(cmdu_tx)->actionhdr();
        action_header->radio_mac() = radio->front.iface_mac;

        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE: {
        auto response_in = beerocks_header->addClass<
            beerocks_message::cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE>();
        if (response_in == nullptr) {
            LOG(ERROR) << "addClass ACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE failed";
            return false;
        }
        LOG(INFO) << "APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE mac="
                  << response_in->params().result.mac
                  << " rx_rssi=" << int(response_in->params().rx_rssi)
                  << " id=" << int(beerocks_header->id());

        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE>(
            cmdu_tx, beerocks_header->id());

        if (response_out == nullptr) {
            LOG(ERROR)
                << "Failed building ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE message!";
            break;
        }

        response_out->params()            = response_in->params();
        response_out->params().src_module = beerocks::BEEROCKS_ENTITY_AP_MANAGER;
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION: {
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass ACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION failed";
            return false;
        }

        auto &client_mac = notification_in->params().mac;
        auto &bssid      = notification_in->params().bssid;
        LOG(INFO) << "client disconnected sta_mac=" << client_mac << " from bssid=" << bssid;

        // If exists, remove client association information for disconnected client.
        auto db = AgentDB::get();
        db->erase_client(client_mac, bssid);

        // notify master
        if (!link_to_controller()) {
            LOG(DEBUG) << "Controller is not connected";
            return true;
        }

        // build 1905.1 message CMDU to send to the controller
        if (!cmdu_tx.create(0, ieee1905_1::eMessageType::TOPOLOGY_NOTIFICATION_MESSAGE)) {
            LOG(ERROR) << "cmdu creation of type TOPOLOGY_NOTIFICATION_MESSAGE, has failed";
            return false;
        }

        auto tlvAlMacAddress = cmdu_tx.addClass<ieee1905_1::tlvAlMacAddress>();
        if (!tlvAlMacAddress) {
            LOG(ERROR) << "addClass ieee1905_1::tlvAlMacAddress failed";
            return false;
        }
        tlvAlMacAddress->mac() = db->bridge.mac;

        auto client_association_event_tlv = cmdu_tx.addClass<wfa_map::tlvClientAssociationEvent>();
        if (!client_association_event_tlv) {
            LOG(ERROR) << "addClass tlvClientAssociationEvent failed";
            return false;
        }
        client_association_event_tlv->client_mac() = notification_in->params().mac;
        client_association_event_tlv->bssid()      = notification_in->params().bssid;
        client_association_event_tlv->association_event() =
            wfa_map::tlvClientAssociationEvent::CLIENT_HAS_LEFT_THE_BSS;

        if (!db->controller_info.prplmesh_controller) {
            LOG(DEBUG) << "non-prplMesh, not adding ClientAssociationEvent VS TLV";
        } else {
            // Add vendor specific tlv
            auto vs_tlv =
                message_com::add_vs_tlv<beerocks_message::tlvVsClientAssociationEvent>(cmdu_tx);

            if (!vs_tlv) {
                LOG(ERROR) << "add_vs_tlv tlvVsClientAssociationEvent failed";
                return false;
            }

            vs_tlv->mac()               = notification_in->params().mac;
            vs_tlv->bssid()             = notification_in->params().bssid;
            vs_tlv->vap_id()            = notification_in->params().vap_id;
            vs_tlv->disconnect_reason() = notification_in->params().reason;
            vs_tlv->disconnect_source() = notification_in->params().source;
            vs_tlv->disconnect_type()   = notification_in->params().type;
        }

        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);

        // profile-2

        // build 1905.1 0x8022 Client Disassociation Stats
        // message CMDU to send to the controller
        if (!cmdu_tx.create(0, ieee1905_1::eMessageType::CLIENT_DISASSOCIATION_STATS_MESSAGE)) {
            LOG(ERROR) << "cmdu creation of type CLIENT_DISASSOCIATION_STATS_MESSAGE, has failed";
            return false;
        }

        // 17.2.23 STA MAC Address Type
        auto sta_mac_address_tlv = cmdu_tx.addClass<wfa_map::tlvStaMacAddressType>();
        if (!sta_mac_address_tlv) {
            LOG(ERROR) << "addClass sta_mac_address_tlv failed";
            return false;
        }
        sta_mac_address_tlv->sta_mac() = notification_in->params().mac;

        // 17.2.64 Reason Code
        auto reason_code_tlv = cmdu_tx.addClass<wfa_map::tlvProfile2ReasonCode>();
        if (!reason_code_tlv) {
            LOG(ERROR) << "addClass reason_code_tlv failed";
            return false;
        }
        reason_code_tlv->reason_code() = wfa_map::tlvProfile2ReasonCode::LEAVING_NETWORK_DISASSOC;

        // 17.2.35 Associated STA Traffic Stats
        // TEMPORARY: adding empty statistics
        auto associated_sta_traffic_stats_tlv =
            cmdu_tx.addClass<wfa_map::tlvAssociatedStaTrafficStats>();
        if (!associated_sta_traffic_stats_tlv) {
            LOG(ERROR) << "addClass associated_sta_traffic_stats_tlv failed";
            return false;
        }
        associated_sta_traffic_stats_tlv->sta_mac() = notification_in->params().mac;

        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);

        break;
    }
    case beerocks_message::ACTION_APMANAGER_ACK: {
        auto response_in = beerocks_header->addClass<beerocks_message::cACTION_APMANAGER_ACK>();
        if (!response_in) {
            LOG(ERROR) << "addClass ACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE failed";
            return false;
        }

        auto cmdu_tx_header =
            cmdu_tx.create(beerocks_header->id(), ieee1905_1::eMessageType::ACK_MESSAGE);

        if (!cmdu_tx_header) {
            LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
            return false;
        }

        LOG(DEBUG) << "sending ACK message back to controller";
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE: {
        auto response_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE>();
        if (response_in == nullptr) {
            LOG(ERROR) << "addClass ACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE failed";
            return false;
        }
        LOG(INFO) << "ACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE, rep_mode="
                  << int(response_in->params().status_code);

        if (!cmdu_tx.create(0, ieee1905_1::eMessageType::CLIENT_STEERING_BTM_REPORT_MESSAGE)) {
            LOG(ERROR) << "cmdu creation of type CLIENT_STEERING_BTM_REPORT_MESSAGE, has failed";
            return false;
        }
        auto steering_btm_report_tlv = cmdu_tx.addClass<wfa_map::tlvSteeringBTMReport>();
        if (!steering_btm_report_tlv) {
            LOG(ERROR) << "addClass wfa_map::tlvSteeringBTMReport failed";
            return false;
        }
        //TODO Add target BSSID
        steering_btm_report_tlv->sta_mac()         = response_in->params().mac;
        steering_btm_report_tlv->btm_status_code() = response_in->params().status_code;

        /*
            If ACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE contains
            non-zero MAC fill up BSSID (client associated with) for
            CLIENT_STEERING_BTM_REPORT_MESSAGE otherwise find BSSID
            in the AgentDB.
        */
        if (response_in->params().source_bssid != net::network_utils::ZERO_MAC) {
            steering_btm_report_tlv->bssid() = response_in->params().source_bssid;
        } else {
            auto agent_db = AgentDB::get();

            /*
                For finding BSSID in AgentDB need to find STA entry.
                STA entry can be found by checking associated clients list
                per radio.
            */
            steering_btm_report_tlv->bssid() = net::network_utils::ZERO_MAC;
            for (const auto &radio : agent_db->get_radios_list()) {
                auto sta =
                    find_if(radio->associated_clients.begin(), radio->associated_clients.end(),
                            [&](const std::pair<sMacAddr, AgentDB::sRadio::sClient> &sta) {
                                return sta.first == steering_btm_report_tlv->sta_mac();
                            });
                if (sta != radio->associated_clients.end()) {
                    steering_btm_report_tlv->bssid() = sta->second.bssid;
                    break;
                }
            }
        }

        LOG(DEBUG) << "sending CLIENT_STEERING_BTM_REPORT_MESSAGE back to controller";
        LOG(DEBUG) << "BTM report source bssid: " << steering_btm_report_tlv->bssid();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE: {
        auto response_in = beerocks_header->addClass<
            beerocks_message::cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE>();
        if (response_in == nullptr) {
            LOG(ERROR)
                << "addClass ACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE failed";
            return false;
        }

        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE>(
            cmdu_tx, beerocks_header->id());
        if (response_out == nullptr) {
            LOG(ERROR) << "Failed building ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE "
                          "message!";
            break;
        }
        LOG(INFO) << "ACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE";
        response_out->mac() = response_in->mac();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION: {
        auto notification_in = beerocks_header->addClass<
            beerocks_message::cACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION>();
        if (!notification_in) {
            LOG(ERROR) << "addClass ACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION failed";
            return false;
        }
        LOG(TRACE) << "received ACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION";

        auto notification_out_bhm = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_HOSTAP_DFS_CAC_STARTED_NOTIFICATION>(cmdu_tx);
        if (!notification_out_bhm) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        notification_out_bhm->params() = notification_in->params();

        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            break;
        }
        auto action_header         = message_com::get_beerocks_header(cmdu_tx)->actionhdr();
        action_header->radio_mac() = radio->front.iface_mac;

        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION: {
        auto notification_in = beerocks_header->addClass<
            beerocks_message::cACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass sACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION failed";
            return false;
        }
        LOG(TRACE) << "received ACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION";

        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            return false;
        }

        radio->channel              = notification_in->params().channel;
        radio->bandwidth            = beerocks::eWiFiBandwidth(notification_in->params().bandwidth);
        radio->vht_center_frequency = notification_in->params().center_frequency1;
        radio->channel_ext_above_primary =
            radio->vht_center_frequency > wireless_utils::channel_to_freq(radio->channel);

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION>(cmdu_tx);
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        notification_out->params() = notification_in->params();

        /**
         * The Controller is not familiar with ZWDFS radio interface, so
         * avoid sending CMDU to the controller when the radio
         * interface is a ZWDFS radio interface.
         */
        if (!radio->front.zwdfs) {
            send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
            send_operating_channel_report(fronthaul_iface);
        }

        auto notification_out_bhm = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION>(cmdu_tx);
        if (!notification_out_bhm) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        notification_out_bhm->params() = notification_in->params();

        auto action_header         = message_com::get_beerocks_header(cmdu_tx)->actionhdr();
        action_header->radio_mac() = radio->front.iface_mac;

        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION: {
        auto notification_in = beerocks_header->addClass<
            beerocks_message::cACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR)
                << "addClass cACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION failed";
            return false;
        }
        LOG(TRACE) << "received ACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION";

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION>(cmdu_tx);
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        notification_out->params() = notification_in->params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION: {
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION>();
        if (!notification_in) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION failed";
            return false;
        }
        LOG(TRACE) << "received ACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION";
        auto &client_mac = notification_in->mac();
        auto &bssid      = notification_in->bssid();
        LOG(INFO) << "Client associated sta_mac=" << client_mac << " to bssid=" << bssid;

        // Check if the client is an Multi-AP Agent, '0' means a regular station.
        if (notification_in->multi_ap_profile() != 0) {
            // TODO:
            // If the Multi-AP Agent supports "Combined Profile-1 and Profile-2" mode, need to
            // configure the bBSS to support it on L2.
        }

        // Save information AgentDB
        auto db = AgentDB::get();
        db->erase_client(client_mac);

        // Set client association information for associated client
        auto radio = db->get_radio_by_mac(bssid, AgentDB::eMacType::BSSID);
        if (!radio) {
            LOG(DEBUG) << "Radio containing bssid " << bssid << " not found";
            break;
        }

        radio->associated_clients.emplace(
            client_mac, AgentDB::sRadio::sClient{bssid, notification_in->association_frame_length(),
                                                 notification_in->association_frame()});

        if (!link_to_controller()) {
            LOG(DEBUG) << "Controller is not connected";
            return true;
        }

        // build 1905.1 message CMDU to send to the controller
        if (!cmdu_tx.create(0, ieee1905_1::eMessageType::TOPOLOGY_NOTIFICATION_MESSAGE)) {
            LOG(ERROR) << "cmdu creation of type TOPOLOGY_NOTIFICATION_MESSAGE, has failed";
            return false;
        }

        auto tlvAlMacAddress = cmdu_tx.addClass<ieee1905_1::tlvAlMacAddress>();
        if (!tlvAlMacAddress) {
            LOG(ERROR) << "addClass ieee1905_1::tlvAlMacAddress failed";
            return false;
        }
        tlvAlMacAddress->mac() = db->bridge.mac;

        auto client_association_event_tlv = cmdu_tx.addClass<wfa_map::tlvClientAssociationEvent>();
        if (!client_association_event_tlv) {
            LOG(ERROR) << "addClass tlvClientAssociationEvent failed";
            return false;
        }
        client_association_event_tlv->client_mac() = notification_in->mac();
        client_association_event_tlv->bssid()      = notification_in->bssid();
        client_association_event_tlv->association_event() =
            wfa_map::tlvClientAssociationEvent::CLIENT_HAS_JOINED_THE_BSS;

        if (!db->controller_info.prplmesh_controller) {
            LOG(DEBUG) << "non-prlMesh, not adding ClientAssociationEvent VS TLV";
        } else {
            // Add vendor specific tlv
            auto vs_tlv =
                message_com::add_vs_tlv<beerocks_message::tlvVsClientAssociationEvent>(cmdu_tx);

            if (!vs_tlv) {
                LOG(ERROR) << "add_vs_tlv tlvVsClientAssociationEvent failed";
                return false;
            }

            vs_tlv->mac()          = notification_in->mac();
            vs_tlv->bssid()        = notification_in->bssid();
            vs_tlv->vap_id()       = notification_in->vap_id();
            vs_tlv->capabilities() = notification_in->capabilities();
        }

        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);

        break;
    }
    case beerocks_message::ACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION: {
        auto notification_in = beerocks_header->addClass<
            beerocks_message::cACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION failed";
            return false;
        }

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_STEERING_EVENT_PROBE_REQ_NOTIFICATION>(
            cmdu_tx, beerocks_header->id());
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        notification_out->params() = notification_in->params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }

    case beerocks_message::ACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_NOTIFICATION: {
        auto notification_in = beerocks_header->addClass<
            beerocks_message::cACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass "
                          "cACTION_APMANAGER_CLIENT_ScACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_"
                          "NOTIFICATIONOFTBLOCK_NOTIFICATION failed";
            return false;
        }

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_STEERING_EVENT_AUTH_FAIL_NOTIFICATION>(
            cmdu_tx, beerocks_header->id());
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        notification_out->params() = notification_in->params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_CLIENT_DISCONNECT_RESPONSE: {
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_CLIENT_DISCONNECT_RESPONSE>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_CLIENT_DISCONNECT_RESPONSE failed";
            return false;
        }

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CLIENT_DISCONNECT_RESPONSE>(cmdu_tx);
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        notification_out->params() = notification_in->params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_STEERING_CLIENT_SET_RESPONSE: {
        LOG(DEBUG) << "ACTION_APMANAGER_STEERING_CLIENT_SET_RESPONSE";
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_STEERING_CLIENT_SET_RESPONSE>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_CLIENT_DISCONNECT_RESPONSE failed";
            return false;
        }

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_STEERING_CLIENT_SET_RESPONSE>(cmdu_tx);
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        notification_out->params() = notification_in->params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_CHANNELS_LIST_RESPONSE: {
        LOG(TRACE) << "received ACTION_APMANAGER_CHANNELS_LIST_RESPONSE";
        auto response =
            beerocks_header->addClass<beerocks_message::cACTION_APMANAGER_CHANNELS_LIST_RESPONSE>();
        if (!response) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_CHANNELS_LIST_RESPONSE failed";
            return false;
        }

        fill_channel_list_to_agent_db(fronthaul_iface, response->channel_list());

        // Forward channels list to the Backhaul manager
        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_CHANNELS_LIST_RESPONSE>(cmdu_tx);
        if (!response_out) {
            LOG(ERROR) << "Failed to build message";
            break;
        }
        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            break;
        }
        auto action_header         = message_com::get_beerocks_header(cmdu_tx)->actionhdr();
        action_header->radio_mac() = radio->front.iface_mac;

        m_backhaul_manager_client->send_cmdu(cmdu_tx);

        // build channel preference report
        auto cmdu_tx_header = cmdu_tx.create(
            beerocks_header->id(), ieee1905_1::eMessageType::CHANNEL_PREFERENCE_REPORT_MESSAGE);

        if (!cmdu_tx_header) {
            LOG(ERROR) << "cmdu creation of type CHANNEL_PREFERENCE_REPORT_MESSAGE, has failed";
            return false;
        }

        auto preferences = get_channel_preferences_from_channels_list(fronthaul_iface);

        auto channel_preference_tlv = cmdu_tx.addClass<wfa_map::tlvChannelPreference>();
        if (!channel_preference_tlv) {
            LOG(ERROR) << "addClass ieee1905_1::tlvChannelPreference has failed";
            return false;
        }

        channel_preference_tlv->radio_uid() = radio->front.iface_mac;

        for (const auto &preference : preferences) {
            // Create operating class object
            auto op_class_channels = channel_preference_tlv->create_operating_classes_list();
            if (!op_class_channels) {
                LOG(ERROR) << "create_operating_classes_list() has failed!";
                return false;
            }
            // TODO: check that the data is parsed properly after fixing the following bug:
            // Since sFlags is defined after dynamic list cPreferenceOperatingClasses it cause data override
            // on the first channel on the list and sFlags itself.
            // See: https://github.com/prplfoundation/prplMesh/issues/8

            auto &operating_class_info           = preference.first;
            auto &operating_class_channels_list  = preference.second;
            op_class_channels->operating_class() = operating_class_info.operating_class;
            if (!op_class_channels->alloc_channel_list(operating_class_channels_list.size())) {
                LOG(ERROR) << "alloc_channel_list() has failed!";
                return false;
            }

            uint8_t idx = 0;
            for (auto channel : operating_class_channels_list) {
                *op_class_channels->channel_list(idx) = channel;
                idx++;
            }

            // Update channel list flags
            op_class_channels->flags() = operating_class_info.flags;

            // Push operating class object to the list of operating class objects
            if (!channel_preference_tlv->add_operating_classes_list(op_class_channels)) {
                LOG(ERROR) << "add_operating_classes_list() has failed!";
                return false;
            }
        }

        // cac tlvs //

        // create status report
        auto cac_status_report_tlv = cmdu_tx.addClass<wfa_map::tlvProfile2CacStatusReport>();
        if (!cac_status_report_tlv) {
            LOG(ERROR) << "Failed to create cac-status-report-tlv";
            return false;
        }

        CacStatusDatabase cac_status_database;

        // fill status report
        auto available_channels =
            cac_status_database.get_available_channels(radio->front.iface_mac);

        if (!cac_status_report_tlv->alloc_available_channels(available_channels.size())) {
            LOG(ERROR) << "Failed to allocate " << available_channels.size()
                       << " structures for available channels";
            return false;
        }
        for (unsigned int i = 0; i < available_channels.size(); ++i) {
            auto &available_ref = std::get<1>(cac_status_report_tlv->available_channels(i));
            available_ref.operating_class = available_channels[i].operating_class;
            available_ref.channel         = available_channels[i].channel;
            available_ref.minutes_since_cac_completion =
                std::chrono::duration_cast<std::chrono::minutes>(available_channels[i].duration)
                    .count();
        }

        // TODO
        // Complete status report
        // https://jira.prplfoundation.org/browse/PPM-1089

        // create completion report
        auto cac_completion_report_tlv =
            cmdu_tx.addClass<wfa_map::tlvProfile2CacCompletionReport>();
        if (!cac_completion_report_tlv) {
            LOG(ERROR) << "Failed to create cac-completion-report-tlv";
            return false;
        }

        // fill completion report
        auto cac_radio = cac_completion_report_tlv->create_cac_radios();
        if (!cac_radio) {
            LOG(ERROR) << "Failed to create cac radio for " << radio->front.iface_mac;
            return false;
        }
        cac_radio->radio_uid() = radio->front.iface_mac;

        const auto &cac_completion =
            cac_status_database.get_completion_status(radio->front.iface_mac);

        cac_radio->operating_class()       = cac_completion.first.operating_class;
        cac_radio->channel()               = cac_completion.first.channel;
        cac_radio->cac_completion_status() = cac_completion.first.completion_status;

        if (!cac_completion.second.empty()) {
            cac_radio->alloc_detected_pairs(cac_completion.second.size());
            for (unsigned int i = 0; i < cac_completion.second.size(); ++i) {
                if (std::get<0>(cac_radio->detected_pairs(i))) {
                    auto &cac_detected_pair = std::get<1>(cac_radio->detected_pairs(i));
                    cac_detected_pair.operating_class_detected = cac_completion.second[i].first;
                    cac_detected_pair.channel_detected         = cac_completion.second[i].second;
                }
            }
        }

        cac_completion_report_tlv->add_cac_radios(cac_radio);

        LOG(DEBUG) << "sending channel preference report for ruid=" << radio->front.iface_mac;

        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);

        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE: {
        // no more configuration
        radio_manager.configuration_in_progress = false;

        LOG(DEBUG) << "received ACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE";
        auto response_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE>();
        if (!response_in) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE failed";
            return false;
        }

        // report about the status
        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE>(cmdu_tx);
        if (!response_out) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        response_out->success() = response_in->success();

        LOG(DEBUG) << "send cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE";
        m_backhaul_manager_client->send_cmdu(cmdu_tx);

        // take actions when the cancelation failed
        if (!response_in->success()) {
            LOG(ERROR) << "cancel active cac failed - resetting the slave";
            fronthaul_reset(radio_manager);
        }

        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE: {
        auto notification_in = beerocks_header->addClass<
            beerocks_message::cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE>();
        if (!notification_in) {
            LOG(ERROR)
                << "addClass ACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE failed";
            return false;
        }
        LOG(TRACE) << "received ACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE";

        auto notification_out_bhm = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE>(cmdu_tx);
        if (!notification_out_bhm) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        notification_out_bhm->success() = notification_in->success();

        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            break;
        }
        auto action_header         = message_com::get_beerocks_header(cmdu_tx)->actionhdr();
        action_header->radio_mac() = radio->front.iface_mac;

        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    default: {
        LOG(ERROR) << "Unknown AP_MANAGER message, action_op: "
                   << int(beerocks_header->action_op());
        return false;
    }
    }

    return true;
}

bool slave_thread::handle_cmdu_monitor_message(const std::string &fronthaul_iface, int fd,
                                               std::shared_ptr<beerocks_header> beerocks_header)
{
    if (beerocks_header->action_op() == beerocks_message::ACTION_MONITOR_JOINED_NOTIFICATION) {
        auto notification =
            beerocks_header->addClass<beerocks_message::cACTION_MONITOR_JOINED_NOTIFICATION>();
        if (!notification) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_JOINED_NOTIFICATION failed";
            return false;
        }
        LOG(DEBUG) << "Received ACTION_MONITOR_JOINED_NOTIFICATION " << notification->iface_name();
        auto &radio_manager = m_radio_managers[notification->iface_name()];
        if (radio_manager.monitor_fd != beerocks::net::FileDescriptor::invalid_descriptor) {
            LOG(FATAL) << "Monitor opened a new socket altough there is already open socket to it";
        }

        radio_manager.monitor_fd = fd;
        static const std::string client_name("monitor ");
        m_cmdu_server->set_client_name(fd, client_name + notification->iface_name());

        if (m_agent_state != STATE_WAIT_FOR_FRONTHAUL_THREADS_JOINED) {
            LOG(WARNING) << "ACTION_MONITOR_JOINED_NOTIFICATION, but slave_state != "
                            "STATE_WAIT_FOR_FRONTHAUL_THREADS_JOINED";
        }
        return true;
    }

    if (fronthaul_iface.empty()) {
        LOG(FATAL) << "Received message from unknown socket, fronthaul_iface is empty. action_op: "
                   << beerocks_header->action_op() << ", incoming fd=" << fd;
    }
    auto &radio_manager = m_radio_managers[fronthaul_iface];

    if (radio_manager.monitor_fd != fd) {
        LOG(FATAL) << "Unknown socket, ACTION_MONITOR action_op: "
                   << int(beerocks_header->action_op());
        return true;
    }

    if (!link_to_controller()) {
        return true;
    }

    switch (beerocks_header->action_op()) {
    case beerocks_message::ACTION_MONITOR_HEARTBEAT_NOTIFICATION: {
        radio_manager.monitor_last_seen       = std::chrono::steady_clock::now();
        radio_manager.monitor_retries_counter = 0;
        break;
    }
    case beerocks_message::ACTION_MONITOR_HOSTAP_AP_DISABLED_NOTIFICATION: {
        auto response_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_HOSTAP_AP_DISABLED_NOTIFICATION>();
        if (response_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_HOSTAP_AP_DISABLED_NOTIFICATION failed";
            return false;
        }
        LOG(INFO) << "received ACTION_MONITOR_HOSTAP_AP_DISABLED_NOTIFICATION";
        if (response_in->vap_id() == beerocks::IFACE_RADIO_ID) {
            LOG(WARNING) << __FUNCTION__ << "AP_Disabled on radio, slave reset";
            if (radio_manager.configuration_in_progress) {
                LOG(INFO) << "configuration is in progress, ignoring";
                break;
            }
            fronthaul_reset(radio_manager);
        }
        break;
    }
    case beerocks_message::ACTION_MONITOR_CLIENT_START_MONITORING_RESPONSE: {
        auto response_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CLIENT_START_MONITORING_RESPONSE>();
        if (!response_in) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CLIENT_START_MONITORING_RESPONSE failed";
            break;
        }

        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CLIENT_START_MONITORING_RESPONSE>(
            cmdu_tx, beerocks_header->id());
        if (!response_out) {
            LOG(ERROR)
                << "Failed building cACTION_CONTROL_CLIENT_START_MONITORING_RESPONSE message!";
            break;
        }
        response_out->success() = response_in->success();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE: {
        auto response_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE>();
        if (response_in == nullptr) {
            LOG(ERROR) << "addClass ACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE failed";
            break;
        }
        LOG(INFO) << "ACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE mac="
                  << response_in->params().result.mac
                  << " rx_rssi=" << int(response_in->params().rx_rssi)
                  << " id=" << int(beerocks_header->id());

        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE>(
            cmdu_tx, beerocks_header->id());
        if (response_out == nullptr) {
            LOG(ERROR)
                << "Failed building ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE message!";
            break;
        }

        response_out->params()            = response_in->params();
        response_out->params().src_module = beerocks::BEEROCKS_ENTITY_MONITOR;
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_START_NOTIFICATION: {
        auto notification_in = beerocks_header->addClass<
            beerocks_message::cACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_START_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR)
                << "addClass ACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_START_NOTIFICATION failed";
            break;
        }

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_START_NOTIFICATION>(
            cmdu_tx, beerocks_header->id());
        if (notification_out == nullptr) {
            LOG(ERROR)
                << "Failed building ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE message!";
            break;
        }
        notification_out->mac() = notification_in->mac();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_HOSTAP_STATS_MEASUREMENT_RESPONSE: {
        /*
             * the following code will break if the structure of
             * message::sACTION_MONITOR_HOSTAP_STATS_MEASUREMENT_RESPONSE
             * will be different from
             * message::sACTION_CONTROL_HOSTAP_STATS_MEASUREMENT_RESPONSE
             */

        // LOG(DEBUG) << "Received ACTION_MONITOR_HOSTAP_STATS_MEASUREMENT_RESPONSE"; // the print is flooding the log

        auto response_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_HOSTAP_STATS_MEASUREMENT_RESPONSE>();
        if (response_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_HOSTAP_STATS_MEASUREMENT_RESPONSE failed";
            return false;
        }

        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_HOSTAP_STATS_MEASUREMENT_RESPONSE>(
            cmdu_tx, beerocks_header->id());
        if (response_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        auto ap_stats_size = response_in->ap_stats_size();
        if (ap_stats_size > 0) {
            if (!response_out->alloc_ap_stats(ap_stats_size)) {
                LOG(ERROR) << "Failed buffer allocation to size=" << int(ap_stats_size);
                break;
            }
            auto ap_stats_tuple_in  = response_in->ap_stats(0);
            auto ap_stats_tuple_out = response_out->ap_stats(0);
            std::copy_n(&std::get<1>(ap_stats_tuple_in), ap_stats_size,
                        &std::get<1>(ap_stats_tuple_out));
        }

        auto sta_stats_size = response_in->sta_stats_size();
        if (sta_stats_size > 0) {
            if (!response_out->alloc_sta_stats(sta_stats_size)) {
                LOG(ERROR) << "Failed buffer allocation to size=" << int(sta_stats_size);
                break;
            }
            auto sta_stats_tuple_in  = response_in->sta_stats(0);
            auto sta_stats_tuple_out = response_out->sta_stats(0);
            std::copy_n(&std::get<1>(sta_stats_tuple_in), sta_stats_size,
                        &std::get<1>(sta_stats_tuple_out));
        }

        // LOG(DEBUG) << "send ACTION_CONTROL_HOSTAP_STATS_MEASUREMENT_RESPONSE"; // the print is flooding the log

        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CLIENT_ASSOCIATED_STA_LINK_METRIC_RESPONSE: {
        auto response_in = beerocks_header->addClass<
            beerocks_message::cACTION_MONITOR_CLIENT_ASSOCIATED_STA_LINK_METRIC_RESPONSE>();
        if (!response_in) {
            LOG(ERROR)
                << "addClass ACTION_MONITOR_CLIENT_ASSOCIATED_STA_LINK_METRIC_RESPONSE failed";
            return false;
        }
        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE>(
            cmdu_tx, beerocks_header->id());
        if (response_out == nullptr) {
            LOG(ERROR)
                << "Failed building ACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE message!";
            break;
        }

        if (!response_out->alloc_bssid_info_list(response_in->bssid_info_list_length())) {
            LOG(ERROR) << "alloc_per_bss_sta_link_metrics failed";
            return false;
        }

        response_out->sta_mac() = response_in->sta_mac();

        for (size_t i = 0; i < response_out->bssid_info_list_length(); ++i) {
            auto &bss_in  = std::get<1>(response_in->bssid_info_list(i));
            auto &bss_out = std::get<1>(response_out->bssid_info_list(i));

            bss_out = bss_in;
        }

        LOG(DEBUG) << "Send ACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE";
        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_HOSTAP_LOAD_MEASUREMENT_NOTIFICATION: {
        auto notification_in = beerocks_header->addClass<
            beerocks_message::cACTION_MONITOR_HOSTAP_LOAD_MEASUREMENT_NOTIFICATION>();
        if (!notification_in) {
            LOG(ERROR) << "addClass cACTION_MONITOR_HOSTAP_LOAD_MEASUREMENT_NOTIFICATION failed";
            return false;
        }
        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_HOSTAP_LOAD_MEASUREMENT_NOTIFICATION>(
            cmdu_tx, beerocks_header->id());
        if (!notification_out) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        notification_out->params() = notification_in->params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CLIENT_NO_RESPONSE_NOTIFICATION: {
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CLIENT_NO_RESPONSE_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass ACTION_MONITOR_CLIENT_NO_RESPONSE_NOTIFICATION failed";
            break;
        }
        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CLIENT_NO_RESPONSE_NOTIFICATION>(
            cmdu_tx, beerocks_header->id());
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building ACTION_CONTROL_CLIENT_NO_RESPONSE_NOTIFICATION message!";
            break;
        }
        notification_out->mac() = notification_in->mac();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CLIENT_BEACON_11K_RESPONSE: {
        int mid = int(beerocks_header->id());
        LOG(TRACE) << "ACTION_MONITOR_CLIENT_BEACON_11K_RESPONSE id: 0x" << std::hex << mid;

        // flow:
        // 1. extract data from response_in (vendor specific response) and build
        // with the extracted data 1905 reponse_out message
        // 2. send ALSO vs response.
        // The reason for sending _both_ responses is because the 1905 response
        // does not contain the data itself, it is being sent just to pass certification tests

        // response in
        auto response_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CLIENT_BEACON_11K_RESPONSE>();
        if (response_in == nullptr) {
            LOG(ERROR) << "addClass ACTION_MONITOR_CLIENT_BEACON_11K_RESPONSE failed";
            break;
        }

        // old vs response:
        auto response_out_vs = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CLIENT_BEACON_11K_RESPONSE>(cmdu_tx,
                                                                          beerocks_header->id());
        if (response_out_vs == nullptr) {
            LOG(ERROR) << "Failed building ACTION_CONTROL_CLIENT_BEACON_11K_RESPONSE message!";
            break;
        }
        response_out_vs->params() = response_in->params();

        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        // end old response

        // new 1905 response:
        if (!cmdu_tx.create(mid, ieee1905_1::eMessageType::BEACON_METRICS_RESPONSE_MESSAGE)) {
            LOG(ERROR) << "cmdu creation of type BEACON_METRICS_RESPONSE_MESSAGE, has failed";
            return false;
        }

        auto response_out_1905 = cmdu_tx.addClass<wfa_map::tlvBeaconMetricsResponse>();
        if (response_out_1905 == nullptr) {
            LOG(ERROR) << "addClass wfa_map::tlvBeaconMetricsResponse failed";
            return false;
        }

        if (!gate::load(cmdu_tx, response_in)) {
            LOG(ERROR) << "unable to load vs beacon response into 1905";
            return false;
        }

        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        // end new 1905 response

        break;
    }
    case beerocks_message::ACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE: {
        LOG(INFO) << "ACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE: action_op: "
                  << int(beerocks_header->action_op());
        auto response_in = beerocks_header->addClass<
            beerocks_message::cACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE>();
        if (response_in == nullptr) {
            LOG(ERROR) << "addClass ACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE failed";
            break;
        }
        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE>(
            cmdu_tx, beerocks_header->id());
        if (response_out == nullptr) {
            LOG(ERROR) << "Failed building ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE "
                          "message!";
            break;
        }
        response_out->mac() = response_in->mac();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CLIENT_NO_ACTIVITY_NOTIFICATION: {
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CLIENT_NO_ACTIVITY_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass ACTION_MONITOR_CLIENT_NO_ACTIVITY_NOTIFICATION failed";
            break;
        }
        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CLIENT_NO_ACTIVITY_NOTIFICATION>(
            cmdu_tx, beerocks_header->id());
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building ACTION_CONTROL_CLIENT_NO_ACTIVITY_NOTIFICATION message!";
            break;
        }
        // Only mac id is the part of notification now, if this changes in future this message will break
        notification_out->mac() = notification_in->mac();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_HOSTAP_ACTIVITY_NOTIFICATION: {
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_HOSTAP_ACTIVITY_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_HOSTAP_ACTIVITY_NOTIFICATION failed";
            return false;
        }

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_HOSTAP_ACTIVITY_NOTIFICATION>(cmdu_tx);
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        notification_out->params() = notification_in->params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_NOTIFICATION: {
        auto notification_in = beerocks_header->addClass<
            beerocks_message::cACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_NOTIFICATION failed";
            return false;
        }

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_NOTIFICATION>(cmdu_tx);
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        notification_out->params() = notification_in->params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_STEERING_EVENT_CLIENT_ACTIVITY_NOTIFICATION: {
        auto notification_in = beerocks_header->addClass<
            beerocks_message::cACTION_MONITOR_STEERING_EVENT_CLIENT_ACTIVITY_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR)
                << "addClass cACTION_MONITOR_STEERING_EVENT_CLIENT_ACTIVITY_NOTIFICATION failed";
            return false;
        }

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_STEERING_EVENT_CLIENT_ACTIVITY_NOTIFICATION>(cmdu_tx);
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building  "
                          "cACTION_CONTROL_STEERING_EVENT_CLIENT_ACTIVITY_NOTIFICATION message!";
            return false;
        }
        notification_out->params() = notification_in->params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_STEERING_EVENT_SNR_XING_NOTIFICATION: {
        auto notification_in = beerocks_header->addClass<
            beerocks_message::cACTION_MONITOR_STEERING_EVENT_SNR_XING_NOTIFICATION>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_STEERING_EVENT_SNR_XING_NOTIFICATION failed";
            return false;
        }

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_STEERING_EVENT_SNR_XING_NOTIFICATION>(cmdu_tx);
        if (notification_out == nullptr) {
            LOG(ERROR)
                << "Failed building cACTION_CONTROL_STEERING_EVENT_SNR_XING_NOTIFICATION message!";
            return false;
        }
        notification_out->params() = notification_in->params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_GROUP_RESPONSE: {
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_STEERING_CLIENT_SET_GROUP_RESPONSE>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_STEERING_CLIENT_SET_GROUP_RESPONSE failed";
            return false;
        }

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_STEERING_CLIENT_SET_GROUP_RESPONSE>(cmdu_tx);
        if (notification_out == nullptr) {
            LOG(ERROR)
                << "Failed building cACTION_CONTROL_STEERING_CLIENT_SET_GROUP_RESPONSE message!";
            return false;
        }
        notification_out->params() = notification_in->params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_RESPONSE: {
        LOG(DEBUG) << "ACTION_MONITOR_STEERING_CLIENT_SET_RESPONSE";
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_STEERING_CLIENT_SET_RESPONSE>();
        if (notification_in == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_STEERING_CLIENT_SET_RESPONSE failed";
            return false;
        }

        auto notification_out = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_STEERING_CLIENT_SET_RESPONSE>(cmdu_tx);
        if (notification_out == nullptr) {
            LOG(ERROR) << "Failed building cACTION_CONTROL_STEERING_CLIENT_SET_RESPONSE message!";
            return false;
        }
        notification_out->params() = notification_in->params();
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE: {
        auto response_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE>();
        if (!response_in) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE failed";
            return false;
        }

        auto response_out_controller = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE>(cmdu_tx);
        if (!response_out_controller) {
            LOG(ERROR) << "Failed building cACTION_CONTROL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE";
            return false;
        }

        response_out_controller->success() = response_in->success();

        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);

        auto response_out_backhaul = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE>(cmdu_tx);
        if (!response_out_backhaul) {
            LOG(ERROR) << "Failed building cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE";
            return false;
        }

        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            LOG(ERROR) << "Failed to retrieve radio from the Agent DB";
            return false;
        }

        auto action_header               = message_com::get_beerocks_header(cmdu_tx)->actionhdr();
        action_header->radio_mac()       = radio->front.iface_mac;
        response_out_backhaul->success() = response_in->success();
        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE: {
        auto response_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE>();
        if (!response_in) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE failed";
            return false;
        }

        auto response_out_backhaul = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE>(cmdu_tx);
        if (!response_out_backhaul) {
            LOG(ERROR) << "Failed building cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE";
            return false;
        }

        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            LOG(ERROR) << "Failed to retrieve radio from the Agent DB";
            return false;
        }

        auto action_header               = message_com::get_beerocks_header(cmdu_tx)->actionhdr();
        action_header->radio_mac()       = radio->front.iface_mac;
        response_out_backhaul->success() = response_in->success();
        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CHANNEL_SCAN_ABORT_RESPONSE: {
        auto response_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_ABORT_RESPONSE>();
        if (!response_in) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CHANNEL_SCAN_ABORT_RESPONSE failed";
            return false;
        }

        auto response_out_backhaul = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE>(cmdu_tx);
        if (!response_out_backhaul) {
            LOG(ERROR) << "Failed building cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE";
            return false;
        }

        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            LOG(ERROR) << "Failed to retrieve radio from the Agent DB";
            return false;
        }

        auto action_header               = message_com::get_beerocks_header(cmdu_tx)->actionhdr();
        action_header->radio_mac()       = radio->front.iface_mac;
        response_out_backhaul->success() = response_in->success();
        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CHANNEL_SCAN_TRIGGERED_NOTIFICATION: {
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_TRIGGERED_NOTIFICATION>();
        if (!notification_in) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CHANNEL_SCAN_TRIGGERED_NOTIFICATION failed";
            return false;
        }

        auto notification_out_controller = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION>(cmdu_tx);
        if (!notification_out_controller) {
            LOG(ERROR) << "Failed building cACTION_CONTROL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION !";
            return false;
        }
        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);

        auto notification_out_backhaul = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION>(cmdu_tx);
        if (!notification_out_backhaul) {
            LOG(ERROR) << "Failed building cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION !";
            return false;
        }

        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            LOG(ERROR) << "Failed to retrieve radio from the Agent DB";
            return false;
        }

        auto action_header         = message_com::get_beerocks_header(cmdu_tx)->actionhdr();
        action_header->radio_mac() = radio->front.iface_mac;
        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CHANNEL_SCAN_RESULTS_NOTIFICATION: {
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_RESULTS_NOTIFICATION>();
        if (!notification_in) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CHANNEL_SCAN_RESULTS_NOTIFICATION failed";
            return false;
        }

        auto notification_out_controller = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CHANNEL_SCAN_RESULTS_NOTIFICATION>(cmdu_tx);
        if (!notification_out_controller) {
            LOG(ERROR) << "Failed building cACTION_CONTROL_CHANNEL_SCAN_RESULTS_NOTIFICATION !";
            return false;
        }

        notification_out_controller->scan_results() = notification_in->scan_results();
        notification_out_controller->is_dump()      = notification_in->is_dump();

        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);

        auto notification_out_backhaul = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION>(cmdu_tx);
        if (!notification_out_backhaul) {
            LOG(ERROR) << "Failed building cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION !";
            return false;
        }

        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            LOG(ERROR) << "Failed to retrieve radio from the Agent DB";
            return false;
        }

        auto action_header         = message_com::get_beerocks_header(cmdu_tx)->actionhdr();
        action_header->radio_mac() = radio->front.iface_mac;
        notification_out_backhaul->scan_results() = notification_in->scan_results();
        notification_out_backhaul->is_dump()      = notification_in->is_dump();
        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CHANNEL_SCAN_FINISHED_NOTIFICATION: {
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_FINISHED_NOTIFICATION>();
        if (!notification_in) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CHANNEL_SCAN_FINISHED_NOTIFICATION failed";
            return false;
        }

        auto notification_out_controller = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CHANNEL_SCAN_FINISHED_NOTIFICATION>(cmdu_tx);
        if (!notification_out_controller) {
            LOG(ERROR) << "Failed building cACTION_CONTROL_CHANNEL_SCAN_FINISHED_NOTIFICATION !";
            return false;
        }

        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);

        auto notification_out_backhaul = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION>(cmdu_tx);
        if (!notification_out_backhaul) {
            LOG(ERROR) << "Failed building cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION !";
            return false;
        }

        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            LOG(ERROR) << "Failed to retrieve radio from the Agent DB";
            return false;
        }

        auto action_header         = message_com::get_beerocks_header(cmdu_tx)->actionhdr();
        action_header->radio_mac() = radio->front.iface_mac;
        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CHANNEL_SCAN_ABORTED_NOTIFICATION: {

        LOG(DEBUG) << "Received ACTION_MONITOR_CHANNEL_SCAN_ABORTED_NOTIFICATION";

        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_ABORTED_NOTIFICATION>();
        if (!notification_in) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CHANNEL_SCAN_ABORTED_NOTIFICATION failed";
            return false;
        }

        auto notification_out_controller = message_com::create_vs_message<
            beerocks_message::cACTION_CONTROL_CHANNEL_SCAN_ABORT_NOTIFICATION>(cmdu_tx);
        if (!notification_out_controller) {
            LOG(ERROR) << "Failed building cACTION_CONTROL_CHANNEL_SCAN_ABORT_NOTIFICATION!";
            return false;
        }

        send_cmdu_to_controller(fronthaul_iface, cmdu_tx);

        auto notification_out_backhaul = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION>(cmdu_tx);
        if (!notification_out_backhaul) {
            LOG(ERROR) << "Failed building cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION!";
            return false;
        }

        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            LOG(ERROR) << "Failed to retrieve radio from the Agent DB";
            return false;
        }

        auto action_header         = message_com::get_beerocks_header(cmdu_tx)->actionhdr();
        action_header->radio_mac() = radio->front.iface_mac;
        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        break;
    }
    default: {
        LOG(ERROR) << "Unknown MONITOR message, action_op: " << int(beerocks_header->action_op());
        return false;
    }
    }

    return true;
}

bool slave_thread::agent_fsm()
{
    switch (m_agent_state) {
    case STATE_WAIT_BEFORE_INIT: {
        if (std::chrono::steady_clock::now() > m_agent_state_timer_sec) {
            LOG(TRACE) << "goto STATE_INIT";
            m_agent_state = STATE_INIT;
        }
        break;
    }
    case STATE_INIT: {
        LOG(INFO) << "STATE_INIT";

        auto db = AgentDB::get();
        std::string iface_mac;
        if (!network_utils::linux_iface_get_mac(db->bridge.iface_name, iface_mac)) {
            LOG(ERROR) << "Failed reading addresses from the bridge!";
            platform_notify_error(bpl::eErrorCode::BH_READING_DATA_FROM_THE_BRIDGE, "");
            m_radio_managers.do_on_each_radio_manager(
                [&](sManagedRadio &radio_manager, const std::string &fronthaul_iface) {
                    radio_manager.stop_on_failure_attempts--;
                    return true;
                });
            agent_reset();
            break;
        }

        // Update bridge parameters on AgentDB.
        db->bridge.mac = tlvf::mac_from_string(iface_mac);

        // On GW Platform, we clear the WAN interface from the database, once getting the
        // configuration from the Platform Manager. Since we initialize the local_gw flag later,
        // check if the WAN interface is empty instead of the local_gw flag.
        if (!db->ethernet.wan.iface_name.empty()) {
            if (!network_utils::linux_iface_get_mac(db->ethernet.wan.iface_name, iface_mac)) {
                LOG(ERROR) << "Failed reading wan mac address! iface="
                           << db->ethernet.wan.iface_name;
                m_radio_managers.do_on_each_radio_manager(
                    [&](sManagedRadio &radio_manager, const std::string &fronthaul_iface) {
                        radio_manager.stop_on_failure_attempts--;
                        return true;
                    });
                agent_reset();
            }

            // Update wan parameters on AgentDB.
            db->ethernet.wan.mac = tlvf::mac_from_string(iface_mac);
        }

        m_task_pool.send_event(eTaskType::AP_AUTOCONFIGURATION,
                               ApAutoConfigurationTask::eEvent::INIT_TASK);

        // Reset the traffic separation configuration as they will be reconfigured on
        // autoconfiguration.
        TrafficSeparation::traffic_seperation_configuration_clear();

        // Clear the channel_list
        // When FCC/ETSI is set, the prplmesh is not restarted, but the salve is.
        // Must clear the map to prevent residues of previous country configuration.
        // This is needed since the map is not cleared when read.
        m_radio_managers.do_on_each_radio_manager(
            [&](const sManagedRadio &radio_manager, const std::string &fronthaul_iface) {
                auto radio = db->radio(fronthaul_iface);
                if (!radio) {
                    return false;
                }
                if (radio_manager.ap_manager_fd == FileDescriptor::invalid_descriptor) {
                    radio->channels_list.clear();
                }
                return true;
            });

        m_agent_state = STATE_LOAD_PLATFORM_CONFIGURATION;
        break;
    }
    case STATE_LOAD_PLATFORM_CONFIGURATION: {
        LOG(DEBUG) << "STATE_LOAD_CONFIGURATION";
        if (!read_platform_configuration()) {
            LOG(DEBUG) << "Read platform configuration failed";
        }
        m_agent_state = STATE_CONNECT_TO_PLATFORM_MANAGER;
        break;
    }
    case STATE_CONNECT_TO_PLATFORM_MANAGER: {
        LOG(DEBUG) << "STATE_CONNECT_TO_PLATFORM_MANAGER";

        // Connect/Reconnect to the platform manager
        if (!m_platform_manager_client) {
            m_platform_manager_client = m_platform_manager_cmdu_client_factory->create_instance();

            LOG_IF(!m_platform_manager_client, FATAL) << "Failed connecting to Platform Manager!";

            beerocks::CmduClient::EventHandlers handlers;
            handlers.on_cmdu_received = [&](uint32_t iface_index, const sMacAddr &dst_mac,
                                            const sMacAddr &src_mac,
                                            ieee1905_1::CmduMessageRx &cmdu_rx) {
                handle_cmdu(m_platform_manager_client->get_fd(), cmdu_rx);
            };

            handlers.on_connection_closed = [&]() {
                LOG(ERROR) << "Client to Platform Manager disconnected, restarting "
                              "Agent";
                m_platform_manager_client.reset();
                LOG(DEBUG) << "goto STATE_STOPPED";
                m_agent_state = STATE_STOPPED;
                return true;
            };
            m_platform_manager_client->set_handlers(handlers);
        } else {
            LOG(DEBUG) << "Using existing client to Platform Manager";
        }

        // CMDU Message
        auto request = message_com::create_vs_message<
            beerocks_message::cACTION_PLATFORM_SON_SLAVE_REGISTER_REQUEST>(cmdu_tx);

        if (!request) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        m_platform_manager_client->send_cmdu(cmdu_tx);

        LOG(TRACE) << "send ACTION_PLATFORM_SON_SLAVE_REGISTER_REQUEST";
        LOG(TRACE) << "goto STATE_WAIT_FOR_PLATFORM_MANAGER_REGISTER_RESPONSE";
        m_agent_state_timer_sec =
            std::chrono::steady_clock::now() +
            std::chrono::seconds(WAIT_FOR_PLATFORM_MANAGER_REGISTER_RESPONSE_TIMEOUT_SEC);
        m_agent_state = STATE_WAIT_FOR_PLATFORM_MANAGER_REGISTER_RESPONSE;
        break;
    }
    case STATE_WAIT_FOR_PLATFORM_MANAGER_REGISTER_RESPONSE: {
        if (std::chrono::steady_clock::now() > m_agent_state_timer_sec) {
            LOG(ERROR) << "STATE_WAIT_FOR_PLATFORM_MANAGER_REGISTER_RESPONSE timeout!";
            platform_notify_error(bpl::eErrorCode::SLAVE_PLATFORM_MANAGER_REGISTER_TIMEOUT, "");
            m_radio_managers.do_on_each_radio_manager(
                [&](sManagedRadio &radio_manager, const std::string &fronthaul_iface) {
                    radio_manager.stop_on_failure_attempts--;
                    return true;
                });
            agent_reset();
        }
        break;
    }
    case STATE_CONNECT_TO_BACKHAUL_MANAGER: {
        m_is_backhaul_disconnected = false;

        // Connect/Reconnect to the backhaul manager
        if (!m_backhaul_manager_client) {
            m_backhaul_manager_client = m_backhaul_manager_cmdu_client_factory->create_instance();

            LOG_IF(!m_backhaul_manager_client, FATAL) << "Failed connecting to Backhaul Manager!";

            beerocks::CmduClient::EventHandlers handlers;
            handlers.on_cmdu_received = [&](uint32_t iface_index, const sMacAddr &dst_mac,
                                            const sMacAddr &src_mac,
                                            ieee1905_1::CmduMessageRx &cmdu_rx) {
                handle_cmdu(m_backhaul_manager_client->get_fd(), cmdu_rx);
            };
            handlers.on_connection_closed = [&]() {
                LOG(ERROR) << "Client to Backhaul Manager disconnected, restarting "
                              "Agent";
                m_backhaul_manager_client.reset();
                LOG(DEBUG) << "goto STATE_STOPPED";
                m_agent_state = STATE_STOPPED;
                return true;
            };
            m_backhaul_manager_client->set_handlers(handlers);
        } else {
            LOG(DEBUG) << "Using existing client to Backhaul Manager";
        }

        // CMDU Message
        auto request =
            message_com::create_vs_message<beerocks_message::cACTION_BACKHAUL_REGISTER_REQUEST>(
                cmdu_tx);

        if (request == nullptr) {
            LOG(ERROR) << "Failed building message!";
            break;
        }

        auto db = AgentDB::get();

        LOG(INFO) << "ACTION_BACKHAUL_REGISTER_REQUEST";

        m_backhaul_manager_client->send_cmdu(cmdu_tx);
        LOG(TRACE) << "send ACTION_BACKHAUL_REGISTER_REQUEST";
        LOG(TRACE) << "goto STATE_WAIT_FOR_BACKHAUL_MANAGER_REGISTER_RESPONSE";
        m_agent_state = STATE_WAIT_FOR_BACKHAUL_MANAGER_REGISTER_RESPONSE;

        break;
    }
    case STATE_WAIT_RETRY_CONNECT_TO_BACKHAUL_MANAGER: {
        if (std::chrono::steady_clock::now() > m_agent_state_timer_sec) {
            LOG(DEBUG) << "retrying to connect connecting to backhaul manager";
            LOG(TRACE) << "goto STATE_CONNECT_TO_BACKHAUL_MANAGER";
            m_agent_state = STATE_CONNECT_TO_BACKHAUL_MANAGER;
        }
        break;
    }
    case STATE_WAIT_FOR_BACKHAUL_MANAGER_REGISTER_RESPONSE: {
        break;
    }
    case STATE_JOIN_INIT: {

        bool all_radios_disabled = true;
        auto db                  = AgentDB::get();

        // Controller only mode skips fronthaul management
        if (db->device_conf.management_mode == BPL_MGMT_MODE_MULTIAP_CONTROLLER) {
            LOG(TRACE) << "Controller Only Mode goto STATE_BACKHAUL_ENABLE";
            m_agent_state = STATE_BACKHAUL_ENABLE;
            break;
        }

        for (const auto &radio_conf_element : db->device_conf.front_radio.config) {
            auto &radio_iface = radio_conf_element.first;
            auto &radio_conf  = radio_conf_element.second;
            LOG_IF(!radio_conf.band_enabled, DEBUG) << "radio " << radio_iface << " is disabled";
            all_radios_disabled &= !radio_conf.band_enabled;
        }

        if (all_radios_disabled) {
            LOG(TRACE) << "goto STATE_BACKHAUL_ENABLE";
            m_agent_state = STATE_BACKHAUL_ENABLE;
            break;
        }

        m_radio_managers.do_on_each_radio_manager(
            [&](const sManagedRadio &radio_manager, const std::string &fronthaul_iface) {
                auto db = AgentDB::get();
                if (!db->device_conf.front_radio.config.at(fronthaul_iface).band_enabled) {
                    return true;
                }
                auto radio = db->radio(fronthaul_iface);
                if (radio) {
                    // Set zwdfs to initial value.
                    radio->front.zwdfs = false;
                }
                if (!radio_manager.fronthaul_started) {
                    // Start the fronthaul process. Before starting, kill the existing one.
                    fronthaul_stop(fronthaul_iface);
                    fronthaul_start(fronthaul_iface);
                }
                return true;
            });

        LOG(TRACE) << "goto STATE_WAIT_FOR_FRONTHAUL_THREADS_JOINED";
        m_agent_state = STATE_WAIT_FOR_FRONTHAUL_THREADS_JOINED;
        break;
    }
    case STATE_WAIT_FOR_FRONTHAUL_THREADS_JOINED: {

        bool all_fronthauls_joined = true;
        m_radio_managers.do_on_each_radio_manager([&](const sManagedRadio &radio_manager,
                                                      const std::string &fronthaul_iface) {
            auto db = AgentDB::get();
            if (!db->device_conf.front_radio.config.at(fronthaul_iface).band_enabled) {
                return true;
            }

            // ZWDFS Monitor will not join
            auto radio = db->radio(fronthaul_iface);
            if (radio && radio->front.zwdfs) {
                return true;
            }

            all_fronthauls_joined &=
                (radio_manager.ap_manager_fd != beerocks::net::FileDescriptor::invalid_descriptor &&
                 radio_manager.monitor_fd != beerocks::net::FileDescriptor::invalid_descriptor);
            return true;
        });

        if (all_fronthauls_joined) {
            LOG(TRACE) << "goto STATE_BACKHAUL_ENABLE";
            m_agent_state = STATE_BACKHAUL_ENABLE;
        }
        break;
    }
    case STATE_BACKHAUL_ENABLE: {
        bool error = false;
        auto db    = AgentDB::get();

        if (db->device_conf.local_gw) {
            LOG(TRACE) << "goto STATE_SEND_BACKHAUL_MANAGER_ENABLE";
            m_agent_state = STATE_SEND_BACKHAUL_MANAGER_ENABLE;
            break;
        }

        // Go over all the radios and check if at least for one of them a wireless backhaul is
        // defined.
        bool backhaul_wireless_iface_exist = false;
        for (const auto &radio_conf_element : config.radios) {
            if (!radio_conf_element.second.backhaul_wireless_iface.empty()) {
                backhaul_wireless_iface_exist = true;
                break;
            }
        }
        if (db->ethernet.wan.iface_name.empty() && !backhaul_wireless_iface_exist) {
            LOG(DEBUG) << "No valid backhaul iface!";
            platform_notify_error(bpl::eErrorCode::CONFIG_NO_VALID_BACKHAUL_INTERFACE, "");
            error = true;
        }

        if (error) {
            m_radio_managers.do_on_each_radio_manager(
                [&](sManagedRadio &radio_manager, const std::string &fronthaul_iface) {
                    radio_manager.stop_on_failure_attempts--;
                    return true;
                });
            agent_reset();
        } else {
            // backhaul manager will request for backhaul iface and tx enable after receiving
            // ACTION_BACKHAUL_ENABLE, when wireless connection is required
            LOG(TRACE) << "goto STATE_SEND_BACKHAUL_MANAGER_ENABLE";
            m_agent_state = STATE_SEND_BACKHAUL_MANAGER_ENABLE;
        }
        break;
    }
    case STATE_SEND_BACKHAUL_MANAGER_ENABLE: {

        // CMDU Message
        auto bh_enable =
            message_com::create_vs_message<beerocks_message::cACTION_BACKHAUL_ENABLE>(cmdu_tx);
        if (bh_enable == nullptr) {
            LOG(ERROR) << "Failed building message!";
            break;
        }

        auto db = AgentDB::get();

        // Send the message
        LOG(DEBUG) << "send ACTION_BACKHAUL_ENABLE";
        m_backhaul_manager_client->send_cmdu(cmdu_tx);

        // Next state
        LOG(TRACE) << "goto STATE_WAIT_FOR_BACKHAUL_MANAGER_CONNECTED_NOTIFICATION";
        m_agent_state = STATE_WAIT_FOR_BACKHAUL_MANAGER_CONNECTED_NOTIFICATION;
        break;
    }
    case STATE_WAIT_FOR_BACKHAUL_MANAGER_CONNECTED_NOTIFICATION: {
        break;
    }
    case STATE_BACKHAUL_MANAGER_CONNECTED: {
        LOG(TRACE) << "BACKHAUL LINK CONNECTED";

        LOG(DEBUG) << "sending "
                      "ACTION_PLATFORM_SON_SLAVE_BACKHAUL_CONNECTION_COMPLETE_NOTIFICATION to "
                      "platform manager";
        auto notification = message_com::create_vs_message<
            beerocks_message::cACTION_PLATFORM_SON_SLAVE_BACKHAUL_CONNECTION_COMPLETE_NOTIFICATION>(
            cmdu_tx);

        if (notification == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        m_platform_manager_client->send_cmdu(cmdu_tx);

        auto db = AgentDB::get();

        // Configure the transport process to bind the al_mac address
        if (!m_broker_client->configure_al_mac(db->bridge.mac)) {
            LOG(FATAL) << "Failed configuring transport process!";
        }

        // In certification mode, if prplMesh is configured with local controller, do not enable
        // the transport process until agent has connected to controller. This way we prevent
        // the agent from connecting to another controller in the testbed, which might still be
        // running from a previous test.
        if (!(db->device_conf.certification_mode && db->device_conf.local_controller)) {
            if (db->device_conf.management_mode != BPL_MGMT_MODE_NOT_MULTIAP) {
                // Configure the transport process to use the network bridge
                if (!m_broker_client->configure_interfaces(db->bridge.iface_name, {}, true, true)) {
                    LOG(FATAL) << "Failed configuring transport process!";
                }
            }
        }
        LOG(TRACE) << "goto STATE_WAIT_FOR_AUTO_CONFIGURATION_COMPLETE";
        m_agent_state = STATE_WAIT_FOR_AUTO_CONFIGURATION_COMPLETE;
        m_task_pool.send_event(eTaskType::AP_AUTOCONFIGURATION,
                               ApAutoConfigurationTask::eEvent::START_AP_AUTOCONFIGURATION);
        break;
    }
    case STATE_WAIT_FOR_AUTO_CONFIGURATION_COMPLETE: {
        auto db = AgentDB::get();
        if (db->statuses.ap_autoconfiguration_completed) {
            LOG(TRACE) << "goto STATE_OPERATIONAL";
            m_agent_state = STATE_OPERATIONAL;

            // Make sure OPERATIONAL state will be done right away.
            return agent_fsm();
        }
        break;
    }
    // Note: The state STATE_OPERATIONAL occurs only once after every Agent reset.
    // See state STATE_WAIT_FOR_AUTO_CONFIGURATION_COMPLETE.
    case STATE_OPERATIONAL: {
        LOG(TRACE) << "Agent is in STATE_OPERATIONAL";

        // In certification mode, if prplMesh is configured with local controller, do not enable the
        // transport process until agent has connected to controller. This way we prevent the agent
        // from connecting to another controller in the testbed, which might still be running from a
        // previous test.
        auto db = AgentDB::get();
        if (db->device_conf.certification_mode && db->device_conf.local_controller) {
            if (db->device_conf.management_mode != BPL_MGMT_MODE_NOT_MULTIAP) {
                // Configure the transport process to use the network bridge
                if (!m_broker_client->configure_interfaces(db->bridge.iface_name, {}, true, true)) {
                    LOG(FATAL) << "Failed configuring transport process!";
                    break;
                }
            }
        }
        m_radio_managers.do_on_each_radio_manager(
            [&](sManagedRadio &radio_manager, const std::string &fronthaul_iface) -> bool {
                auto db                                = AgentDB::get();
                radio_manager.stop_on_failure_attempts = db->device_conf.stop_on_failure_attempts;
                return true;
            });
        break;
    }
    case STATE_STOPPED: {
        break;
    }
    default: {
        LOG(ERROR) << "Unknown state!";
        break;
    }
    }
    return true;
}

void slave_thread::fronthaul_stop(const std::string &fronthaul_iface)
{
    LOG(INFO) << "fronthaul stop " << fronthaul_iface;

    // Kill Fronthaul pid
    os_utils::kill_pid(config.temp_path + "pid/",
                       std::string(BEEROCKS_FRONTHAUL) + "_" + fronthaul_iface);
}

void slave_thread::fronthaul_start(const std::string &fronthaul_iface)
{
    LOG(INFO) << "fronthaul start " << fronthaul_iface;

    m_radio_managers[fronthaul_iface].fronthaul_started = true;

    // Start new Fronthaul process
    std::string file_name = "./" + std::string(BEEROCKS_FRONTHAUL);

    // Check if file does not exist in current location
    if (access(file_name.c_str(), F_OK) == -1) {
        file_name = mapf::utils::get_install_path() + "bin/" + std::string(BEEROCKS_FRONTHAUL);
    }
    std::string cmd = file_name + " -i " + fronthaul_iface;
    SYSTEM_CALL(cmd, true);
}

bool slave_thread::monitor_heartbeat_check(const std::string &fronthaul_iface)
{
    auto &radio_manager = m_radio_managers[fronthaul_iface];
    if (radio_manager.monitor_fd == beerocks::net::FileDescriptor::invalid_descriptor) {
        return true;
    }
    auto now = std::chrono::steady_clock::now();
    int time_elapsed_secs =
        std::chrono::duration_cast<std::chrono::seconds>(now - radio_manager.monitor_last_seen)
            .count();
    if (time_elapsed_secs > MONITOR_HEARTBEAT_TIMEOUT_SEC) {
        radio_manager.monitor_retries_counter++;
        radio_manager.monitor_last_seen = now;
        LOG(INFO) << "time_elapsed_secs > MONITOR_HEARTBEAT_TIMEOUT_SEC monitor_retries_counter = "
                  << radio_manager.monitor_retries_counter;
    }
    if (radio_manager.monitor_retries_counter >= MONITOR_HEARTBEAT_RETRIES) {
        LOG(INFO)
            << "monitor_retries_counter >= MONITOR_HEARTBEAT_RETRIES monitor_retries_counter = "
            << radio_manager.monitor_retries_counter << " agent_reset!";
        radio_manager.monitor_retries_counter = 0;
        return false;
    }
    return true;
}

bool slave_thread::ap_manager_heartbeat_check(const std::string &fronthaul_iface)
{
    auto &radio_manager = m_radio_managers[fronthaul_iface];

    if (radio_manager.ap_manager_fd == beerocks::net::FileDescriptor::invalid_descriptor) {
        return true;
    }
    auto now = std::chrono::steady_clock::now();
    int time_elapsed_secs =
        std::chrono::duration_cast<std::chrono::seconds>(now - radio_manager.ap_manager_last_seen)
            .count();
    if (time_elapsed_secs > AP_MANAGER_HEARTBEAT_TIMEOUT_SEC) {
        radio_manager.ap_manager_retries_counter++;
        radio_manager.ap_manager_last_seen = now;
        LOG(INFO) << "time_elapsed_secs > AP_MANAGER_HEARTBEAT_TIMEOUT_SEC "
                     "ap_manager_retries_counter = "
                  << radio_manager.ap_manager_retries_counter;
    }
    if (radio_manager.ap_manager_retries_counter >= AP_MANAGER_HEARTBEAT_RETRIES) {
        LOG(INFO) << "ap_manager_retries_counter >= AP_MANAGER_HEARTBEAT_RETRIES "
                     "ap_manager_retries_counter = "
                  << radio_manager.ap_manager_retries_counter << " agent_reset!";
        radio_manager.ap_manager_retries_counter = 0;
        return false;
    }
    return true;
}

bool slave_thread::link_to_controller()
{
    auto db = AgentDB::get();
    return db->statuses.ap_autoconfiguration_completed;
}

bool slave_thread::send_cmdu_to_controller(const std::string &fronthaul_iface,
                                           ieee1905_1::CmduMessageTx &cmdu_tx)
{
    auto db = AgentDB::get();
    if (cmdu_tx.getMessageType() == ieee1905_1::eMessageType::VENDOR_SPECIFIC_MESSAGE) {
        if (!db->controller_info.prplmesh_controller) {
            return true; // don't send VS messages to non prplmesh controllers
        }
        auto beerocks_header = message_com::get_beerocks_header(cmdu_tx);
        if (!beerocks_header) {
            LOG(ERROR) << "Failed getting beerocks_header!";
            return false;
        }

        auto radio = db->radio(fronthaul_iface);
        if (radio) {
            beerocks_header->actionhdr()->radio_mac() = radio->front.iface_mac;
        }
        beerocks_header->actionhdr()->direction() = beerocks::BEEROCKS_DIRECTION_CONTROLLER;
    }

    sMacAddr dst_addr;
    switch (cmdu_tx.getMessageType()) {
    case ieee1905_1::eMessageType::TOPOLOGY_NOTIFICATION_MESSAGE:
    case ieee1905_1::eMessageType::AP_AUTOCONFIGURATION_SEARCH_MESSAGE:
    case ieee1905_1::eMessageType::ASSOCIATION_STATUS_NOTIFICATION_MESSAGE:
        dst_addr = network_utils::MULTICAST_1905_MAC_ADDR;
        break;
    default:
        dst_addr = db->controller_info.bridge_mac;
        break;
    }

    return m_broker_client->send_cmdu(cmdu_tx, dst_addr, db->bridge.mac);
}

bool slave_thread::send_cmdu(int fd, ieee1905_1::CmduMessageTx &cmdu_tx)
{
    return m_cmdu_server->send_cmdu(fd, cmdu_tx);
}

bool slave_thread::forward_cmdu_to_uds(int fd, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    return m_cmdu_server->forward_cmdu(fd, 0, {}, {}, cmdu_rx);
}

bool slave_thread::forward_cmdu_to_controller(ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto db = AgentDB::get();
    sMacAddr dst_addr;
    switch (cmdu_tx.getMessageType()) {
    case ieee1905_1::eMessageType::TOPOLOGY_NOTIFICATION_MESSAGE:
    case ieee1905_1::eMessageType::AP_AUTOCONFIGURATION_SEARCH_MESSAGE:
    case ieee1905_1::eMessageType::ASSOCIATION_STATUS_NOTIFICATION_MESSAGE:
        dst_addr = network_utils::MULTICAST_1905_MAC_ADDR;
        break;
    default:
        dst_addr = db->controller_info.bridge_mac;
        break;
    }

    return m_broker_client->forward_cmdu(cmdu_rx, dst_addr, db->bridge.mac);
}

bool slave_thread::handle_client_association_request(int fd, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    const auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received CLIENT_ASSOCIATION_CONTROL_REQUEST_MESSAGE, mid=" << std::dec
               << int(mid);

    auto association_control_request_tlv =
        cmdu_rx.getClass<wfa_map::tlvClientAssociationControlRequest>();
    if (!association_control_request_tlv) {
        LOG(ERROR) << "addClass wfa_map::tlvClientAssociationControlRequest failed";
        return false;
    }

    const auto &bssid   = association_control_request_tlv->bssid_to_block_client();
    const auto &sta_mac = std::get<1>(association_control_request_tlv->sta_list(0));

    auto block = association_control_request_tlv->association_control();
    if (block == wfa_map::tlvClientAssociationControlRequest::UNBLOCK) {
        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_CLIENT_ALLOW_REQUEST>(cmdu_tx, mid);
        if (!request_out) {
            LOG(ERROR) << "Failed building ACTION_APMANAGER_CLIENT_ALLOW_REQUEST message!";
            return false;
        }

        request_out->mac()   = sta_mac;
        request_out->bssid() = bssid;
    } else if (block == wfa_map::tlvClientAssociationControlRequest::BLOCK) {
        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_CLIENT_DISALLOW_REQUEST>(cmdu_tx, mid);
        if (!request_out) {
            LOG(ERROR) << "Failed building ACTION_APMANAGER_CLIENT_DISALLOW_REQUEST message!";
            return false;
        }

        request_out->mac()                 = sta_mac;
        request_out->bssid()               = bssid;
        request_out->validity_period_sec() = association_control_request_tlv->validity_period_sec();
    }

    auto db    = AgentDB::get();
    auto radio = db->get_radio_by_mac(bssid, AgentDB::eMacType::BSSID);
    if (!radio) {
        LOG(ERROR) << "BSSID " << bssid << " was not found in any of the Agent radios";
        return false;
    }
    const auto &radio_manager = m_radio_managers[radio->front.iface_name];

    send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);

    if (!cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE)) {
        LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
        return false;
    }

    LOG(DEBUG) << "sending ACK message back to controller";
    return send_cmdu_to_controller(radio->front.iface_name, cmdu_tx);
}

bool slave_thread::handle_1905_higher_layer_data_message(int fd, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    const auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received HIGHER_LAYER_DATA_MESSAGE , mid=" << std::hex << int(mid);

    auto tlvHigherLayerData = cmdu_rx.getClass<wfa_map::tlvHigherLayerData>();
    if (!tlvHigherLayerData) {
        LOG(ERROR) << "addClass wfa_map::tlvHigherLayerData failed";
        return false;
    }

    const auto protocol       = tlvHigherLayerData->protocol();
    const auto payload_length = tlvHigherLayerData->payload_length();
    LOG(DEBUG) << "Protocol: " << std::hex << int(protocol);
    LOG(DEBUG) << "Payload-Length: " << std::hex << int(payload_length);

    // Build ACK message CMDU
    auto cmdu_tx_header = cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE);
    if (!cmdu_tx_header) {
        LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
        return false;
    }
    LOG(DEBUG) << "Sending ACK message to the originator, mid=" << std::hex << int(mid);

    return send_cmdu_to_controller({}, cmdu_tx);
}

bool slave_thread::handle_ack_message(int fd, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    // TODO - this is a stub handler for the purpose of controller certification testing,
    //       will be implemented later on agent certification
    const auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received ACK_MESSAGE, mid=" << std::dec << int(mid);
    return true;
}

bool slave_thread::handle_client_steering_request(int fd, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    const auto mid = cmdu_rx.getMessageId();

    auto steering_request_tlv          = cmdu_rx.getClass<wfa_map::tlvSteeringRequest>();
    auto steering_request_tlv_profile2 = cmdu_rx.getClass<wfa_map::tlvProfile2SteeringRequest>();
    if (!steering_request_tlv && !steering_request_tlv_profile2) {
        LOG(ERROR) << "addClass wfa_map::tlvSteeringRequest failed";
        return false;
    }

    LOG(DEBUG) << "Received CLIENT_STEERING_REQUEST_MESSAGE , mid=" << std::hex << int(mid);

    auto request_mode = steering_request_tlv_profile2
                            ? steering_request_tlv_profile2->request_flags().request_mode
                            : steering_request_tlv->request_flags().request_mode;
    LOG(DEBUG) << "request_mode: " << std::hex << int(request_mode);

    if (request_mode ==
        wfa_map::tlvSteeringRequest::REQUEST_IS_A_STEERING_MANDATE_TO_TRIGGER_STEERING) {
        // TODO Handle 0 or more then 1 sta in list, currenlty cli steers only 1 client
        LOG(DEBUG) << "Request Mode bit is set - Steering Mandate";

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_CLIENT_BSS_STEER_REQUEST>(cmdu_tx, mid);
        if (!request_out) {
            LOG(ERROR) << "Failed building ACTION_APMANAGER_CLIENT_BSS_STEER_REQUEST message!";
            return false;
        }

        if (steering_request_tlv_profile2) {
            auto bssid_list                 = steering_request_tlv_profile2->target_bssid_list(0);
            request_out->params().cur_bssid = steering_request_tlv_profile2->bssid();
            request_out->params().mac = std::get<1>(steering_request_tlv_profile2->sta_list(0));
            request_out->params().disassoc_timer_ms =
                steering_request_tlv_profile2->btm_disassociation_timer_ms();
            request_out->params().target.bssid = std::get<1>(bssid_list).target_bssid;
            request_out->params().target.operating_class =
                std::get<1>(bssid_list).target_bss_operating_class;
            request_out->params().target.channel =
                std::get<1>(bssid_list).target_bss_channel_number;
            request_out->params().disassoc_imminent =
                steering_request_tlv_profile2->request_flags().btm_disassociation_imminent_bit;
            request_out->params().target.reason = std::get<1>(bssid_list).target_bss_reason_code;
        } else {
            auto bssid_list                 = steering_request_tlv->target_bssid_list(0);
            request_out->params().cur_bssid = steering_request_tlv->bssid();
            request_out->params().mac       = std::get<1>(steering_request_tlv->sta_list(0));
            request_out->params().disassoc_timer_ms =
                steering_request_tlv->btm_disassociation_timer_ms();
            request_out->params().target.bssid = std::get<1>(bssid_list).target_bssid;
            request_out->params().target.operating_class =
                std::get<1>(bssid_list).target_bss_operating_class;
            request_out->params().target.channel =
                std::get<1>(bssid_list).target_bss_channel_number;
            request_out->params().disassoc_imminent =
                steering_request_tlv->request_flags().btm_disassociation_imminent_bit;
            request_out->params().target.reason = -1; // Mark that reason is not added
        }

        auto db = AgentDB::get();
        auto radio =
            db->get_radio_by_mac(request_out->params().cur_bssid, AgentDB::eMacType::BSSID);
        if (!radio) {
            LOG(ERROR) << "Radio with BSSID " << request_out->params().cur_bssid
                       << " as requested on steering request, not found ";
            return false;
        }

        send_cmdu(m_radio_managers[radio->front.iface_name].ap_manager_fd, cmdu_tx);
        return true;
    } else {

        // Handling of steering opportunity

        // NOTE: the implementation below does not actually take the steering
        // opportunity and tries to steer. Instead, it just reports ACK
        // and steering-completed.
        // Taking no action is a legitimate result of steering opportunity request,
        // and this is what is done here.
        // Later in time we may actually implement the opportunity to steer.

        LOG(DEBUG) << "Request Mode bit is not set - Steering Opportunity";

        auto cmdu_tx_header = cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE);

        if (!cmdu_tx_header) {
            LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
            return false;
        }

        LOG(DEBUG) << "sending ACK message back to controller";
        send_cmdu_to_controller({}, cmdu_tx);

        // build and send steering completed message
        cmdu_tx_header = cmdu_tx.create(0, ieee1905_1::eMessageType::STEERING_COMPLETED_MESSAGE);

        if (!cmdu_tx_header) {
            LOG(ERROR) << "cmdu creation of type STEERING_COMPLETED_MESSAGE, has failed";
            return false;
        }
        LOG(DEBUG) << "sending STEERING_COMPLETED_MESSAGE back to controller";
        return send_cmdu_to_controller({}, cmdu_tx);
    }
}

bool slave_thread::handle_beacon_metrics_query(int fd, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received BEACON_METRICS_QUERY_MESSAGE, mid=" << std::hex << int(mid);

    // create vs message
    auto request_out =
        message_com::create_vs_message<beerocks_message::cACTION_MONITOR_CLIENT_BEACON_11K_REQUEST>(
            cmdu_tx, mid);
    if (request_out == nullptr) {
        LOG(ERROR) << "Failed building ACTION_MONITOR_CLIENT_BEACON_11K_REQUEST message!";
        return false;
    }

    if (!gate::load(request_out, cmdu_rx)) {
        LOG(ERROR) << "failed translating 1905 message to vs message";
        return false;
    }

    auto db    = AgentDB::get();
    auto radio = db->get_radio_by_mac(request_out->params().sta_mac, AgentDB::eMacType::CLIENT);
    if (!radio) {
        LOG(ERROR) << "Radio with connected sta_mac " << request_out->params().sta_mac
                   << " as requested on beacon metrics query, not found ";
        return false;
    }

    send_cmdu(m_radio_managers[radio->front.iface_name].monitor_fd, cmdu_tx);
    return true;
}

bool slave_thread::handle_ap_metrics_query(int fd, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    // Extract the first bssid on the query to find out on which fronthaul the request has been
    // sent to. This code is temporary and eventually will be merged with the handler in the
    // LinkMetricsCollectionTask and move there. PPM-352.
    auto tlvApMetricQuery = cmdu_rx.getClass<wfa_map::tlvApMetricQuery>();

    // List of radio interface names to forward the message.
    std::unordered_set<std::string> radios_to_forward;
    for (auto i = 0; i < tlvApMetricQuery->bssid_list_length(); i++) {
        auto bssid_tuple = tlvApMetricQuery->bssid_list(0);
        if (!std::get<0>(bssid_tuple)) {
            LOG(ERROR) << "Failed to get bssid from tlvApMetricQuery";
        }
        // Check only the first bssid since link_metrics_collection_task is splitting the message
        // in a way that each of the splitted messages will have only BSSs of a single radio.
        auto &bssid = std::get<1>(tlvApMetricQuery->bssid_list(0));
        auto db     = AgentDB::get();
        auto radio  = db->get_radio_by_mac(bssid, AgentDB::eMacType::BSSID);
        if (!radio) {
            LOG(ERROR) << "Radio with BSSID " << bssid
                       << " as requested on ap metrics query, not found ";
            return false;
        }
        radios_to_forward.insert(radio->front.iface_name);
    }

    std::for_each(
        radios_to_forward.begin(), radios_to_forward.end(), [&](const std::string &radio_iface) {
            if (!forward_cmdu_to_uds(m_radio_managers[radio_iface].monitor_fd, cmdu_rx)) {
                LOG(ERROR) << "Failed sending AP_METRICS_QUERY_MESSAGE message to monitor_socket "
                           << radio_iface;
            }
        });

    return true;
}

bool slave_thread::handle_monitor_ap_metrics_response(const std::string &fronthaul_iface, int fd,
                                                      ieee1905_1::CmduMessageRx &cmdu_rx)
{
    LOG(DEBUG) << "Received AP_METRICS_QUERY_RESPONSE, mid=" << std::hex << cmdu_rx.getMessageId();

    if (!m_backhaul_manager_client->forward_cmdu(cmdu_rx)) {
        LOG(ERROR) << "Failed sending AP_METRICS_RESPONSE_MESSAGE message to backhaul_manager";
        return false;
    }
    return true;
}

bool slave_thread::handle_channel_preference_query(int fd, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    const auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received CHANNEL_PREFERENCE_QUERY_MESSAGE, mid=" << std::dec << int(mid);

    for (auto &radio_manager_map_element : m_radio_managers.get()) {
        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_CHANNELS_LIST_REQUEST>(cmdu_tx, mid);

        if (!request_out) {
            LOG(ERROR) << "Failed building message ACTION_APMANAGER_CHANNELS_LIST_REQUEST!";
            return false;
        }

        auto &radio_manager = radio_manager_map_element.second;
        send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
    }
    return true;
}

wfa_map::cPreferenceOperatingClasses::ePreference
slave_thread::get_channel_preference(beerocks::message::sWifiChannel channel,
                                     const sChannelPreference &preference,
                                     const std::set<uint8_t> &preference_channels_list)
{
    // According to Table 23 in the MultiAP Specification, an empty channel list field
    // indicates that the indicated preference applies to all channels in the operating class.
    if (preference_channels_list.empty()) {
        return wfa_map::cPreferenceOperatingClasses::ePreference(preference.flags.preference);
    }

    uint8_t center_channel = 0;
    auto bw                = static_cast<beerocks::eWiFiBandwidth>(channel.channel_bandwidth);
    auto operating_class   = wireless_utils::get_operating_class_by_channel(channel);

    LOG_IF(operating_class != preference.operating_class, FATAL)
        << "Invalid channel operating class " << int(operating_class)
        << ", preference operating class is " << int(preference.operating_class);

    // operating classes 128,129,130 use center channel **unlike the other classes**,
    // so convert channel and bandwidth to center channel.
    // For more info, refer to Table E-4 in the 802.11 specification.
    if (operating_class == 128 || operating_class == 129 || operating_class == 130) {
        center_channel = wireless_utils::get_5g_center_channel(channel.channel, bw);
    }

    // explicitely restrict non-operable channels
    auto channel_to_check =
        (operating_class == 128 || operating_class == 129 || operating_class == 130)
            ? center_channel
            : channel.channel;
    for (const auto ch : preference_channels_list) {
        if (channel_to_check == ch) {
            return wfa_map::cPreferenceOperatingClasses::ePreference(preference.flags.preference);
        }
    }
    // Default to the highest preference
    return wfa_map::cPreferenceOperatingClasses::ePreference::PREFERRED14;
}

beerocks::message::sWifiChannel
slave_thread::channel_selection_select_channel(const std::string &fronthaul_iface)
{
    auto db    = AgentDB::get();
    auto radio = db->radio(fronthaul_iface);
    if (!radio) {
        LOG(DEBUG) << "Radio of interface " << fronthaul_iface << " does not exist on the db";
        return {};
    }

    for (const auto &preference : m_controller_channel_preferences) {
        auto &preference_info         = preference.first;
        auto &preference_channel_list = preference.second;

        if (preference_channel_list.empty()) {
            continue;
        }
        for (const auto &channel_info_pair : radio->channels_list) {
            auto channel       = channel_info_pair.first;
            auto &channel_info = channel_info_pair.second;
            for (auto &bw_info : channel_info.supported_bw_list) {

                beerocks::message::sWifiChannel wifi_channel(channel, bw_info.bandwidth);
                auto operating_class = wireless_utils::get_operating_class_by_channel(wifi_channel);

                // Skip DFS channels
                if (channel_info.dfs_state != beerocks_message::eDfsState::NOT_DFS) {
                    LOG(DEBUG) << "Skip DFS channel " << channel << ", operating class "
                               << operating_class;
                    continue;
                }
                // Skip channels from other operating classes.
                if (operating_class != preference_info.operating_class) {
                    continue;
                }
                // Skip restricted channels
                if (get_channel_preference(wifi_channel, preference_info,
                                           preference_channel_list) ==
                    wfa_map::cPreferenceOperatingClasses::ePreference::NON_OPERABLE) {
                    LOG(DEBUG) << "Skip restricted channel " << channel << ", operating class "
                               << operating_class;
                    continue;
                }
                // If we got this far, we found a candidate channel, so switch to it
                LOG(DEBUG) << "Selected channel " << channel << ", operating class "
                           << operating_class;
                return wifi_channel;
            }
        }
    }

    LOG(ERROR) << "Could not find a suitable channel";
    return beerocks::message::sWifiChannel();
}

bool slave_thread::channel_selection_current_channel_restricted(const std::string &fronthaul_iface)
{
    auto db    = AgentDB::get();
    auto radio = db->radio(fronthaul_iface);
    if (!radio) {
        LOG(DEBUG) << "Radio of interface " << fronthaul_iface << " does not exist on the db";
        return false;
    }

    beerocks::message::sWifiChannel channel(radio->channel, radio->bandwidth);
    auto operating_class = wireless_utils::get_operating_class_by_channel(channel);

    if (operating_class == 0) {
        LOG(ERROR) << "Unknown operating class for bandwidth= " << channel.channel_bandwidth
                   << " channel=" << channel.channel
                   << ". Considering the channel to be restricted";
        return true;
    }

    LOG(DEBUG) << "Current channel " << int(channel.channel) << " bw "
               << beerocks::utils::convert_bandwidth_to_int(
                      beerocks::eWiFiBandwidth(channel.channel_bandwidth))
               << " oper_class " << int(operating_class);
    for (const auto &preference : m_controller_channel_preferences) {
        // for now we handle only non-operable preference
        // TODO - handle as part of https://github.com/prplfoundation/prplMesh/issues/725
        auto &preference_info         = preference.first;
        auto &preference_channel_list = preference.second;
        if (preference_info.flags.preference !=
            wfa_map::cPreferenceOperatingClasses::ePreference::NON_OPERABLE) {
            LOG(WARNING) << "Ignoring operable channels preference";
            continue;
        }
        // Skip channels from other operating classes.
        if (operating_class != preference_info.operating_class) {
            continue;
        }
        if (get_channel_preference(channel, preference_info, preference_channel_list) ==
            wfa_map::cPreferenceOperatingClasses::ePreference::NON_OPERABLE) {
            LOG(INFO) << "Current channel " << int(channel.channel)
                      << " restricted, channel switch required";
            return true;
        }
    }
    LOG(INFO) << "Current channel " << int(channel.channel)
              << " not restricted, channel switch not required";
    return false;
}

bool slave_thread::get_controller_channel_preference(const std::string &fronthaul_iface,
                                                     ieee1905_1::CmduMessageRx &cmdu_rx)
{
    m_controller_channel_preferences.clear();
    auto db    = AgentDB::get();
    auto radio = db->radio(fronthaul_iface);
    if (!radio) {
        LOG(DEBUG) << "Radio of interface " << fronthaul_iface << " does not exist on the db";
        return false;
    }

    for (auto channel_preference_tlv : cmdu_rx.getClassList<wfa_map::tlvChannelPreference>()) {

        const auto &ruid = channel_preference_tlv->radio_uid();
        if (ruid != radio->front.iface_mac) {
            LOG(DEBUG) << "ruid_rx=" << ruid << ", son_slave_ruid=" << radio->front.iface_mac;
            continue;
        }

        // read all operating class list
        auto operating_classes_list_length =
            channel_preference_tlv->operating_classes_list_length();

        for (int oc_idx = 0; oc_idx < operating_classes_list_length; oc_idx++) {
            std::stringstream ss;
            auto operating_class_tuple = channel_preference_tlv->operating_classes_list(oc_idx);
            if (!std::get<0>(operating_class_tuple)) {
                LOG(ERROR) << "getting operating class entry has failed!";
                return false;
            }
            auto &op_class_channels = std::get<1>(operating_class_tuple);
            auto operating_class    = op_class_channels.operating_class();

            auto channel_preference =
                sChannelPreference(op_class_channels.operating_class(), op_class_channels.flags());

            const auto &op_class_chan_set =
                wireless_utils::operating_class_to_channel_set(operating_class);
            ss << "operating class=" << +operating_class;

            auto channel_list_length = op_class_channels.channel_list_length();

            ss << ", preference=" << +channel_preference.flags.preference
               << ", reason=" << +channel_preference.flags.reason_code;
            ss << ", channel_list={";
            if (channel_list_length == 0) {
                ss << "}";
            }

            auto &channels_set = m_controller_channel_preferences[channel_preference];

            for (int ch_idx = 0; ch_idx < channel_list_length; ch_idx++) {
                auto channel = op_class_channels.channel_list(ch_idx);
                if (!channel) {
                    LOG(ERROR) << "getting channel entry has failed!";
                    return false;
                }

                // Check if channel is valid for operating class
                if (op_class_chan_set.find(*channel) == op_class_chan_set.end()) {
                    LOG(ERROR) << "Channel " << *channel << " invalid for operating class "
                               << operating_class;
                    return false;
                }

                ss << +(*channel);

                // add comma if not last channel in the list, else close list by add curl brackets
                ss << (((ch_idx + 1) != channel_list_length) ? "," : "}");

                channels_set.insert(*channel);
            }
            LOG(DEBUG) << ss.str();
        }
    }

    return true;
}

bool slave_thread::channel_selection_get_transmit_power_limit(const std::string &fronthaul_iface,
                                                              ieee1905_1::CmduMessageRx &cmdu_rx,
                                                              int &power_limit)
{
    auto db    = AgentDB::get();
    auto radio = db->radio(fronthaul_iface);
    if (!radio) {
        LOG(DEBUG) << "Radio of interface " << fronthaul_iface << " does not exist on the db";
        return false;
    }
    for (const auto &tx_power_limit_tlv : cmdu_rx.getClassList<wfa_map::tlvTransmitPowerLimit>()) {

        const auto &ruid = tx_power_limit_tlv->radio_uid();
        if (ruid != radio->front.iface_mac) {
            LOG(DEBUG) << "ruid_rx=" << ruid << ", son_slave_ruid=" << radio->front.iface_mac;
            continue;
        }

        power_limit = tx_power_limit_tlv->transmit_power_limit_dbm();
        LOG(DEBUG) << std::dec << "received tlvTransmitPowerLimit " << (int)power_limit;
        // Only one limit per ruid
        return true;
    }
    return false;
}

bool slave_thread::handle_channel_selection_request(int fd, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    const auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received CHANNEL_SELECTION_REQUEST_MESSAGE, mid=" << std::dec << int(mid);

    std::string fronthaul_iface;
    for (auto &radio_manager_map_element : m_radio_managers.get()) {
        fronthaul_iface     = radio_manager_map_element.first;
        auto &radio_manager = radio_manager_map_element.second;

        auto db    = AgentDB::get();
        auto radio = db->radio(fronthaul_iface);
        if (!radio) {
            return false;
        }

        int power_limit = 0;
        bool power_limit_received =
            channel_selection_get_transmit_power_limit(fronthaul_iface, cmdu_rx, power_limit);

        auto response_code = wfa_map::tlvChannelSelectionResponse::eResponseCode::ACCEPT;
        beerocks::message::sWifiChannel channel_to_switch;
        bool switch_required = false;
        if (get_controller_channel_preference(fronthaul_iface, cmdu_rx)) {
            // Only restricted channels are be included in channel selection request.
            if (channel_selection_current_channel_restricted(fronthaul_iface)) {
                channel_to_switch = channel_selection_select_channel(fronthaul_iface);
                if (channel_to_switch.channel != 0) {
                    switch_required = true;
                    LOG(INFO) << "Switch to channel " << channel_to_switch.channel << ", bw "
                              << beerocks::utils::convert_bandwidth_to_int(
                                     beerocks::eWiFiBandwidth(channel_to_switch.channel_bandwidth));
                } else {
                    LOG(INFO) << "Decline channel selection request " << radio->front.iface_mac;
                    response_code = wfa_map::tlvChannelSelectionResponse::eResponseCode::
                        DECLINE_VIOLATES_MOST_RECENTLY_REPORTED_PREFERENCES;
                }
            }
        } else {
            LOG(ERROR) << "Failed to update channel preference";
            response_code = wfa_map::tlvChannelSelectionResponse::eResponseCode::
                DECLINE_VIOLATES_MOST_RECENTLY_REPORTED_PREFERENCES;
        }

        // build and send channel response message
        if (!cmdu_tx.create(mid, ieee1905_1::eMessageType::CHANNEL_SELECTION_RESPONSE_MESSAGE)) {
            LOG(ERROR) << "cmdu creation of type CHANNEL_SELECTION_RESPONSE_MESSAGE, has failed";
            return false;
        }

        auto channel_selection_response_tlv =
            cmdu_tx.addClass<wfa_map::tlvChannelSelectionResponse>();
        if (!channel_selection_response_tlv) {
            LOG(ERROR) << "addClass ieee1905_1::tlvChannelSelectionResponse has failed";
            return false;
        }

        channel_selection_response_tlv->radio_uid()     = radio->front.iface_mac;
        channel_selection_response_tlv->response_code() = response_code;
        // Send ack back to the sender.
        if (!send_cmdu_to_controller(fronthaul_iface, cmdu_tx)) {
            LOG(ERROR) << "failed to send CHANNEL_SELECTION_RESPONSE_MESSAGE";
            return false;
        }

        // Normally, when a channel switch is required, a CSA notification
        // will be received with the new channel setting which is when
        // the agent will send the operating channel report.
        // In case of only a tx power limit change, there will still be
        // a CSA notification which will hold the new power limit and also
        // trigger sending the operating channel report.
        // If neither channel switch nor power limit change is required,
        // we need to explicitly send the event.
        if (!switch_required && !power_limit_received) {
            LOG(DEBUG) << "Channel switch not required, sending operating channel report";
            send_operating_channel_report(fronthaul_iface);
            continue;
        }

        auto request_out = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START>(cmdu_tx, mid);
        if (!request_out) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        LOG(DEBUG) << "send ACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START";

        // If only tx power limit change is required, set channel to current
        request_out->cs_params().channel =
            switch_required ? channel_to_switch.channel : radio->channel;
        request_out->cs_params().bandwidth =
            switch_required ? channel_to_switch.channel_bandwidth : uint8_t(radio->bandwidth);
        request_out->tx_limit()       = power_limit;
        request_out->tx_limit_valid() = power_limit_received;

        ///////////////////////////////////////////////////////////////////
        // TODO https://github.com/prplfoundation/prplMesh/issues/797
        //
        // Short term solution
        // In non-EasyMesh mode, never modify hostapd configuration
        // and in this case don't switch channel
        //
        ////////////////////////////////////////////////////////////////////
        if (db->device_conf.management_mode != BPL_MGMT_MODE_NOT_MULTIAP) {
            send_cmdu(radio_manager.ap_manager_fd, cmdu_tx);
        } else {
            LOG(WARNING) << "non-EasyMesh mode - skip channel switch";
        }
    }

    return true;
}

bool slave_thread::send_operating_channel_report(const std::string &fronthaul_iface)
{
    // build and send operating channel report message
    if (!cmdu_tx.create(0, ieee1905_1::eMessageType::OPERATING_CHANNEL_REPORT_MESSAGE)) {
        LOG(ERROR) << "cmdu creation of type OPERATING_CHANNEL_REPORT_MESSAGE, has failed";
        return false;
    }

    auto db    = AgentDB::get();
    auto radio = db->radio(fronthaul_iface);
    if (!radio) {
        LOG(DEBUG) << "Radio of interface " << fronthaul_iface << " does not exist on the db";
        return false;
    }

    auto operating_channel_report_tlv = cmdu_tx.addClass<wfa_map::tlvOperatingChannelReport>();
    if (!operating_channel_report_tlv) {
        LOG(ERROR) << "addClass ieee1905_1::operating_channel_report_tlv has failed";
        return false;
    }
    operating_channel_report_tlv->radio_uid() = radio->front.iface_mac;

    auto op_classes_list = operating_channel_report_tlv->alloc_operating_classes_list();
    if (!op_classes_list) {
        LOG(ERROR) << "alloc_operating_classes_list() has failed!";
        return false;
    }

    auto operating_class_entry_tuple = operating_channel_report_tlv->operating_classes_list(0);
    if (!std::get<0>(operating_class_entry_tuple)) {
        LOG(ERROR) << "getting operating class entry has failed!";
        return false;
    }

    auto &operating_class_entry = std::get<1>(operating_class_entry_tuple);
    beerocks::message::sWifiChannel channel;
    channel.channel_bandwidth = radio->bandwidth;
    channel.channel           = radio->channel;
    auto center_channel       = wireless_utils::freq_to_channel(radio->vht_center_frequency);
    auto operating_class      = wireless_utils::get_operating_class_by_channel(channel);

    operating_class_entry.operating_class = operating_class;
    // operating classes 128,129,130 use center channel **unlike the other classes** (See Table
    // E-4 in 802.11 spec)
    operating_class_entry.channel_number =
        (operating_class == 128 || operating_class == 129 || operating_class == 130)
            ? center_channel
            : channel.channel;
    operating_channel_report_tlv->current_transmit_power() = radio->tx_power_dB;

    return send_cmdu_to_controller(fronthaul_iface, cmdu_tx);
}

void slave_thread::fill_channel_list_to_agent_db(
    const std::string &fronthaul_iface,
    const std::shared_ptr<beerocks_message::cChannelList> &channel_list_class)
{
    if (!channel_list_class) {
        LOG(ERROR) << "Channel list is nullptr";
        return;
    }

    auto db    = AgentDB::get();
    auto radio = db->radio(fronthaul_iface);
    if (!radio) {
        return;
    }

    // Copy channels list to the AgentDB
    auto channels_list_length = channel_list_class->channels_list_length();
    for (uint8_t ch_idx = 0; ch_idx < channels_list_length; ch_idx++) {
        auto &channel_info = std::get<1>(channel_list_class->channels_list(ch_idx));
        auto channel       = channel_info.beacon_channel();
        radio->channels_list[channel].tx_power_dbm = channel_info.tx_power_dbm();
        radio->channels_list[channel].dfs_state    = channel_info.dfs_state();
        auto supported_bw_size                     = channel_info.supported_bandwidths_length();
        radio->channels_list[channel].supported_bw_list.resize(supported_bw_size);
        std::copy_n(&std::get<1>(channel_info.supported_bandwidths(0)), supported_bw_size,
                    radio->channels_list[channel].supported_bw_list.begin());

        for (const auto &supported_bw : radio->channels_list[channel].supported_bw_list) {
            LOG(DEBUG) << "channel=" << int(channel) << ", bw="
                       << beerocks::utils::convert_bandwidth_to_int(
                              beerocks::eWiFiBandwidth(supported_bw.bandwidth))
                       << ", rank=" << supported_bw.rank
                       << ", multiap_preference=" << int(supported_bw.multiap_preference);
        }
    }
}

void slave_thread::save_channel_params_to_db(const std::string &fronthaul_iface,
                                             beerocks_message::sApChannelSwitch params)
{
    auto db    = AgentDB::get();
    auto radio = db->radio(fronthaul_iface);
    if (!radio) {
        LOG(DEBUG) << "Radio of interface " << fronthaul_iface << " does not exist on the db";
        return;
    }

    radio->channel                   = params.channel;
    radio->bandwidth                 = static_cast<beerocks::eWiFiBandwidth>(params.bandwidth);
    radio->channel_ext_above_primary = params.channel_ext_above_primary;
    radio->vht_center_frequency      = params.vht_center_frequency;
    radio->tx_power_dB               = params.tx_power;
}

void slave_thread::save_cac_capabilities_params_to_db(const std::string &fronthaul_iface)
{
    auto db    = AgentDB::get();
    auto radio = db->radio(fronthaul_iface);
    if (!radio) {
        LOG(DEBUG) << "Radio of interface " << fronthaul_iface << " does not exist on the db";
        return;
    }
    if (son::wireless_utils::is_frequency_band_5ghz(radio->freq_type)) {
        AgentDB::sRadio::sCacCapabilities::sCacMethodCapabilities cac_capabilities_local;

        // we'll update the value when we receive cac-started event.
        // there is no way to query the hardware until a CAC is
        // actually performed.
        // Until PPM-855 is solved we will set the value to 10 minutes as default.
        cac_capabilities_local.cac_duration_sec = 600;

        for (const auto &channel_info_element : radio->channels_list) {
            auto channel       = channel_info_element.first;
            auto &channel_info = channel_info_element.second;
            if (channel_info.dfs_state == beerocks_message::eDfsState::NOT_DFS) {
                continue;
            }
            for (auto &bw_info : channel_info.supported_bw_list) {
                auto wifi_channel    = beerocks::message::sWifiChannel(channel, bw_info.bandwidth);
                auto operating_class = wireless_utils::get_operating_class_by_channel(wifi_channel);
                if (operating_class == 0) {
                    continue;
                }
                cac_capabilities_local.operating_classes[operating_class].push_back(
                    wifi_channel.channel);
            }
        }

        cac_capabilities_local.cac_method = wfa_map::eCacMethod::CONTINUOUS_CAC;

        // insert "regular" 5g
        radio->cac_capabilities.cac_method_capabilities.insert(
            std::make_pair(cac_capabilities_local.cac_method, cac_capabilities_local));

        // insert zwdfs 5g
        if (radio->front.zwdfs) {
            cac_capabilities_local.cac_method = wfa_map::eCacMethod::MIMO_DIMENSION_REDUCED;
            radio->cac_capabilities.cac_method_capabilities.insert(
                std::make_pair(cac_capabilities_local.cac_method, cac_capabilities_local));
        }
    }
}

std::map<slave_thread::sChannelPreference, std::set<uint8_t>>
slave_thread::get_channel_preferences_from_channels_list(const std::string &fronthaul_iface)
{
    std::map<sChannelPreference, std::set<uint8_t>> preferences;

    auto db    = AgentDB::get();
    auto radio = db->radio(fronthaul_iface);
    if (!radio) {
        return {};
    }

    for (const auto &oper_class : wireless_utils::operating_classes_list) {
        auto oper_class_num             = oper_class.first;
        const auto &oper_class_channels = oper_class.second.channels;
        auto oper_class_bw              = oper_class.second.band;

        for (auto channel_of_oper_class : oper_class_channels) {

            // Operating classes 128,129,130 use center channel **unlike the other classes**,
            // so convert channel and bandwidth to center channel.
            // For more info, refer to Table E-4 in the 802.11 specification.
            std::vector<uint8_t> beacon_channels;
            if (oper_class_num == 128 || oper_class_num == 129 || oper_class_num == 130) {
                beacon_channels = wireless_utils::center_channel_5g_to_beacon_channels(
                    channel_of_oper_class, oper_class_bw);
            } else {
                beacon_channels.push_back(channel_of_oper_class);
            }

            for (const auto beacon_channel : beacon_channels) {

                // Channel is not supported.
                auto it_ch = radio->channels_list.find(beacon_channel);
                if (it_ch == radio->channels_list.end()) {

                    sChannelPreference pref(
                        oper_class_num,
                        wfa_map::cPreferenceOperatingClasses::ePreference::NON_OPERABLE,
                        wfa_map::cPreferenceOperatingClasses::eReasonCode::UNSPECIFIED);
                    preferences[pref].insert(channel_of_oper_class);
                    break;
                }

                // Bandwidth of a channel is not supported.
                auto &supported_channel_info = it_ch->second;
                auto &supported_bw_list      = supported_channel_info.supported_bw_list;
                auto it_bw =
                    std::find_if(supported_bw_list.begin(), supported_bw_list.end(),
                                 [&](const beerocks_message::sSupportedBandwidth &bw_info) {
                                     return bw_info.bandwidth == oper_class_bw;
                                 });
                if (it_bw == supported_bw_list.end()) {
                    sChannelPreference pref(
                        oper_class_num,
                        wfa_map::cPreferenceOperatingClasses::ePreference::NON_OPERABLE,
                        wfa_map::cPreferenceOperatingClasses::eReasonCode::UNSPECIFIED);
                    preferences[pref].insert(channel_of_oper_class);
                    break;
                }

                // The rest of the checks are relevant only for 5 GHz band.
                if (son::wireless_utils::channels_table_5g.find(beacon_channel) !=
                    son::wireless_utils::channels_table_5g.end()) {

                    // Channel DFS state is "Unavailable".
                    auto overlapping_beacon_channels =
                        son::wireless_utils::get_overlapping_beacon_channels(beacon_channel,
                                                                             oper_class_bw);

                    auto preference_size = preferences.size();
                    for (const auto overlap_ch : overlapping_beacon_channels) {
                        it_ch = radio->channels_list.find(overlap_ch);
                        if (it_ch == radio->channels_list.end()) {
                            LOG(ERROR) << "Overlap channel " << overlap_ch << " is not supported";
                            sChannelPreference pref(
                                oper_class_num,
                                wfa_map::cPreferenceOperatingClasses::ePreference::NON_OPERABLE,
                                wfa_map::cPreferenceOperatingClasses::eReasonCode::UNSPECIFIED);
                            preferences[pref].insert(channel_of_oper_class);
                            break;
                        }

                        auto &overlap_channel_info = it_ch->second;

                        if (overlap_channel_info.dfs_state ==
                            beerocks_message::eDfsState::UNAVAILABLE) {
                            sChannelPreference pref(
                                oper_class_num,
                                wfa_map::cPreferenceOperatingClasses::ePreference::NON_OPERABLE,
                                wfa_map::cPreferenceOperatingClasses::eReasonCode::
                                    OPERATION_DISALLOWED_DUE_TO_RADAR_DETECTION_ON_A_DFS_CHANNEL);
                            preferences[pref].insert(channel_of_oper_class);
                            break;
                        }
                    }

                    // If an unavailable channel has been inserted, skip to the next channel and not
                    // add a valid preference (code below). This is because the checks above are
                    // done in internal for-loop in which the break calls inside it, breaks the
                    // internal loop.
                    if (preference_size != preferences.size()) {
                        break;
                    }
                }

                /**
                 * For now do not insert the real channel preference. It will be uncomment in
                 * a separated Merge Request after testing it. PPM-655.
                 */

                // // Channel is supported and have valid preference.
                // sChannelPreference pref(
                //     oper_class_num,
                //     static_cast<wfa_map::cPreferenceOperatingClasses::ePreference>(
                //         it_bw->multiap_preference),
                //     wfa_map::cPreferenceOperatingClasses::eReasonCode::UNSPECIFIED);
                // preferences[pref].insert(channel_of_oper_class);
            }
        }
    }
    return preferences;
}

bool slave_thread::update_vaps_info(const std::string &iface,
                                    const beerocks_message::sVapInfo *vaps)
{
    auto db    = AgentDB::get();
    auto radio = db->radio(iface);
    if (!radio) {
        return false;
    }
    for (uint8_t vap_idx = 0; vap_idx < eBeeRocksIfaceIds::IFACE_TOTAL_VAPS; vap_idx++) {
        auto &bss         = radio->front.bssids[vap_idx];
        bss.mac           = vaps[vap_idx].mac;
        bss.ssid          = vaps[vap_idx].ssid;
        bss.fronthaul_bss = vaps[vap_idx].fronthaul_vap;
        bss.backhaul_bss  = vaps[vap_idx].backhaul_vap;
        bss.backhaul_bss_disallow_profile1_agent_association =
            vaps[vap_idx].profile1_backhaul_sta_association_disallowed;
        bss.backhaul_bss_disallow_profile2_agent_association =
            vaps[vap_idx].profile2_backhaul_sta_association_disallowed;

        if (vaps[vap_idx].mac != network_utils::ZERO_MAC) {
            LOG(DEBUG) << "BSS " << bss.mac << ", ssid:" << bss.ssid
                       << ", fBSS: " << bss.fronthaul_bss << ", bBSS: " << bss.backhaul_bss
                       << ", p1_dis: " << bss.backhaul_bss_disallow_profile1_agent_association
                       << ", p2_dis: " << bss.backhaul_bss_disallow_profile2_agent_association;
        }
    }
    return true;
}
