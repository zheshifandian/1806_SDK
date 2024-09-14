/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bpl/bpl_cfg.h>

#include "../../common/uci/bpl_uci.h"
#include "../../common/utils/utils.h"
#include "../../common/utils/utils_net.h"

#include "bpl_cfg_helper.h"
#include "bpl_cfg_uci.h"

#include <bcl/beerocks_string_utils.h>
#include <mapf/common/logger.h>
#include <mapf/common/utils.h>

using namespace mapf;

namespace beerocks {
namespace bpl {

int cfg_get_hostap_iface_steer_vaps(int32_t radio_num,
                                    char hostap_iface_steer_vaps[BPL_LOAD_STEER_ON_VAPS_LEN])
{
    if (!hostap_iface_steer_vaps) {
        MAPF_ERR("invalid input: hostap_iface_steer_vaps is NULL");
        return RETURN_ERR;
    }

    if (radio_num < 0) {
        MAPF_ERR("invalid input: radio_num < 0");
        return RETURN_ERR;
    }

    return cfg_get_prplmesh_radio_param(radio_num, "hostap_iface_steer_vaps",
                                        hostap_iface_steer_vaps, BPL_LOAD_STEER_ON_VAPS_LEN);
}

int cfg_is_enabled()
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int("enable", &retVal) == RETURN_ERR) {
        MAPF_ERR("cfg_is_enabled: Failed to read Enable parameter\n");
        return RETURN_ERR;
    }
    return retVal;
}

int cfg_is_master()
{
    switch (cfg_get_management_mode()) {
    case BPL_MGMT_MODE_MULTIAP_CONTROLLER_AGENT:
        return 1;
    case BPL_MGMT_MODE_MULTIAP_CONTROLLER:
        return 1;
    case BPL_MGMT_MODE_MULTIAP_AGENT:
        return 0;
    case BPL_MGMT_MODE_NOT_MULTIAP:
        return (cfg_get_operating_mode() == BPL_OPER_MODE_GATEWAY) ? 1 : 0;
    default:
        return -1;
    }
}

int cfg_get_management_mode()
{
    int retVal                                = 0;
    char mgmt_mode[BPL_GW_DB_MANAGE_MODE_LEN] = {0};
    if (cfg_get_prplmesh_param("management_mode", mgmt_mode, BPL_GW_DB_MANAGE_MODE_LEN) < 0) {
        MAPF_ERR("cfg_get_management_mode: Failed to read management_mode");
        retVal = -1;
    } else {
        std::string mode_str(mgmt_mode);
        if (mode_str == "Multi-AP-Controller-and-Agent") {
            retVal = BPL_MGMT_MODE_MULTIAP_CONTROLLER_AGENT;
        } else if (mode_str == "Multi-AP-Controller") {
            retVal = BPL_MGMT_MODE_MULTIAP_CONTROLLER;
        } else if (mode_str == "Multi-AP-Agent") {
            retVal = BPL_MGMT_MODE_MULTIAP_AGENT;
        } else if (mode_str == "Not-Multi-AP") {
            retVal = BPL_MGMT_MODE_NOT_MULTIAP;
        } else {
            MAPF_ERR("cfg_get_management_mode: Unexpected management_mode");
            retVal = -1;
        }
    }
    return retVal;
}

int cfg_get_operating_mode()
{
    int retVal                            = 0;
    char op_mode[BPL_GW_DB_OPER_MODE_LEN] = {0};
    if (cfg_get_prplmesh_param("operating_mode", op_mode, BPL_GW_DB_OPER_MODE_LEN) < 0) {
        MAPF_ERR("cfg_get_operating_mode: Failed to read OperatingMode\n");
        retVal = -1;
    } else {
        std::string mode_str(op_mode);
        if (mode_str == "Gateway") {
            retVal = BPL_OPER_MODE_GATEWAY;
        } else if (mode_str == "Gateway-WISP") {
            retVal = BPL_OPER_MODE_GATEWAY_WISP;
        } else if (mode_str == "WDS-Extender") {
            retVal = BPL_OPER_MODE_WDS_EXTENDER;
        } else if (mode_str == "WDS-Repeater") {
            retVal = BPL_OPER_MODE_WDS_REPEATER;
        } else if (mode_str == "L2NAT-Client") {
            retVal = BPL_OPER_MODE_L2NAT_CLIENT;
        } else {
            MAPF_ERR("cfg_get_operating_mode: Unexpected OperatingMode\n");
            retVal = -1;
        }
    }
    return retVal;
}

int cfg_get_certification_mode()
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int("certification_mode", &retVal) == RETURN_ERR) {
        MAPF_ERR("cfg_get_certification_mode: Failed to read certification_mode parameter\n");
        return RETURN_ERR;
    }
    return retVal;
}

int cfg_get_load_steer_on_vaps(int num_of_interfaces,
                               char load_steer_on_vaps[BPL_LOAD_STEER_ON_VAPS_LEN])
{
    if (num_of_interfaces < 1) {
        MAPF_ERR("invalid input: max num_of_interfaces value < 1");
        return RETURN_ERR;
    }

    if (!load_steer_on_vaps) {
        MAPF_ERR("invalid input: load_steer_on_vaps is NULL");
        return RETURN_ERR;
    }

    std::string load_steer_on_vaps_str;
    char hostap_iface_steer_vaps[BPL_LOAD_STEER_ON_VAPS_LEN] = {0};
    for (int index = 0; index < num_of_interfaces; index++) {
        if (cfg_get_hostap_iface_steer_vaps(index, hostap_iface_steer_vaps) == RETURN_OK) {
            if (std::string(hostap_iface_steer_vaps).length() > 0) {
                if (!load_steer_on_vaps_str.empty()) {
                    load_steer_on_vaps_str.append(",");
                }
                load_steer_on_vaps_str.append(std::string(hostap_iface_steer_vaps));
                MAPF_DBG("adding interface " << hostap_iface_steer_vaps
                                             << " to the steer on vaps list");
            }
        }
    }

    if (load_steer_on_vaps_str.empty()) {
        MAPF_DBG("steer on vaps list is not configured");
        return RETURN_OK;
    }

    mapf::utils::copy_string(load_steer_on_vaps, load_steer_on_vaps_str.c_str(),
                             BPL_LOAD_STEER_ON_VAPS_LEN);

    return RETURN_OK;
}

int cfg_get_stop_on_failure_attempts()
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int_default("stop_on_failure_attempts", &retVal,
                                           DEFAULT_STOP_ON_FAILURE_ATTEMPTS) == RETURN_ERR) {

        MAPF_INFO("cfg_get_stop_on_failure_attempts: Failed to read stop_on_failure_attempts "
                  "parameter\n");
        return RETURN_ERR;
    }
    return retVal;
}

int cfg_is_onboarding()
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int("onboarding", &retVal) == RETURN_ERR) {
        MAPF_ERR("cfg_is_onboarding: Failed to read Onboarding parameter\n");
        return RETURN_ERR;
    }
    return retVal;
}

int cfg_get_rdkb_extensions()
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int_default("rdkb_extensions", &retVal, DEFAULT_RDKB_EXTENSIONS) ==
        RETURN_ERR) {
        MAPF_INFO("cfg_get_rdkb_extensions: Failed to read RDKB Extensions parameter\n");
        return RETURN_ERR;
    }
    return retVal;
}

bool cfg_get_band_steering(bool &band_steering)
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int("band_steering", &retVal) == RETURN_ERR) {
        return false;
    }

    band_steering = (retVal == 1);
    return true;
}

bool cfg_set_band_steering(bool band_steering)
{
    std::string option = "band_steering";
    std::string value  = std::to_string(((int)band_steering));

    return cfg_set_prplmesh_config(option, value);
}

bool cfg_get_client_roaming(bool &client_roaming)
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int("client_roaming", &retVal) == RETURN_ERR) {
        return false;
    }

    client_roaming = (retVal == 1);
    return true;
}

bool cfg_set_client_roaming(bool client_roaming)
{
    std::string option = "client_roaming";
    std::string value  = std::to_string(((int)client_roaming));

    return cfg_set_prplmesh_config(option, value);
}

int cfg_get_wifi_params(const char iface[BPL_IFNAME_LEN], struct BPL_WLAN_PARAMS *wlan_params)
{
    if (!iface || !wlan_params) {
        MAPF_ERR("cfg_get_wifi_params: invalid input: iface = "
                 << intptr_t(iface) << " wlan_params = " << intptr_t(wlan_params));
        return RETURN_ERR;
    }

    // The UCI "disabled" setting is optional, defaults to false if not present
    bool disabled = false;
    cfg_uci_get_wireless_bool(TYPE_RADIO, iface, "disabled", &disabled);
    wlan_params->enabled = !disabled;

    if (cfg_uci_get_wireless_bool(TYPE_RADIO, iface, "sub_band_dfs", &wlan_params->sub_band_dfs) ==
        RETURN_ERR) {
        // Failed to find "sub_band_dfs", set to to default value.
        wlan_params->sub_band_dfs = false;
    }

    // The UCI "channel" setting is not documented as optional, but for Intel
    // wireless (as probably for other drivers) it is. We do not want to
    // fail when wifi still works fine, so default to "auto" (0) and if
    // can't get the channel from UCI just move on.
    wlan_params->channel = 0;
    cfg_get_channel(iface, &wlan_params->channel);

    // country code
    char alpha_2[MAX_UCI_BUF_LEN] = {0};
    cfg_uci_get_wireless_from_ifname(TYPE_RADIO, iface, "country", alpha_2);

    wlan_params->country_code[0] = alpha_2[0];
    wlan_params->country_code[1] = alpha_2[1];

    return RETURN_OK;
}

int cfg_get_backhaul_params(int *max_vaps, int *network_enabled, int *preferred_radio_band)
{
    if (max_vaps) {
        //get max_vaps
    }

    if (network_enabled) {
        //get network_enabled
    }

    if (preferred_radio_band) {
        char backhaul_band[BPL_BACKHAUL_BAND_LEN] = {0};
        //get preferred_radio_band
        int retVal = cfg_get_prplmesh_param("backhaul_band", backhaul_band, BPL_BACKHAUL_BAND_LEN);
        if (retVal == RETURN_ERR) {
            MAPF_ERR("cfg_get_backhaul_params: Failed to read backhaul_band parameter\n");
            return RETURN_ERR;
        }
        std::string preferred_bh_band(backhaul_band);
        if (preferred_bh_band.compare("2.4GHz") == 0) {
            *preferred_radio_band = BPL_RADIO_BAND_2G;
        } else if (preferred_bh_band.compare("5GHz") == 0) {
            *preferred_radio_band = BPL_RADIO_BAND_5G;
        } else if (preferred_bh_band.compare("auto") == 0) {
            *preferred_radio_band = BPL_RADIO_BAND_AUTO;
        } else {
            MAPF_ERR("cfg_get_backhaul_params: unknown backhaul_band parameter value\n");
            return RETURN_ERR;
        }
    }

    return RETURN_OK;
}

int cfg_get_backhaul_vaps(char *backhaul_vaps_buf, const int buf_len) { return 0; }

int cfg_get_beerocks_credentials(const int radio_dir, char ssid[BPL_SSID_LEN],
                                 char pass[BPL_PASS_LEN], char sec[BPL_SEC_LEN])
{
    int retVal = 0;

    retVal |= cfg_get_prplmesh_param("ssid", ssid, BPL_SSID_LEN);
    retVal |= cfg_get_prplmesh_param("mode_enabled", sec, BPL_SEC_LEN);
    if (!strcmp(sec, "WEP-64") || !strcmp(sec, "WEP-128")) {
        retVal |= cfg_get_prplmesh_param("wep_key", pass, BPL_PASS_LEN);
    } else {
        retVal |= cfg_get_prplmesh_param("key_passphrase", pass, BPL_PASS_LEN);
    }

    return retVal;
}

int cfg_get_security_policy()
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int("mem_only_psk", &retVal) == RETURN_ERR) {
        MAPF_ERR("cfg_get_security_policy: Failed to read mem_only_psk parameter\n");
        return RETURN_ERR;
    }
    return retVal;
}

int cfg_notify_onboarding_completed(const char ssid[BPL_SSID_LEN], const char pass[BPL_PASS_LEN],
                                    const char sec[BPL_SEC_LEN],
                                    const char iface_name[BPL_IFNAME_LEN], const int success)
{
    //return (sl_beerocks_notify_onboarding_completed(ssid, pass, sec, iface_name, success) ? 0 : -1);
    return 0;
}

int cfg_notify_error(int code, const char data[BPL_ERROR_STRING_LEN]) { return 0; }

int cfg_get_administrator_credentials(char pass[BPL_PASS_LEN]) { return 0; }

int cfg_get_sta_iface(const char iface[BPL_IFNAME_LEN], char sta_iface[BPL_IFNAME_LEN])
{
    if (!iface || !sta_iface) {
        MAPF_ERR("cfg_get_sta_iface: invalid input: iface or sta_iface are NULL");
        return RETURN_ERR;
    }

    // Find the "prplmesh.wifi-device" section in UCI configuration for the given interface
    const std::string package_name = "prplmesh";
    const std::string section_type = "wifi-device";
    std::string section_name;
    if (!uci_find_section_by_option(package_name, section_type, "hostap_iface", iface,
                                    section_name)) {
        LOG(ERROR) << "Failed to find configuration section for interface " << iface;
        return RETURN_ERR;
    }

    if (section_name.empty()) {
        LOG(ERROR) << "Configuration for interface " << iface << " not found";
        return RETURN_ERR;
    }

    // Get the name of the STA interface for the given Host AP interface
    std::string option_value;
    if (!uci_get_option(package_name, section_type, section_name, "sta_iface", option_value)) {
        LOG(ERROR) << "Failed to get STA interface for Host AP interface " << iface;
        return RETURN_ERR;
    }

    mapf::utils::copy_string(sta_iface, option_value.c_str(), BPL_IFNAME_LEN);

    return RETURN_OK;
}

int cfg_get_hostap_iface(int32_t radio_num, char hostap_iface[BPL_IFNAME_LEN])
{
    if (!hostap_iface) {
        MAPF_ERR("cfg_get_hostap_iface: invalid input: hostap_iface is NULL");
        return RETURN_ERR;
    }

    if (radio_num < 0) {
        MAPF_ERR("cfg_get_hostap_iface: invalid input: radio_num < 0");
        return RETURN_ERR;
    }

    return cfg_get_prplmesh_radio_param(radio_num, "hostap_iface", hostap_iface, BPL_IFNAME_LEN);
}

int cfg_get_all_prplmesh_wifi_interfaces(BPL_WLAN_IFACE *interfaces, int *num_of_interfaces)
{
    if (!interfaces) {
        MAPF_ERR("cfg_get_all_prplmesh_wifi_interfaces: invalid input: interfaces is NULL");
        return RETURN_ERR;
    }
    if (!num_of_interfaces) {
        MAPF_ERR("cfg_get_all_prplmesh_wifi_interfaces: invalid input: num_of_interfaces is NULL");
        return RETURN_ERR;
    }
    if (*num_of_interfaces < 1) {
        MAPF_ERR(
            "cfg_get_all_prplmesh_wifi_interfaces: invalid input: max num_of_interfaces value < 1");
        return RETURN_ERR;
    }

    std::unordered_map<std::string, std::string> hostapd_ifaces;
    if (cfg_get_prplmesh_hostapd_ifaces(hostapd_ifaces) == RETURN_ERR) {
        MAPF_DBG("cfg_get_all_prplmesh_wifi_interfaces: failed to get avaliable interfaces");
        return RETURN_ERR;
    }

    if (*num_of_interfaces < (int)hostapd_ifaces.size()) {
        MAPF_ERR("cfg_get_all_prplmesh_wifi_interfaces: invalid input: max num_of_interfaces < "
                 "number of avaliable interfaces");
        return RETURN_ERR;
    }

    int iface_idx = 0;
    for (const auto &iface_iter : hostapd_ifaces) {
        // the `first` element of iface_iter is a string structured "radioN" where N represents the
        // given interface's index
        interfaces[iface_idx].radio_num = atoi(iface_iter.first.substr(5).c_str());
        strncpy_s(interfaces[iface_idx].ifname, BPL_IFNAME_LEN, iface_iter.second.c_str(),
                  BPL_IFNAME_LEN - 1);
        iface_idx++;
    }

    *num_of_interfaces = iface_idx;

    return RETURN_OK;
}

bool cfg_get_zwdfs_enable(bool &enable)
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int_default("zwdfs_enable", &retVal, DEFAULT_ZWDFS_ENABLE) ==
        RETURN_ERR) {
        MAPF_ERR("Failed to read zwdfs_enable parameter");
        return false;
    }

    enable = (retVal == 1);

    return true;
}

bool cfg_get_best_channel_rank_threshold(uint32_t &threshold)
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int_default("best_channel_rank_th", &retVal,
                                           DEFAULT_BEST_CHANNEL_RANKING_TH) == RETURN_ERR) {
        MAPF_ERR("Failed to read best_channel_rank_th parameter");
        return false;
    }

    if (retVal < 0) {
        MAPF_ERR("best_channel_rank_th is configured to a negative value");
        return false;
    }

    threshold = retVal;

    return true;
}

bool cfg_get_persistent_db_enable(bool &enable)
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int_default("persistent_db", &retVal, DEFAULT_PERSISTENT_DB) ==
        RETURN_ERR) {
        MAPF_ERR("Failed to read persistent-db-enable parameter");
        return false;
    }

    enable = (retVal == 1);

    return true;
}

bool cfg_get_persistent_db_commit_changes_interval(unsigned int &interval_sec)
{
    int commit_changes_value = DEFAULT_COMMIT_CHANGES_INTERVAL_VALUE_SEC;

    if (cfg_get_prplmesh_param_int_default("commit_changes_interval", &commit_changes_value,
                                           DEFAULT_COMMIT_CHANGES_INTERVAL_VALUE_SEC) ==
        RETURN_ERR) {
        MAPF_ERR("Failed to read commit_changes_interval parameter");
        return false;
    }

    interval_sec = commit_changes_value;

    return true;
}

bool cfg_get_clients_persistent_db_max_size(int &max_size)
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int_default("clients_persistent_db_max_size", &retVal,
                                           DEFAULT_CLIENTS_PERSISTENT_DB_MAX_SIZE) == RETURN_ERR) {
        MAPF_ERR("Failed to read clients-persistent-db-max-size parameter");
        return false;
    }

    max_size = retVal;

    return true;
}

bool cfg_get_steer_history_persistent_db_max_size(size_t &max_size)
{
    int retVal = -1;

    if (cfg_get_prplmesh_param_int_default("steer_history_db_max_size", &retVal,
                                           DEFAULT_STEER_HISTORY_PERSISTENT_DB_MAX_SIZE) ==
        RETURN_ERR) {
        MAPF_ERR("Failed to read steer-history-persistent-db-max-size parameter");
        return false;
    }
    max_size = retVal;
    return true;
}

bool cfg_get_max_timelife_delay_minutes(int &max_timelife_delay_minutes)
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int_default("max_timelife_delay_minutes", &retVal,
                                           DEFAULT_MAX_TIMELIFE_DELAY_MINUTES) == RETURN_ERR) {
        MAPF_ERR("Failed to read max-timelife-delay-minutes parameter");
        return false;
    }

    max_timelife_delay_minutes = retVal;

    return true;
}

bool cfg_get_unfriendly_device_max_timelife_delay_minutes(
    int &unfriendly_device_max_timelife_delay_minutes)
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int_default("unfriendly_device_max_timelife_delay_minutes", &retVal,
                                           DEFAULT_UNFRIENDLY_DEVICE_MAX_TIMELIFE_DELAY_MINUTES) ==
        RETURN_ERR) {
        MAPF_ERR("Failed to read unfriendly-device-max-timelife-delay-minutes parameter");
        return false;
    }

    unfriendly_device_max_timelife_delay_minutes = retVal;

    return true;
}

bool cfg_get_persistent_db_aging_interval(int &persistent_db_aging_interval_sec)
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int_default("persistent_db_aging_interval_sec", &retVal,
                                           DEFAULT_PERSISTENT_DB_AGING_INTERVAL_SEC) ==
        RETURN_ERR) {
        MAPF_ERR("Failed to read persistent-db-aging-interval-sec parameter - setting "
                 "default value");
        return false;
    }

    persistent_db_aging_interval_sec = retVal;

    return true;
}

bool bpl_cfg_get_wpa_supplicant_ctrl_path(const std::string &iface, std::string &wpa_ctrl_path)
{
    const char *path{"/var/run/wpa_supplicant/"};
    wpa_ctrl_path = path + iface;
    return true;
}

bool bpl_cfg_get_hostapd_ctrl_path(const std::string &iface, std::string &hostapd_ctrl_path)
{
    const char *path{"/var/run/hostapd/"};
    hostapd_ctrl_path = path + iface;
    return true;
}

bool cfg_get_link_metrics_request_interval(std::chrono::seconds &link_metrics_request_interval_sec)
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int_default(
            "link_metrics_request_interval_sec", &retVal,
            DEFAULT_LINK_METRICS_REQUEST_INTERVAL_VALUE_SEC.count()) == RETURN_ERR) {
        MAPF_INFO("Failed to read link_metrics_request_interval_sec parameter - setting "
                  "default value");
        return false;
    }

    link_metrics_request_interval_sec = std::chrono::seconds{retVal};
    return true;
}

bool cfg_set_link_metrics_request_interval(std::chrono::seconds &link_metrics_request_interval_sec)
{
    std::string option = "link_metrics_request_interval_sec";
    std::string value  = std::to_string(link_metrics_request_interval_sec.count());

    return cfg_set_prplmesh_config(option, value);
}

bool cfg_get_unsuccessful_assoc_report_policy(bool &unsuccessful_assoc_report_policy)
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int_default("unsuccessful_assoc_report_policy", &retVal,
                                           DEFAULT_UNSUCCESSFUL_ASSOC_REPORT_POLICY) ==
        RETURN_ERR) {
        MAPF_INFO("Failed to read unsuccessful_assoc_report_policy parameter - setting "
                  "default value");
        return false;
    }

    unsuccessful_assoc_report_policy = (retVal == 1);
    LOG(DEBUG) << "get unsuccessful_assoc_report_policy: " << unsuccessful_assoc_report_policy;
    return true;
}

bool cfg_set_unsuccessful_assoc_report_policy(bool &unsuccessful_assoc_report_policy)
{
    std::string option = "unsuccessful_assoc_report_policy";
    std::string value  = std::to_string((int)unsuccessful_assoc_report_policy);
    LOG(DEBUG) << "set unsuccessful_assoc_report_policy: " << value;
    return cfg_set_prplmesh_config(option, value);
}

bool cfg_get_unsuccessful_assoc_max_reporting_rate(int &unsuccessful_assoc_max_reporting_rate)
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int_default("unsuccessful_assoc_max_reporting_rate", &retVal,
                                           DEFAULT_UNSUCCESSFUL_ASSOC_MAX_REPORTING_RATE) ==
        RETURN_ERR) {
        MAPF_INFO("Failed to read unsuccessful_assoc_max_reporting_rate parameter - setting "
                  "default value");
        return false;
    }

    unsuccessful_assoc_max_reporting_rate = retVal;

    return true;
}

bool cfg_set_unsuccessful_assoc_max_reporting_rate(int &unsuccessful_assoc_max_reporting_rate)
{
    std::string option = "unsuccessful_assoc_max_reporting_rate";
    std::string value  = std::to_string(unsuccessful_assoc_max_reporting_rate);

    return cfg_set_prplmesh_config(option, value);
}

static bool bpl_cfg_get_bss_configuration(const std::string &section_name,
                                          son::wireless_utils::sBssInfoConf &configuration)
{
    OptionsUnorderedMap options;
    if (!uci_get_section("wireless", "wifi-iface", section_name, options)) {
        LOG(ERROR) << "Failed to get values for section " << section_name;
        return false;
    }

    // Fill in wireless credentials from option values read from UCI configuration.
    configuration.ssid = options["ssid"];

    auto starts_with = [](const std::string &prefix, const std::string &value) {
        return (value.compare(0, prefix.size(), prefix) == 0);
    };

    auto contains = [](const std::string &substring, const std::string &value) {
        return (value.find(substring) != std::string::npos);
    };

    auto get_authentication_type = [&](const std::string &encryption) {
        if ("none" == encryption || encryption.empty()) {
            return WSC::eWscAuth::WSC_AUTH_OPEN;
        } else if (starts_with("psk2", encryption)) {
            return WSC::eWscAuth::WSC_AUTH_WPA2PSK;
        } else if ("sae" == encryption) {
            return WSC::eWscAuth::WSC_AUTH_SAE;
        }
        return WSC::eWscAuth::WSC_AUTH_INVALID;
    };
    configuration.authentication_type = get_authentication_type(options["encryption"]);

    auto get_encryption_type = [&](const std::string &encryption) {
        if ("none" == encryption || encryption.empty()) {
            return WSC::eWscEncr::WSC_ENCR_NONE;
        } else if (contains("+tkip", encryption)) {
            return WSC::eWscEncr::WSC_ENCR_TKIP;
        } else if (("psk2" == encryption) || ("sae" == encryption) ||
                   contains("+aes", encryption) || contains("+ccmp", encryption)) {
            return WSC::eWscEncr::WSC_ENCR_AES;
        }
        return WSC::eWscEncr::WSC_ENCR_INVALID;
    };
    configuration.encryption_type = get_encryption_type(options["encryption"]);

    configuration.network_key = options["key"];

    return true;
}

bool bpl_cfg_get_wireless_settings(std::list<son::wireless_utils::sBssInfoConf> &wireless_settings)
{
    // Get all "wireless.wifi-iface" section names in UCI configuration
    const std::string package_name = "wireless";
    const std::string section_type = "wifi-iface";
    std::vector<std::string> sections;
    if (!uci_get_all_sections(package_name, section_type, sections)) {
        LOG(ERROR) << "Failed to get section names";
        return false;
    }

    // Read SSID and WiFi credentials from each "wireless.wifi-iface" section found.
    for (const auto &section_name : sections) {
        std::string mode;
        if (!uci_get_option(package_name, section_type, section_name, "mode", mode)) {
            LOG(DEBUG) << "Failed to get 'mode' from section " << section_name;
            continue;
        }

        // Silently ignore sections that do not configure an AP.
        if (mode != "ap") {
            continue;
        }

        std::string hidden;
        uci_get_option(package_name, section_type, section_name, "hidden", hidden);
        // the hidden option might not exist, in which case we treat
        // it as if it was 0 (i.e. we don't skip the section).

        std::string disabled;
        uci_get_option(package_name, section_type, section_name, "disabled", disabled);
        // the disabled option might not exist, in which case we treat
        // it as if it was 0 (i.e. we don't skip the section).

        if (hidden == "1" || disabled == "1") {
            LOG(INFO) << "Skipping section for hidden or disabled BSS: " << section_name;
            continue;
        }

        std::string ssid;
        uci_get_option(package_name, section_type, section_name, "ssid", ssid);
        // the SSID might not be set, in which case we treat
        // it as if it was an empty string (i.e. we *do* skip the section).
        if (ssid.empty()) {
            LOG(INFO) << "Skipping configuration for section with unset or empty SSID: "
                      << section_name;
            continue;
        }

        son::wireless_utils::sBssInfoConf configuration;
        if (!bpl_cfg_get_bss_configuration(section_name, configuration)) {
            LOG(DEBUG) << "Failed to get SSID and WiFi credentials from section " << section_name;
            continue;
        }

        if (configuration.authentication_type == WSC::eWscAuth::WSC_AUTH_INVALID ||
            configuration.encryption_type == WSC::eWscEncr::WSC_ENCR_INVALID) {
            LOG(INFO) << "Skipping configuration for section with invalid authentication or "
                         "encryption type: "
                      << section_name;
            continue;
        }

        // Operating classes are not specified in UCI configuration, but we can guess based on
        // the mode used by hostapd that was set for the radio.
        // To get the mode, first get the device and then the mode inside the section for that
        // device.
        std::string device;
        if (!uci_get_option(package_name, section_type, section_name, "device", device)) {
            LOG(DEBUG) << "Failed to get 'device' from section " << section_name;
            continue;
        }

        // Option "hwmode" in device section selects the wireless protocol to use, possible values
        // are 11b, 11g, and 11a.
        std::string hwmode;
        if (!uci_get_option(package_name, "wifi-device", device, "hwmode", hwmode)) {
            LOG(DEBUG) << "Failed to get 'hwmode' from section " << device;
            continue;
        }

        // The mode used by upstream hostapd (11b, 11g, 11n, 11ac, 11ax) is governed by several parameters in
        // the configuration file. However, as explained in the comment below from hostapd.conf, the
        // hw_mode parameter is sufficient to determine the band.
        //
        // # Operation mode (a = IEEE 802.11a (5 GHz), b = IEEE 802.11b (2.4 GHz),
        // # g = IEEE 802.11g (2.4 GHz), ad = IEEE 802.11ad (60 GHz); a/g options are used
        // # with IEEE 802.11n (HT), too, to specify band). For IEEE 802.11ac (VHT), this
        // # needs to be set to hw_mode=a. For IEEE 802.11ax (HE) on 6 GHz this needs
        // # to be set to hw_mode=a.
        //
        // Note that this will need to be revisited for 6GHz operation, which we don't support at
        // the moment.
        //
        // For MaxLinear's devices, by default '11bgnax' is used for 2.4Ghz bands, and '11anacax' is
        // used for 5Ghz bands (see 'files/scripts/lib/netifd/wireless/mac80211.sh' in the swpal package).
        if (hwmode.empty() || (hwmode == "11b") || (hwmode == "11g") || hwmode == "11bgnax") {
            configuration.operating_class.splice(
                configuration.operating_class.end(),
                son::wireless_utils::string_to_wsc_oper_class("24g"));
        } else if (hwmode == "11a" || hwmode == "11anacax") {
            configuration.operating_class.splice(
                configuration.operating_class.end(),
                son::wireless_utils::string_to_wsc_oper_class("5g"));
        } else {
            LOG(DEBUG) << "Failed to get frequency band for SSID " << configuration.ssid
                       << " from hwmode " << hwmode;
            continue;
        }

        // Multi - AP
        std::string management_mode;
        uci_get_option("prplmesh", "prplmesh", "config", "management_mode", management_mode);
        if (management_mode == "Not-Multi-AP") {
            configuration.fronthaul = true;
            configuration.backhaul  = false;
        } else {
            std::string multi_ap;
            uci_get_option(package_name, section_type, section_name, "multi_ap", multi_ap);
            if (multi_ap.empty()) {
                LOG(INFO)
                    << "multi_ap configuration is not found, assign as only fronthaul support";
                configuration.fronthaul = true;
                configuration.backhaul  = false;
            } else {
                if (multi_ap == "1") {
                    configuration.fronthaul = false;
                    configuration.backhaul  = true;
                } else if (multi_ap == "2") {
                    configuration.fronthaul = true;
                    configuration.backhaul  = false;
                } else if (multi_ap == "3") {
                    configuration.fronthaul = true;
                    configuration.backhaul  = true;
                } else {
                    LOG(ERROR) << "multi_ap configuration value is unrecognized " << multi_ap
                               << ", assign as only fronthaul support";
                    configuration.fronthaul = true;
                    configuration.backhaul  = false;
                }
            }
        }

        wireless_settings.push_back(configuration);

        LOG(DEBUG) << "Configuration added for SSID " << configuration.ssid
                   << " (hwmode = " << hwmode << ")";
    }

    return true;
}

bool bpl_cfg_get_wifi_credentials(const std::string &iface,
                                  son::wireless_utils::sBssInfoConf &configuration)
{
    // Find the "wireless.wifi-iface" section in UCI configuration for the given interface
    std::string section_name;
    if (!uci_find_section_by_option("wireless", "wifi-iface", "ifname", iface, section_name)) {
        LOG(ERROR) << "Failed to find configuration section for interface " << iface;
        return false;
    }

    if (section_name.empty()) {
        LOG(ERROR) << "Configuration for interface " << iface << " not found";
        return false;
    }

    // Get SSID and wireless credentials for the given interface.
    if (!bpl_cfg_get_bss_configuration(section_name, configuration)) {
        LOG(ERROR) << "Failed to get wireless configuration for interface " << iface
                   << " at section " << section_name;
        return false;
    }

    return true;
}

bool bpl_cfg_set_wifi_credentials(const std::string &iface,
                                  const son::wireless_utils::sBssInfoConf &configuration)
{
    // Find the "wireless.wifi-iface" section in UCI configuration for the given interface
    const std::string package_name = "wireless";
    const std::string section_type = "wifi-iface";
    const std::string option_name  = "ifname";
    std::string section_name;
    if (!uci_find_section_by_option(package_name, section_type, option_name, iface, section_name)) {
        LOG(ERROR) << "Failed to find configuration section for interface " << iface;
        return false;
    }

    if (section_name.empty()) {
        LOG(ERROR) << "Configuration for interface " << iface << " not found";
        return false;
    }

    // Overwrite UCI configuration with wireless credentials for the given interface
    OptionsUnorderedMap options;
    options["ssid"] = configuration.ssid;

    auto get_encryption = [](WSC::eWscAuth authentication_type, WSC::eWscEncr encryption_type) {
        std::string encryption = "none";
        if (authentication_type == WSC::eWscAuth::WSC_AUTH_WPA2PSK) {
            encryption = "psk2";
            if (encryption_type == WSC::eWscEncr::WSC_ENCR_TKIP) {
                encryption += "+tkip";
            } else if (encryption_type == WSC::eWscEncr::WSC_ENCR_AES) {
                encryption += "+aes";
            }
        } else if (authentication_type == WSC::eWscAuth::WSC_AUTH_SAE) {
            encryption = "sae";
        }
        return encryption;
    };
    options["encryption"] =
        get_encryption(configuration.authentication_type, configuration.encryption_type);

    options["key"] = configuration.network_key;

    // Write UCI options in the "wireless.wifi-iface" section for the given interface.
    if (!uci_set_section(package_name, section_type, section_name, options, true)) {
        LOG(ERROR) << "Failed to set wireless configuration for interface " << iface
                   << " at section " << section_name;
        return false;
    }

    return true;
}

bool bpl_cfg_get_mandatory_interfaces(std::string &mandatory_interfaces)
{
    mandatory_interfaces.clear();

    constexpr int MANDATORY_INTERFACES_SIZE = BPL_IFNAME_LEN * BPL_NUM_OF_INTERFACES + 1;
    char tmp_mandatory_interfaces[MANDATORY_INTERFACES_SIZE];

    if (cfg_get_prplmesh_param("mandatory_interfaces", tmp_mandatory_interfaces,
                               MANDATORY_INTERFACES_SIZE) < 0) {
        LOG(DEBUG) << "Optional parameter mandatory_interfaces doesn't exist";
        return true;
    }

    mandatory_interfaces = std::string(tmp_mandatory_interfaces);

    return true;
}

bool bpl_get_lan_interfaces(std::vector<std::string> &lan_iface_list)
{
    std::string iface_names;
    if (cfg_uci_get_lan_interfaces("lan", iface_names) != RETURN_OK) {
        LOG(ERROR) << "Read network lan interfaces names from UCI failed.";
    }

    lan_iface_list = beerocks::string_utils::str_split(iface_names, ' ');
    return true;
}

bool bpl_cfg_get_backhaul_wire_iface(std::string &iface)
{
    char ifname[BPL_IFNAME_LEN + 1] = {0};

    int retVal = cfg_get_prplmesh_param("backhaul_wire_iface", ifname, BPL_IFNAME_LEN);
    if (retVal == RETURN_ERR) {
        MAPF_ERR("bpl_cfg_get_backhaul_wire_iface: Failed to read backhaul_wire_iface parameter\n");
        return false;
    }

    iface = ifname;

    return true;
}

bool cfg_get_roaming_hysteresis_percent_bonus(int &roaming_hysteresis_percent_bonus)
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int("roaming_hysteresis_percent_bonus", &retVal) == RETURN_ERR) {
        return false;
    }

    roaming_hysteresis_percent_bonus = retVal;
    return true;
}

bool cfg_set_roaming_hysteresis_percent_bonus(int roaming_hysteresis_percent_bonus)
{
    std::string option = "roaming_hysteresis_percent_bonus";
    std::string value  = std::to_string(roaming_hysteresis_percent_bonus);

    return cfg_set_prplmesh_config(option, value);
}

bool cfg_get_steering_disassoc_timer_msec(std::chrono::milliseconds &steering_disassoc_timer_msec)
{
    int retVal = -1;
    if (cfg_get_prplmesh_param_int("steering_disassoc_timer_msec", &retVal) == RETURN_ERR) {
        return false;
    }

    steering_disassoc_timer_msec = std::chrono::milliseconds{retVal};
    return true;
}

bool cfg_set_steering_disassoc_timer_msec(std::chrono::milliseconds &steering_disassoc_timer_msec)
{
    std::string option = "steering_disassoc_timer_msec";
    std::string value  = std::to_string(steering_disassoc_timer_msec.count());

    return cfg_set_prplmesh_config(option, value);
}

bool cfg_get_clients_measurement_mode(eClientsMeasurementMode &clients_measurement_mode)
{
    int val = -1;
    if (cfg_get_prplmesh_param_int_default("clients_measurement_mode", &val,
                                           int(eClientsMeasurementMode::ENABLE_ALL)) ==
        RETURN_ERR) {
        clients_measurement_mode = eClientsMeasurementMode::ENABLE_ALL;
        LOG(ERROR) << "Failed to read clients_measurement_mode parameter, "
                      "default value: ENABLE_ALL";
        return false;
    }

    if (val > int(eClientsMeasurementMode::ONLY_CLIENTS_SELECTED_FOR_STEERING)) {
        LOG(WARNING) << "clients_measurement_mode is set to invalid value - setting "
                        "default value: ENABLE_ALL";
        clients_measurement_mode = eClientsMeasurementMode::ENABLE_ALL;
    } else {
        clients_measurement_mode = eClientsMeasurementMode(val);
    }

    return true;
}

bool cfg_get_radio_stats_enable(bool &radio_stats_enable)
{
    int val = -1;
    if (cfg_get_prplmesh_param_int_default("radio_stats_enable", &val, int(true)) == RETURN_ERR) {
        LOG(ERROR) << "Failed to read radio_stats_enable parameter, setting default value true";
        radio_stats_enable = true;
        return false;
    }

    radio_stats_enable = bool(val == 1);
    return true;
}

bool bpl_cfg_get_monitored_BSSs_by_radio_iface(const std::string &iface,
                                               std::set<std::string> &monitored_BSSs)
{
    // To get the correct VAP list for a given interface name, we first need to get all the
    // avaliable ifaces in relation to their sections.
    // Key: Section, Value: Radio iface
    std::unordered_map<std::string, std::string> radio_ifaces;
    if (cfg_get_prplmesh_hostapd_ifaces(radio_ifaces) == RETURN_ERR) {
        LOG(ERROR) << "Failed to get avaliable interfaces!";
        return false;
    }
    // Iterate over the interface names to find the matching section
    const auto it = std::find_if(radio_ifaces.begin(), radio_ifaces.end(),
                                 [&iface](std::pair<std::string, std::string> const &item) {
                                     return (item.second == iface);
                                 });
    if (it == radio_ifaces.end()) {
        LOG(ERROR) << "Failed to find matching iface " << iface;
        return false;
    }
    // Once the correct section is found we can get the monitored_bsss option.
    const std::string package_name = "prplmesh";
    const std::string section_type = "wifi-device";
    const std::string option_name  = "hostap_iface_monitor_vaps";
    std::string monitored_bsss;
    if (!uci_get_option(package_name, section_type, (*it).first, option_name, monitored_bsss)) {
        LOG(DEBUG) << "Failed to get 'hostap_iface_monitor_vaps' from section " << (*it).first;
        return true;
    }
    // the monitored_bsss option is a list seperated by the ',' delimiter
    auto monitored_BSSs_vec = beerocks::string_utils::str_split(monitored_bsss, ',');
    monitored_BSSs =
        std::move(std::set<std::string>(monitored_BSSs_vec.begin(), monitored_BSSs_vec.end()));
    return true;
    // break monitor vaps to vector
}

} // namespace bpl
} // namespace beerocks
