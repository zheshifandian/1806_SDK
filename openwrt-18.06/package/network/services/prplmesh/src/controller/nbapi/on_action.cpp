/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "on_action.h"

#include <beerocks/tlvf/beerocks_message_bml.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <locale.h>
#include <time.h>

using namespace beerocks;
using namespace net;
using namespace son;
namespace prplmesh {
namespace controller {
namespace actions {

// Actions

son::db *g_database = nullptr;

/*
** Set the number of seconds since this Associated Device was last attempted to be steered.
*/
static amxd_status_t action_last_steer_time(amxd_object_t *object, amxd_param_t *param,
                                            amxd_action_t reason, const amxc_var_t *const args,
                                            amxc_var_t *const retval, void *priv)
{
    if (reason != action_param_read) {
        return amxd_status_function_not_implemented;
    }
    if (!param) {
        LOG(WARNING) << "Missing value of amxd_param_t in action_last_steer_time.";
        return amxd_status_parameter_not_found;
    }

    auto status = amxd_action_param_read(object, param, reason, args, retval, priv);
    if (status != amxd_status_ok) {
        return status;
    }

    // Get param value directly without going though the object action handlers
    auto last_steer_ts = amxd_object_get_param_def(object, "LastSteerTimeStamp");
    if (!last_steer_ts) {
        LOG(WARNING) << "Missing value of LastSteerTimeStamp in action_last_steer_time.";
        return amxd_status_parameter_not_found;
    }

    std::string last_steer_ts_str = amxc_var_constcast(cstring_t, &last_steer_ts->value);
    if (last_steer_ts_str.empty()) {
        amxc_var_set(uint64_t, retval, 0);
        return amxd_status_ok;
    }

    amxc_ts_t ts_steer, ts_now;
    amxc_ts_parse(&ts_steer, last_steer_ts_str.c_str(), last_steer_ts_str.length());
    amxc_ts_now(&ts_now);

    amxc_var_set(uint64_t, retval, (ts_now.sec - ts_steer.sec));
    return amxd_status_ok;
}

/*
** Set (LastConnectTime) the number of seconds since station is associated. It is created from assoc. time of station object
*/
static amxd_status_t action_read_assoc_time(amxd_object_t *object, amxd_param_t *param,
                                            amxd_action_t reason, const amxc_var_t *const args,
                                            amxc_var_t *const retval, void *priv)
{

    if (reason != action_param_read) {
        return amxd_status_function_not_implemented;
    }
    if (!param) {
        return amxd_status_parameter_not_found;
    }

    auto status = amxd_action_param_read(object, param, reason, args, retval, priv);
    if (status != amxd_status_ok) {
        return status;
    }

    // Initialization of ambiorix triggers read action and it leads un-initalized 1db object
    if (!g_database) {
        return amxd_status_unknown_error;
    }

    amxd_param_t *mac_param = amxd_object_get_param_def(object, "MACAddress");
    if (mac_param == nullptr) {
        LOG(ERROR) << "MACAddress can not be read in STA datamodel";
        return amxd_status_parameter_not_found;
    }
    auto sta_mac = tlvf::mac_from_string(amxc_var_constcast(cstring_t, &mac_param->value));

    // Initialization of ambiorix objects triggers read action and it leads un-initalized values
    if (sta_mac == beerocks::net::network_utils::ZERO_MAC) {
        return amxd_status_object_not_found;
    }

    auto station = g_database->get_station(sta_mac);
    if (!station) {
        LOG(ERROR) << "Station is not found on db with mac: " << sta_mac;
        return amxd_status_object_not_found;
    }

    if (station->assoc_timestamp.empty()) {
        amxc_var_set(uint64_t, retval, 0);
        return amxd_status_ok;
    }

    amxc_ts_t ts_assoc, ts_now;
    amxc_ts_parse(&ts_assoc, station->assoc_timestamp.c_str(), station->assoc_timestamp.length());
    amxc_ts_now(&ts_now);

    amxc_var_set(uint64_t, retval, (ts_now.sec - ts_assoc.sec));
    return amxd_status_ok;
}

static amxd_status_t action_read_last_change(amxd_object_t *object, amxd_param_t *param,
                                             amxd_action_t reason, const amxc_var_t *const args,
                                             amxc_var_t *const retval, void *priv)
{
    /*
        This action retrieves CreationTime of BSS instance from BSS.LastChange parameter,
        since BSS.Enabled changes just once when we create BSS instance BSS.LastChange is constant.
        Than we retrieve CurrentTime and subtract CreationTime for getting time passed
        from creation in seconds.
    */
    if (reason != action_param_read) {
        return amxd_status_function_not_implemented;
    }
    if (!param) {
        return amxd_status_parameter_not_found;
    }

    auto status = amxd_action_param_read(object, param, reason, args, retval, priv);
    if (status != amxd_status_ok) {
        return status;
    }
    auto creation_time = amxc_var_dyncast(uint64_t, retval);

    auto current_time = std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::steady_clock::now().time_since_epoch())
                            .count();

    uint64_t last_change = current_time - creation_time;

    amxc_var_set(uint64_t, retval, last_change);

    return amxd_status_ok;
}

static std::string get_param_string(amxd_object_t *object, const char *param_name)
{
    amxc_var_t param;
    std::string param_val;

    amxc_var_init(&param);
    if (amxd_object_get_param(object, param_name, &param) == amxd_status_ok) {
        auto param_val_cstring = amxc_var_dyncast(cstring_t, &param);
        if (param_val_cstring) {
            param_val.assign(param_val_cstring);
        }
    }
    amxc_var_clean(&param);
    return param_val;
}

static bool get_param_bool(amxd_object_t *object, const char *param_name)
{
    amxc_var_t param;
    bool param_val = false;

    amxc_var_init(&param);
    if (amxd_object_get_param(object, param_name, &param) == amxd_status_ok) {
        param_val = amxc_var_constcast(bool, &param);
    } else {
        LOG(ERROR) << "Fail to get param: " << param_name;
    }
    amxc_var_clean(&param);
    return param_val;
}

/**
* @brief Overwrite an action 'get' aka 'read' for Device.WiFi.DataElements.Network.AccessPointCommit
* data element, that when this element is triggered the bss information from
* Device.WiFi.DataElements.Network.AccessPoint and Device.WiFi.DataElements.Network.AccessPoint.n.Security,
* where n = element's index, objects will be stored in the sAccessPoint structure.
*/
amxd_status_t access_point_commit(amxd_object_t *object, amxd_function_t *func, amxc_var_t *args,
                                  amxc_var_t *ret)
{
    amxc_var_clean(ret);
    amxd_object_t *access_point = amxd_object_get_child(object, "AccessPoint");

    if (!access_point) {
        LOG(WARNING) << "Fail to get AccessPoint object from data model";
        return amxd_status_ok;
    }

    g_database->clear_bss_info_configuration();
    amxd_object_for_each(instance, it, access_point)
    {
        son::wireless_utils::sBssInfoConf bss_info;
        amxd_object_t *access_point_inst = amxc_llist_it_get_data(it, amxd_object_t, it);
        amxd_object_t *security_inst     = amxd_object_get_child(access_point_inst, "Security");

        bss_info.ssid      = get_param_string(access_point_inst, "SSID");
        auto multi_ap_mode = get_param_string(access_point_inst, "MultiApMode");
        bss_info.backhaul  = (multi_ap_mode.find("Backhaul") != std::string::npos);
        bss_info.fronthaul = (multi_ap_mode.find("Fronthaul") != std::string::npos);

        if (!bss_info.backhaul && !bss_info.fronthaul) {
            LOG(DEBUG) << "MultiApMode for AccessPoint: " << bss_info.ssid << " is not set.";
            continue;
        }
        if (get_param_bool(access_point_inst, "Band2_4G")) {
            bss_info.operating_class.splice(bss_info.operating_class.end(),
                                            son::wireless_utils::string_to_wsc_oper_class("24g"));
        }
        if (get_param_bool(access_point_inst, "Band5GH")) {
            bss_info.operating_class.splice(bss_info.operating_class.end(),
                                            son::wireless_utils::string_to_wsc_oper_class("5gh"));
        }
        if (get_param_bool(access_point_inst, "Band5GL")) {
            bss_info.operating_class.splice(bss_info.operating_class.end(),
                                            son::wireless_utils::string_to_wsc_oper_class("5gl"));
        }
        if (get_param_bool(access_point_inst, "Band6G")) {
            bss_info.operating_class.splice(bss_info.operating_class.end(),
                                            son::wireless_utils::string_to_wsc_oper_class("6g"));
        }
        if (bss_info.operating_class.empty()) {
            LOG(DEBUG) << "Band for Access Point: " << bss_info.ssid << " is not set.";
            continue;
        }

        std::string mode_enabled = get_param_string(security_inst, "ModeEnabled");
        if (mode_enabled == "WPA3-Personal") {
            bss_info.network_key         = get_param_string(security_inst, "SAEPassphrase");
            bss_info.authentication_type = WSC::eWscAuth::WSC_AUTH_SAE;
            bss_info.encryption_type     = WSC::eWscEncr::WSC_ENCR_AES;
        } else if (mode_enabled == "WPA2-Personal") {
            bss_info.network_key = get_param_string(security_inst, "PreSharedKey");
            if (bss_info.network_key.empty()) {
                bss_info.network_key = get_param_string(security_inst, "KeyPassphrase");
            }
            bss_info.authentication_type = WSC::eWscAuth::WSC_AUTH_WPA2PSK;
            bss_info.encryption_type     = WSC::eWscEncr::WSC_ENCR_AES;
        } else {
            bss_info.authentication_type = WSC::eWscAuth::WSC_AUTH_OPEN;
            bss_info.encryption_type     = WSC::eWscEncr::WSC_ENCR_NONE;
        }

        if (bss_info.authentication_type != WSC::eWscAuth::WSC_AUTH_OPEN &&
            bss_info.network_key.empty()) {
            LOG(WARNING) << "BSS: " << bss_info.ssid << " with mode: " << mode_enabled
                         << " missing value for network key.";
            continue;
        }
        LOG(DEBUG) << "Add bss info configration for AP with ssid: " << bss_info.ssid
                   << " and operating classes: " << bss_info.operating_class;
        g_database->add_bss_info_configuration(bss_info);
    }

    // Update wifi credentials
    uint8_t m_tx_buffer[beerocks::message::MESSAGE_BUFFER_LENGTH];
    ieee1905_1::CmduMessageTx cmdu_tx(m_tx_buffer, sizeof(m_tx_buffer));
    auto connected_agents = g_database->get_all_connected_agents();

    if (!connected_agents.empty()) {
        if (!son_actions::send_ap_config_renew_msg(cmdu_tx, *g_database)) {
            LOG(ERROR) << "Failed son_actions::send_ap_config_renew_msg ! ";
        }
    }
    return amxd_status_ok;
}

amxd_status_t client_steering(amxd_object_t *object, amxd_function_t *func, amxc_var_t *args,
                              amxc_var_t *ret)
{
    auto controller_ctx = g_database->get_controller_ctx();

    auto sta_mac      = GET_CHAR(args, "station_mac");
    auto target_bssid = GET_CHAR(args, "target_bssid");

    if (!sta_mac || !target_bssid) {
        LOG(ERROR) << "Failed to get proper arguments.";
        return amxd_status_parameter_not_found;
    }

    if (!controller_ctx) {
        LOG(ERROR) << "Failed to get controller context.";
        return amxd_status_unknown_error;
    }

    if (!g_database->can_start_client_steering(sta_mac, target_bssid)) {
        LOG(WARNING) << "Failed to initiate steering on the client: " << sta_mac
                     << " with attempt connecting to an AP with BSSID: " << target_bssid;
    } else {
        controller_ctx->start_client_steering(sta_mac, target_bssid);
    }
    return amxd_status_ok;
}

/**
 * @brief Initiate channel scan from NBAPI for given radio and channels.
 *
 * Example of usage:
 * ubus call Device.WiFi.DataElements.Network ScanTrigger
 * '{"channels_list": "36, 44", channels_num: "2"}'
 *
 * When channel list does not contain any channels
 * scan triggering for all supported channels of specified radio.
 */
amxd_status_t trigger_scan(amxd_object_t *object, amxd_function_t *func, amxc_var_t *args,
                           amxc_var_t *ret)
{
    auto controller_ctx = g_database->get_controller_ctx();

    if (!controller_ctx) {
        LOG(ERROR) << "Failed to get controller context.";
        return amxd_status_unknown_error;
    }

    amxc_var_t value;

    amxc_var_init(&value);
    amxd_object_get_param(object, "ID", &value);
    std::string radio_mac = amxc_var_constcast(cstring_t, &value);

    if (radio_mac.empty()) {
        LOG(ERROR) << "radio_mac is empty";
        return amxd_status_parameter_not_found;
    }

    std::string channels_list = GET_CHAR(args, "channels_list");
    int pool_size             = amxc_var_dyncast(uint32_t, GET_ARG(args, "channels_num"));
    std::array<uint8_t, beerocks::message::SUPPORTED_CHANNELS_LENGTH> channel_pool;
    std::vector<std::string> channels_vec = beerocks::string_utils::str_split(channels_list, ',');
    int i                                 = 0;

    for (auto channel_str : channels_vec) {
        if (pool_size == 0) {
            break;
        }
        int channel_num = atoi(channel_str.c_str());
        if (channel_num < std::numeric_limits<uint8_t>::min() ||
            std::numeric_limits<uint8_t>::max() < channel_num) {
            LOG(ERROR) << "Channel #" << channel_num << " is out of range.";
            return amxd_status_unknown_error;
        }
        channel_pool[i] = channel_num;
        i++;
    }

    if (i != pool_size) {
        LOG(ERROR) << "Wrong number of channels: " << pool_size
                   << " or data entered in wrong format: " << channels_list;
        return amxd_status_unknown_error;
    }

    if (!controller_ctx->trigger_scan(tlvf::mac_from_string(radio_mac), channel_pool, pool_size,
                                      PREFERRED_DWELLTIME_MS)) {
        LOG(ERROR) << "Failed to trigger scan from NBAPI for radio: " << radio_mac
                   << " with channels: " << channels_list;
        return amxd_status_unknown_error;
    }
    return amxd_status_ok;
}

// Events

amxd_dm_t *g_data_model = nullptr;

static void rm_params(amxd_object_t *object, const char *param_name)
{
    amxd_param_t *param = amxd_object_get_param_def(object, param_name);

    if (param) {
        amxd_action_param_destroy(object, param, action_param_destroy, NULL, NULL, NULL);
        amxd_param_delete(&param);
    }
}

static void add_string_param(const char *param_name, amxd_object_t *param_owner_obj)
{
    amxd_param_t *param = NULL;

    amxd_param_new(&param, param_name, AMXC_VAR_ID_CSTRING);
    amxd_object_add_param(param_owner_obj, param);
}

/**
 * @brief Removes PreSharedKey, KeyPassphrase, SAEPassphrase parameters
 * from Device.WiFi.DataElements.Network.AccessPoint.*.Security object.
 * event_rm_params() invokes when value of parameter
 * Device.WiFi.DataElements.Network.AccessPoint.*.Security.ModeEnabled changed
 * from "WPA2-Personal" to any of other available values.
 */
static void event_rm_params(const char *const sig_name, const amxc_var_t *const data,
                            void *const priv)
{
    amxd_object_t *security_obj = amxd_dm_signal_get_object(g_data_model, data);

    if (!security_obj) {
        LOG(WARNING)
            << "Failed to get object Device.WiFi.DataElements.Network.AccessPoint.*.Security";
        return;
    }
    rm_params(security_obj, "PreSharedKey");
    rm_params(security_obj, "KeyPassphrase");
    rm_params(security_obj, "SAEPassphrase");
}

/**
 * @brief Add PreSharedKey, KeyPassphrase, SAEPassphrase parameters
 * to Device.WiFi.DataElements.Network.AccessPoint.*.Security object.
 * Function invokes when value of parameter
 * Device.WiFi.DataElements.Network.AccessPoint.*.Security.ModeEnabled changed to "WPA2-Personal".
 */
static void event_add_hidden_params(const char *const sig_name, const amxc_var_t *const data,
                                    void *const priv)
{
    amxd_object_t *security_obj = amxd_dm_signal_get_object(g_data_model, data);

    if (!security_obj) {
        LOG(WARNING)
            << "Failed to get object Device.WiFi.DataElements.Network.AccessPoint.*.Security";
        return;
    }
    add_string_param("PreSharedKey", security_obj);
    add_string_param("KeyPassphrase", security_obj);
    add_string_param("SAEPassphrase", security_obj);
}

/**
 * @brief Event handler for controller configuration change.
 *
 * event_configuration_changed is invoked when value of parameter
 * in Device.WiFi.DataElements.Configuration object changes with set command.
 */
static void event_configuration_changed(const char *const sig_name, const amxc_var_t *const data,
                                        void *const priv)
{
    amxd_object_t *configuration = amxd_dm_signal_get_object(g_data_model, data);

    if (!configuration) {
        LOG(WARNING) << "Failed to get object Device.WiFi.DataElements.Configuration";
        return;
    }

    son::db::sDbNbapiConfig nbapi_config;
    nbapi_config.client_band_steering =
        amxd_object_get_bool(configuration, "BandSteeringEnabled", nullptr);
    nbapi_config.client_optimal_path_roaming =
        amxd_object_get_bool(configuration, "ClientSteeringEnabled", nullptr);
    nbapi_config.roaming_hysteresis_percent_bonus =
        amxd_object_get_int32_t(configuration, "SteeringCurrentBonus", nullptr);
    nbapi_config.steering_disassoc_timer_msec = std::chrono::milliseconds{
        amxd_object_get_int32_t(configuration, "SteeringDisassociationTimer", nullptr)};
    nbapi_config.link_metrics_request_interval_seconds = std::chrono::seconds{
        amxd_object_get_int32_t(configuration, "LinkMetricsRequestInterval", nullptr)};

    if (!g_database->update_master_configuration(nbapi_config)) {
        LOG(ERROR) << "Failed update master configuration from NBAPI.";
    }

    if (!g_database->dm_update_collection_intervals(
            nbapi_config.link_metrics_request_interval_seconds)) {
        LOG(ERROR) << "Failed update collection intervals of all agents.";
    }

    // TODO Save persistent settings with amxo_parser_save() (PPM-1419)
}

std::vector<beerocks::nbapi::sActionsCallback> get_actions_callback_list(void)
{
    const std::vector<beerocks::nbapi::sActionsCallback> actions_list = {
        {"action_read_assoc_time", action_read_assoc_time},
        {"action_read_last_change", action_read_last_change},
        {"action_last_steer_time", action_last_steer_time},
    };
    return actions_list;
}

std::vector<beerocks::nbapi::sEvents> get_events_list(void)
{
    const std::vector<beerocks::nbapi::sEvents> events_list = {
        {"event_rm_params", event_rm_params},
        {"event_add_hidden_params", event_add_hidden_params},
        {"event_configuration_changed", event_configuration_changed},
    };
    return events_list;
}

std::vector<beerocks::nbapi::sFunctions> get_func_list(void)
{
    const std::vector<beerocks::nbapi::sFunctions> functions_list = {
        {"access_point_commit", "Device.WiFi.DataElements.Network.AccessPointCommit",
         access_point_commit},
        {"client_steering", "Device.WiFi.DataElements.Network.ClientSteering", client_steering},
        {"trigger_scan", "Device.WiFi.DataElements.Network.Device.Radio.ScanTrigger",
         trigger_scan}};
    return functions_list;
}

beerocks::nbapi::ambiorix_func_ptr get_access_point_commit(void) { return access_point_commit; }

} // namespace actions
} // namespace controller
} // namespace prplmesh
