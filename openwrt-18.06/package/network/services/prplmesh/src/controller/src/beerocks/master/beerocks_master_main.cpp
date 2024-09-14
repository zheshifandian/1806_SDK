/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_cmdu_server_factory.h>
#include <bcl/beerocks_config_file.h>
#include <bcl/beerocks_event_loop_impl.h>
#include <bcl/beerocks_logging.h>
#include <bcl/beerocks_timer_factory_impl.h>
#include <bcl/beerocks_timer_manager_impl.h>
#include <bcl/beerocks_ucc_server_factory.h>
#include <bcl/beerocks_version.h>
#include <bcl/network/network_utils.h>
#include <bcl/network/sockets_impl.h>
#include <bpl/bpl_cfg.h>
#include <btl/broker_client_factory_factory.h>
#include <mapf/common/utils.h>

#include <easylogging++.h>

#include "controller.h"
#include "db/db.h"

#ifdef ENABLE_NBAPI
#include "ambiorix_impl.h"
#include "on_action.h"

#ifndef AMBIORIX_BACKEND_PATH
#define AMBIORIX_BACKEND_PATH "/usr/bin/mods/amxb/mod-amxb-ubus.so"
#endif // AMBIORIX_BACKEND_PATH

#ifndef AMBIORIX_BUS_URI
#define AMBIORIX_BUS_URI "ubus:"
#endif // AMBIORIX_BUS_URI

#ifndef CONTROLLER_DATAMODEL_PATH
#define CONTROLLER_DATAMODEL_PATH "config/odl/controller.odl"
#endif

#endif //#else // ENABLE_NBAPI

#include "ambiorix_dummy.h"

//#endif

// Do not use this macro anywhere else in ire process
// It should only be there in one place in each executable module
BEEROCKS_INIT_BEEROCKS_VERSION

static bool g_running     = true;
static bool s_kill_master = false;
static int s_signal       = 0;

// Pointer to logger instance
static beerocks::logging *s_pLogger = nullptr;

static void handle_signal()
{
    if (!s_signal)
        return;

    switch (s_signal) {

    // Terminate
    case SIGTERM:
    case SIGINT:
        LOG(INFO) << "Caught signal '" << strsignal(s_signal) << "' Exiting...";
        g_running = false;
        break;

    // Roll log file
    case SIGUSR1: {
        LOG(INFO) << "LOG Roll Signal!";
        if (!s_pLogger) {
            LOG(ERROR) << "Invalid logger pointer!";
            return;
        }

        s_pLogger->apply_settings();
        LOG(INFO) << "--- Start of file after roll ---";
        break;
    }

    default:
        LOG(WARNING) << "Unhandled Signal: '" << strsignal(s_signal) << "' Ignoring...";
        break;
    }

    s_signal = 0;
}

static void init_signals()
{
    // Signal handler function
    auto signal_handler = [](int signum) { s_signal = signum; };

    struct sigaction sigterm_action;
    sigterm_action.sa_handler = signal_handler;
    sigemptyset(&sigterm_action.sa_mask);
    sigterm_action.sa_flags = 0;
    sigaction(SIGTERM, &sigterm_action, NULL);

    struct sigaction sigint_action;
    sigint_action.sa_handler = signal_handler;
    sigemptyset(&sigint_action.sa_mask);
    sigint_action.sa_flags = 0;
    sigaction(SIGINT, &sigint_action, NULL);

    struct sigaction sigusr1_action;
    sigusr1_action.sa_handler = signal_handler;
    sigemptyset(&sigusr1_action.sa_mask);
    sigusr1_action.sa_flags = 0;
    sigaction(SIGUSR1, &sigusr1_action, NULL);
}

static bool parse_arguments(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "k")) != -1) {
        switch (opt) {
        case 'k': {
            s_kill_master = true;
            break;
        }
        case '?': {
            return false;
        }
        }
    }
    return true;
}

static void fill_master_config(son::db::sDbMasterConfig &master_conf,
                               const beerocks::config_file::sConfigMaster &main_master_conf)
{
    master_conf.vendor = main_master_conf.vendor;
    master_conf.model  = main_master_conf.model;
    master_conf.ucc_listener_port =
        beerocks::string_utils::stoi(main_master_conf.ucc_listener_port);
    master_conf.load_ire_roaming           = (main_master_conf.load_ire_roaming == "1");
    master_conf.load_service_fairness      = (main_master_conf.load_service_fairness == "1");
    master_conf.load_dfs_reentry           = (main_master_conf.load_dfs_reentry == "1");
    master_conf.load_rdkb_extensions       = (main_master_conf.load_rdkb_extensions == "1");
    master_conf.load_client_11k_roaming    = (main_master_conf.load_client_11k_roaming == "1");
    master_conf.load_legacy_client_roaming = (main_master_conf.load_legacy_client_roaming == "1");
    master_conf.load_load_balancing        = (main_master_conf.load_load_balancing == "1");
    master_conf.load_diagnostics_measurements =
        (main_master_conf.load_diagnostics_measurements == "1");
    master_conf.load_backhaul_measurements = (main_master_conf.load_backhaul_measurements == "1");
    master_conf.load_front_measurements    = (main_master_conf.load_front_measurements == "1");
    master_conf.load_health_check          = (main_master_conf.load_health_check == "1");
    master_conf.load_monitor_on_vaps       = (main_master_conf.load_monitor_on_vaps == "1");
    master_conf.diagnostics_measurements_polling_rate_sec =
        beerocks::string_utils::stoi(main_master_conf.diagnostics_measurements_polling_rate_sec);
    master_conf.ire_rssi_report_rate_sec =
        beerocks::string_utils::stoi(main_master_conf.ire_rssi_report_rate_sec);

    master_conf.roaming_unconnected_client_rssi_compensation_db = beerocks::string_utils::stoi(
        main_master_conf.roaming_unconnected_client_rssi_compensation_db);
    master_conf.roaming_hop_percent_penalty =
        beerocks::string_utils::stoi(main_master_conf.roaming_hop_percent_penalty);
    master_conf.roaming_band_pathloss_delta_db =
        beerocks::string_utils::stoi(main_master_conf.roaming_band_pathloss_delta_db);
    master_conf.roaming_5ghz_failed_attemps_threshold =
        beerocks::string_utils::stoi(main_master_conf.roaming_5ghz_failed_attemps_threshold);
    master_conf.roaming_24ghz_failed_attemps_threshold =
        beerocks::string_utils::stoi(main_master_conf.roaming_24ghz_failed_attemps_threshold);
    master_conf.roaming_11v_failed_attemps_threshold =
        beerocks::string_utils::stoi(main_master_conf.roaming_11v_failed_attemps_threshold);
    master_conf.roaming_rssi_cutoff_db =
        beerocks::string_utils::stoi(main_master_conf.roaming_rssi_cutoff_db);
    master_conf.monitor_total_ch_load_notification_lo_th_percent = beerocks::string_utils::stoi(
        main_master_conf.monitor_total_channel_load_notification_lo_th_percent);
    master_conf.monitor_total_ch_load_notification_hi_th_percent = beerocks::string_utils::stoi(
        main_master_conf.monitor_total_channel_load_notification_hi_th_percent);
    master_conf.monitor_total_ch_load_notification_delta_th_percent = beerocks::string_utils::stoi(
        main_master_conf.monitor_total_channel_load_notification_delta_th_percent);
    master_conf.monitor_min_active_clients =
        beerocks::string_utils::stoi(main_master_conf.monitor_min_active_clients);
    master_conf.monitor_active_client_th =
        beerocks::string_utils::stoi(main_master_conf.monitor_active_client_th);
    master_conf.monitor_client_load_notification_delta_th_percent = beerocks::string_utils::stoi(
        main_master_conf.monitor_client_load_notification_delta_th_percent);
    master_conf.monitor_ap_idle_threshold_B =
        beerocks::string_utils::stoi(main_master_conf.monitor_ap_idle_threshold_B);
    master_conf.monitor_ap_active_threshold_B =
        beerocks::string_utils::stoi(main_master_conf.monitor_ap_active_threshold_B);
    master_conf.monitor_ap_idle_stable_time_sec =
        beerocks::string_utils::stoi(main_master_conf.monitor_ap_idle_stable_time_sec);
    master_conf.monitor_rx_rssi_notification_threshold_dbm =
        beerocks::string_utils::stoi(main_master_conf.monitor_rx_rssi_notification_threshold_dbm);
    master_conf.monitor_rx_rssi_notification_delta_db =
        beerocks::string_utils::stoi(main_master_conf.monitor_rx_rssi_notification_delta_db);
    master_conf.monitor_disable_initiative_arp =
        beerocks::string_utils::stoi(main_master_conf.monitor_disable_initiative_arp);
    master_conf.channel_selection_random_delay =
        beerocks::string_utils::stoi(main_master_conf.channel_selection_random_delay);
    master_conf.fail_safe_5G_frequency =
        beerocks::string_utils::stoi(main_master_conf.fail_safe_5G_frequency);
    master_conf.fail_safe_5G_bw = beerocks::string_utils::stoi(main_master_conf.fail_safe_5G_bw);
    master_conf.fail_safe_5G_vht_frequency =
        beerocks::string_utils::stoi(main_master_conf.fail_safe_5G_vht_frequency);
    master_conf.channel_selection_long_delay =
        beerocks::string_utils::stoi(main_master_conf.channel_selection_long_delay);
    master_conf.roaming_sticky_client_rssi_threshold =
        beerocks::string_utils::stoi(main_master_conf.roaming_sticky_client_rssi_threshold);
    master_conf.credentials_change_timeout_sec =
        beerocks::string_utils::stoi(main_master_conf.credentials_change_timeout_sec);
    // get channel vector
    std::string s         = main_master_conf.global_restricted_channels;
    std::string delimiter = ",";

    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        master_conf.global_restricted_channels.push_back(beerocks::string_utils::stoi(token));
        s.erase(0, pos + delimiter.length());
    }
    if (!s.empty()) {
        master_conf.global_restricted_channels.push_back(beerocks::string_utils::stoi(s));
    }

    // platform settings
    master_conf.certification_mode = beerocks::bpl::cfg_get_certification_mode();
    char load_steer_on_vaps[BPL_LOAD_STEER_ON_VAPS_LEN] = {0};
    if (beerocks::bpl::cfg_get_load_steer_on_vaps(beerocks::IRE_MAX_SLAVES, load_steer_on_vaps) <
        0) {
        master_conf.load_steer_on_vaps = std::string();
    } else {
        master_conf.load_steer_on_vaps = std::string(load_steer_on_vaps);
    }

    if (!beerocks::bpl::cfg_get_persistent_db_enable(master_conf.persistent_db)) {
        LOG(DEBUG) << "Failed to read persistent db enable, setting to default value: "
                   << bool(beerocks::bpl::DEFAULT_PERSISTENT_DB);
        master_conf.persistent_db = bool(beerocks::bpl::DEFAULT_PERSISTENT_DB);
    }
    if (!beerocks::bpl::cfg_get_clients_persistent_db_max_size(
            master_conf.clients_persistent_db_max_size)) {
        LOG(DEBUG)
            << "Failed to read max number of clients in persistent db, setting to default value: "
            << beerocks::bpl::DEFAULT_CLIENTS_PERSISTENT_DB_MAX_SIZE;
        master_conf.clients_persistent_db_max_size =
            beerocks::bpl::DEFAULT_CLIENTS_PERSISTENT_DB_MAX_SIZE;
    }
    if (!beerocks::bpl::cfg_get_steer_history_persistent_db_max_size(
            master_conf.steer_history_persistent_db_max_size)) {
        LOG(DEBUG) << "Failed to read max number of steer history in persistent db, setting to "
                      "default value: "
                   << beerocks::bpl::DEFAULT_STEER_HISTORY_PERSISTENT_DB_MAX_SIZE;
        master_conf.steer_history_persistent_db_max_size =
            beerocks::bpl::DEFAULT_STEER_HISTORY_PERSISTENT_DB_MAX_SIZE;
    }
    if (!beerocks::bpl::cfg_get_max_timelife_delay_minutes(
            master_conf.max_timelife_delay_minutes)) {
        LOG(DEBUG)
            << "Failed to read max lifetime of clients in persistent db, setting to default value: "
            << beerocks::bpl::DEFAULT_MAX_TIMELIFE_DELAY_MINUTES << " minutes";
        master_conf.max_timelife_delay_minutes = beerocks::bpl::DEFAULT_MAX_TIMELIFE_DELAY_MINUTES;
    }
    if (!beerocks::bpl::cfg_get_unfriendly_device_max_timelife_delay_minutes(
            master_conf.unfriendly_device_max_timelife_delay_minutes)) {
        LOG(DEBUG) << "Failed to read max lifetime of unfriendly clients in persistent db, setting "
                      "to default value: "
                   << beerocks::bpl::DEFAULT_UNFRIENDLY_DEVICE_MAX_TIMELIFE_DELAY_MINUTES
                   << " minutes";
        master_conf.unfriendly_device_max_timelife_delay_minutes =
            beerocks::bpl::DEFAULT_UNFRIENDLY_DEVICE_MAX_TIMELIFE_DELAY_MINUTES;
    }
    if (!beerocks::bpl::cfg_get_persistent_db_aging_interval(
            master_conf.persistent_db_aging_interval)) {
        LOG(DEBUG) << "Failed to read persistent DB aging interval in persistent db, setting "
                      "to default value: "
                   << beerocks::bpl::DEFAULT_PERSISTENT_DB_AGING_INTERVAL_SEC << " seconds";
        master_conf.persistent_db_aging_interval =
            beerocks::bpl::DEFAULT_PERSISTENT_DB_AGING_INTERVAL_SEC;
    }
    if (!beerocks::bpl::cfg_get_persistent_db_commit_changes_interval(
            master_conf.persistent_db_commit_changes_interval_seconds)) {
        LOG(DEBUG) << "Failed to read commit_changes interval, setting to default value: "
                   << beerocks::bpl::DEFAULT_COMMIT_CHANGES_INTERVAL_VALUE_SEC;

        master_conf.persistent_db_commit_changes_interval_seconds =
            beerocks::bpl::DEFAULT_COMMIT_CHANGES_INTERVAL_VALUE_SEC;
    }
    if (!beerocks::bpl::cfg_get_link_metrics_request_interval(
            master_conf.link_metrics_request_interval_seconds)) {
        LOG(DEBUG) << "Failed to read link_metrics_request interval, setting to default value: "
                   << beerocks::bpl::DEFAULT_LINK_METRICS_REQUEST_INTERVAL_VALUE_SEC.count();

        master_conf.link_metrics_request_interval_seconds =
            beerocks::bpl::DEFAULT_LINK_METRICS_REQUEST_INTERVAL_VALUE_SEC;
    }

    master_conf.dhcp_monitor_interval_seconds =
        beerocks::bpl::DEFAULT_DHCP_MONITOR_INTERVAL_VALUE_SEC;

    if (!beerocks::bpl::cfg_get_band_steering(master_conf.load_client_band_steering)) {
        LOG(DEBUG) << "Failed to read cfg_get_band_steering, setting to default value: "
                   << beerocks::bpl::DEFAULT_BAND_STEERING;

        master_conf.load_client_band_steering = beerocks::bpl::DEFAULT_BAND_STEERING;
    }

    if (!beerocks::bpl::cfg_get_client_roaming(master_conf.load_client_optimal_path_roaming)) {
        LOG(DEBUG) << "Failed to read cfg_get_client_roaming, setting to default value: "
                   << beerocks::bpl::DEFAULT_CLIENT_ROAMING;

        master_conf.load_client_optimal_path_roaming = beerocks::bpl::DEFAULT_CLIENT_ROAMING;
    }

    if (!beerocks::bpl::cfg_get_roaming_hysteresis_percent_bonus(
            master_conf.roaming_hysteresis_percent_bonus)) {
        LOG(DEBUG) << "Failed to read roaming_hysteresis_percent_bonus, setting to default value: "
                   << beerocks::bpl::DEFAULT_ROAMING_HYSTERESIS_PERCENT_BONUS;

        master_conf.roaming_hysteresis_percent_bonus =
            beerocks::bpl::DEFAULT_ROAMING_HYSTERESIS_PERCENT_BONUS;
    }

    if (!beerocks::bpl::cfg_get_steering_disassoc_timer_msec(
            master_conf.steering_disassoc_timer_msec)) {
        LOG(DEBUG) << "Failed to read steering_disassoc_timer_msec, setting to default value: "
                   << beerocks::bpl::DEFAULT_STEERING_DISASSOC_TIMER_MSEC.count();

        master_conf.steering_disassoc_timer_msec =
            beerocks::bpl::DEFAULT_STEERING_DISASSOC_TIMER_MSEC;
    }

    if ((master_conf.management_mode = beerocks::bpl::cfg_get_management_mode()) < 0) {
        LOG(DEBUG) << "Failed to read management mode, setting to default value: "
                   << BPL_MGMT_MODE_MULTIAP_CONTROLLER_AGENT;

        master_conf.management_mode = BPL_MGMT_MODE_MULTIAP_CONTROLLER_AGENT;
    }

    if (!beerocks::bpl::cfg_get_unsuccessful_assoc_report_policy(
            master_conf.unsuccessful_assoc_report_policy)) {
        LOG(DEBUG) << "Failed to read unsuccessful_assoc_report_policy, setting to "
                      "default value: "
                   << beerocks::bpl::DEFAULT_UNSUCCESSFUL_ASSOC_REPORT_POLICY;

        master_conf.unsuccessful_assoc_report_policy =
            beerocks::bpl::DEFAULT_UNSUCCESSFUL_ASSOC_REPORT_POLICY;
    }

    if (!beerocks::bpl::cfg_get_unsuccessful_assoc_max_reporting_rate(
            master_conf.unsuccessful_assoc_max_reporting_rate)) {
        LOG(DEBUG) << "Failed to read unsuccessful_assoc_max_reporting_rate, setting to "
                      "default value: "
                   << beerocks::bpl::DEFAULT_UNSUCCESSFUL_ASSOC_MAX_REPORTING_RATE;

        master_conf.unsuccessful_assoc_max_reporting_rate =
            beerocks::bpl::DEFAULT_UNSUCCESSFUL_ASSOC_MAX_REPORTING_RATE;
    }
}

/**
 * @brief Fills Device.WiFi.DataElements.Configuration datamodels according to master config.
 *
 * @param ambiorix_datamodel datamodel pointer.
 * @param master_conf master configuration object to read settings.
 * @return True if success otherwise false.
 */
static bool
fill_nbapi_config_from_master_conf(std::shared_ptr<beerocks::nbapi::Ambiorix> ambiorix_datamodel,
                                   son::db::sDbMasterConfig &master_conf)
{
    bool ret_val = true;

    // ambiorix->set methods trigger data change event. It is not harmfull, but needed to be remembered.

    ret_val &=
        ambiorix_datamodel->set("Device.WiFi.DataElements.Configuration", "BandSteeringEnabled",
                                master_conf.load_client_band_steering);

    ret_val &=
        ambiorix_datamodel->set("Device.WiFi.DataElements.Configuration", "ClientSteeringEnabled",
                                master_conf.load_client_optimal_path_roaming);

    ret_val &=
        ambiorix_datamodel->set("Device.WiFi.DataElements.Configuration", "SteeringCurrentBonus",
                                master_conf.roaming_hysteresis_percent_bonus);

    ret_val &= ambiorix_datamodel->set("Device.WiFi.DataElements.Configuration",
                                       "SteeringDisassociationTimer",
                                       master_conf.steering_disassoc_timer_msec.count());

    ret_val &= ambiorix_datamodel->set("Device.WiFi.DataElements.Configuration",
                                       "LinkMetricsRequestInterval",
                                       master_conf.link_metrics_request_interval_seconds.count());

    return ret_val;
}

int main(int argc, char *argv[])
{
    std::cout << "Beerocks Controller Process Start" << std::endl;

    init_signals();

    // Check for version query first, handle and exit if requested.
    std::string module_description;
    std::ofstream versionfile;
    if (beerocks::version::handle_version_query(argc, argv, module_description)) {
        return 0;
    }

    //get command line options
    if (!parse_arguments(argc, argv)) {
        std::cout << "Usage: " << argv[0] << " -k {kill master}" << std::endl;
        return 1;
    }

    // read master config file
    std::string master_config_file_path =
        CONF_FILES_WRITABLE_PATH + std::string(BEEROCKS_CONTROLLER) +
        ".conf"; //search first in platform-specific default directory
    beerocks::config_file::sConfigMaster beerocks_master_conf;
    if (!beerocks::config_file::read_master_config_file(master_config_file_path,
                                                        beerocks_master_conf)) {
        master_config_file_path = mapf::utils::get_install_path() + "config/" +
                                  std::string(BEEROCKS_CONTROLLER) +
                                  ".conf"; // if not found, search in beerocks path
        if (!beerocks::config_file::read_master_config_file(master_config_file_path,
                                                            beerocks_master_conf)) {
            std::cout << "config file '" << master_config_file_path << "' args error." << std::endl;
            return 1;
        }
    }

    // read slave config file
    std::string slave_config_file_path =
        CONF_FILES_WRITABLE_PATH + std::string(BEEROCKS_AGENT) +
        ".conf"; //search first in platform-specific default directory
    beerocks::config_file::sConfigSlave beerocks_slave_conf;
    if (!beerocks::config_file::read_slave_config_file(slave_config_file_path,
                                                       beerocks_slave_conf)) {
        slave_config_file_path = mapf::utils::get_install_path() + "config/" +
                                 std::string(BEEROCKS_AGENT) +
                                 ".conf"; // if not found, search in beerocks path
        if (!beerocks::config_file::read_slave_config_file(slave_config_file_path,
                                                           beerocks_slave_conf)) {
            std::cout << "config file '" << slave_config_file_path << "' args error." << std::endl;
            return 1;
        }
    }

    std::string base_master_name = std::string(BEEROCKS_CONTROLLER);

    //kill running master
    beerocks::os_utils::kill_pid(beerocks_master_conf.temp_path + "pid/", base_master_name);

    // only kill and exit
    if (s_kill_master) {
        return 0;
    }

    //init logger
    beerocks::logging logger(base_master_name, beerocks_master_conf.sLog);
    s_pLogger = &logger;
    logger.apply_settings();
    LOG(INFO) << std::endl
              << "Running " << base_master_name << " Version " << BEEROCKS_VERSION << " Build date "
              << BEEROCKS_BUILD_DATE << std::endl
              << std::endl;
    beerocks::version::log_version(argc, argv);
    versionfile.open(beerocks_master_conf.temp_path + "beerocks_master_version");
    versionfile << BEEROCKS_VERSION << std::endl << BEEROCKS_REVISION;
    versionfile.close();

    // Redirect stdout / stderr
    if (logger.get_log_files_enabled()) {
        beerocks::os_utils::redirect_console_std(beerocks_master_conf.sLog.files_path +
                                                 base_master_name + "_std.log");
    }

    //write pid file
    beerocks::os_utils::write_pid_file(beerocks_master_conf.temp_path, base_master_name);
    std::string pid_file_path =
        beerocks_master_conf.temp_path + "pid/" + base_master_name; // for file touching

    // fill master configuration
    son::db::sDbMasterConfig master_conf;
    fill_master_config(master_conf, beerocks_master_conf);

    // Create application event loop to wait for blocking I/O operations.
    auto event_loop = std::make_shared<beerocks::EventLoopImpl>();
    LOG_IF(!event_loop, FATAL) << "Unable to create event loop!";

    // Create timer factory to create instances of timers.
    auto timer_factory = std::make_shared<beerocks::TimerFactoryImpl>();
    LOG_IF(!timer_factory, FATAL) << "Unable to create timer factory!";

    // Create timer manager to help using application timers.
    auto timer_manager = std::make_shared<beerocks::TimerManagerImpl>(timer_factory, event_loop);
    LOG_IF(!timer_manager, FATAL) << "Unable to create timer manager!";

    // Create UDS address where the server socket will listen for incoming connection requests.
    std::string uds_path =
        beerocks_slave_conf.temp_path + "/" + std::string(BEEROCKS_CONTROLLER_UDS);
    auto uds_address = beerocks::net::UdsAddress::create_instance(uds_path);
    LOG_IF(!uds_address, FATAL) << "Unable to create UDS server address!";

    // Create server to exchange CMDU messages with clients connected through a UDS socket
    auto cmdu_server = beerocks::CmduServerFactory::create_instance(uds_address, event_loop);
    LOG_IF(!cmdu_server, FATAL) << "Unable to create CMDU server!";

    beerocks::net::network_utils::iface_info bridge_info;
    const auto &bridge_iface = beerocks_slave_conf.bridge_iface;
    if (beerocks::net::network_utils::get_iface_info(bridge_info, bridge_iface) != 0) {
        LOG(ERROR) << "Failed reading addresses from the bridge!";
        return 0;
    }

#ifdef ENABLE_NBAPI
    // Prepare vector with actions: name and pointer to function
    auto on_action_handlers = prplmesh::controller::actions::get_actions_callback_list();
    auto events_list        = prplmesh::controller::actions::get_events_list();
    auto funcs_list         = prplmesh::controller::actions::get_func_list();

    auto controller_dm_path = mapf::utils::get_install_path() + "config/odl/controller.odl";
    auto amb_dm_obj         = std::make_shared<beerocks::nbapi::AmbiorixImpl>(
        event_loop, on_action_handlers, events_list, funcs_list);
    LOG_IF(!amb_dm_obj, FATAL) << "Unable to create Ambiorix!";
    LOG_IF(!amb_dm_obj->init(AMBIORIX_BACKEND_PATH, AMBIORIX_BUS_URI, controller_dm_path), FATAL)
        << "Unable to init ambiorix object!";
#else
    auto amb_dm_obj = std::make_shared<beerocks::nbapi::AmbiorixDummy>();
#endif //ENABLE_NBAPI

    // Set Network.ID to the Data Model
    if (!amb_dm_obj->set("Device.WiFi.DataElements.Network", "ID", bridge_info.mac)) {
        LOG(ERROR) << "Failed to add Network.ID, mac: " << bridge_info.mac;
        return false;
    }

    if (!amb_dm_obj->set("Device.WiFi.DataElements.Network", "ControllerID", bridge_info.mac)) {
        LOG(ERROR) << "Failed to add Network.ControllerID, mac: " << bridge_info.mac;
        return false;
    }

    son::db master_db(master_conf, logger, tlvf::mac_from_string(bridge_info.mac), amb_dm_obj);

#ifdef ENABLE_NBAPI
    prplmesh::controller::actions::g_database   = &master_db;
    prplmesh::controller::actions::g_data_model = beerocks::nbapi::g_data_model;
#endif

    fill_nbapi_config_from_master_conf(amb_dm_obj, master_conf);

    // The prplMesh controller needs to be configured with the SSIDs and credentials that have to
    // be configured on the agents. Even though NBAPI exists to configure this, there is a lot of
    // existing software out there that doesn't use it. Therefore, prplMesh should also read the
    // configuration out of the legacy wireless settings.
    std::list<son::wireless_utils::sBssInfoConf> wireless_settings;
    if (beerocks::bpl::bpl_cfg_get_wireless_settings(wireless_settings)) {
        for (const auto &configuration : wireless_settings) {
            master_db.add_bss_info_configuration(configuration);
        }
    } else {
        LOG(DEBUG) << "failed to read wireless settings";
    }

    // diagnostics_thread diagnostics(master_db);

    // UCC server must be created in certification mode only and if a valid TCP port has been set
    uint16_t port = master_db.config.ucc_listener_port;
    std::unique_ptr<beerocks::UccServer> ucc_server;
    if (master_db.setting_certification_mode() && (port != 0)) {

        LOG(INFO) << "Certification mode enabled (listening on port " << port << ")";

        // Create server to exchange UCC commands and replies with clients connected through the socket
        ucc_server = beerocks::UccServerFactory::create_instance(port, event_loop);
        LOG_IF(!ucc_server, FATAL) << "Unable to create UCC server!";
    }

    // Create broker client factory to create broker clients when requested
    std::string broker_uds_path =
        beerocks_slave_conf.temp_path + "/" + std::string(BEEROCKS_BROKER_UDS);
    auto broker_client_factory =
        beerocks::btl::create_broker_client_factory(broker_uds_path, event_loop);
    LOG_IF(!broker_client_factory, FATAL) << "Unable to create broker client factory!";

    son::Controller controller(master_db, std::move(broker_client_factory), std::move(ucc_server),
                               std::move(cmdu_server), timer_manager, event_loop);

    if (!amb_dm_obj->set_current_time("Device.WiFi.DataElements.Network")) {
        return false;
    };

    LOG_IF(!controller.start(), FATAL) << "Unable to start controller!";

    auto touch_time_stamp_timeout = std::chrono::steady_clock::now();
    while (g_running) {
        // Handle signals
        if (s_signal) {
            handle_signal();
            continue;
        }

        if (std::chrono::steady_clock::now() > touch_time_stamp_timeout) {
            beerocks::os_utils::touch_pid_file(pid_file_path);
            touch_time_stamp_timeout = std::chrono::steady_clock::now() +
                                       std::chrono::seconds(beerocks::TOUCH_PID_TIMEOUT_SECONDS);
        }

        // Run application event loop and break on error.
        if (event_loop->run() < 0) {
            LOG(ERROR) << "Event loop failure!";
            break;
        }
    }

    s_pLogger = nullptr;

    controller.stop();

    return 0;
}
