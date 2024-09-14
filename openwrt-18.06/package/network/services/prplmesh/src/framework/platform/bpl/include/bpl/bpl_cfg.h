/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BPL_CFG_H_
#define _BPL_CFG_H_

#include "bpl.h"
#include "bpl_err.h"

#include <bcl/son/son_wireless_utils.h>

#include <stdint.h>
#include <string>

namespace beerocks {
namespace bpl {

/****************************************************************************/
/******************************* Definitions ********************************/
/****************************************************************************/

/**
 * @brief The possible clients measurement modes are:
 * disable_all - No client is measured (there are some use-cases where prplmesh doesn't manage the clients in the platform)
 * enable_all - The default configuration. All connected clients are measured.
 * only_clients_selected_for_steering - Only clients that are selected for steering (by sending
 * the ACTION_MONITOR_STEERING_CLIENT_SET_REQUEST to the monitor) will be measured. This mode is useful for systems with
 * many clients where only several are configured for steering - so no need to monitor the rest of clients.
 */
enum class eClientsMeasurementMode : uint8_t {
    DISABLE_ALL = 0,
    ENABLE_ALL,
    ONLY_CLIENTS_SELECTED_FOR_STEERING
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eClientsMeasurementMode_str(eClientsMeasurementMode enum_value) {
    switch (enum_value) {
    case eClientsMeasurementMode::DISABLE_ALL:                        return "eClientsMeasurementMode::DISABLE_ALL";
    case eClientsMeasurementMode::ENABLE_ALL:                         return "eClientsMeasurementMode::ENABLE_ALL";
    case eClientsMeasurementMode::ONLY_CLIENTS_SELECTED_FOR_STEERING: return "eClientsMeasurementMode::ONLY_CLIENTS_SELECTED_FOR_STEERING";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eClientsMeasurementMode value) { return out << eClientsMeasurementMode_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end

/* Wi-Fi Settings */
#define BPL_DEV_INFO_LEN (32 + 4) /* Device information string length */
#define BPL_SSID_LEN (32 + 1)     /* Maximal length of Wi-Fi SSID */
#define BPL_PASS_LEN (64 + 1)     /* Maximal length of Wi-Fi password */
#define BPL_SEC_LEN 32            /* Maximal length of Wi-Fi security string */
#define BPL_IFNAME_LEN 32         /* Maximal length of Wi-Fi interface name */
#define BPL_NUM_OF_INTERFACES 7   /* Maximal number of Interfaces: (3 APs + 3 STAs) + 1 Wired */
#define BPL_MNS_DATA_LEN 256      /* Maximal length of BPL MNS data */
#define BPL_BACK_VAPS_GROUPS 4 /* Backhaul VAPs Groups size, group contain 1 Vap for every radio */
#define BPL_BACK_VAPS_IN_GROUP 3   /* Number of VAPs in 1 VAP group */
#define BPL_MAC_ADDR_OCTETS_LEN 6  /* Number of octets in mac address */
#define BPL_IPV4_ADDR_OCTETS_LEN 4 /* Number of octets in ipv4 address */
#define BPL_BACKHAUL_BAND_LEN 7    /* Max length of backhaul preferred band: 2.4GHz/5GHz/auto */

/* Radio Direction */
#define BPL_RADIO_BAND_AUTO 0
#define BPL_RADIO_FRONT 0
#define BPL_RADIO_BACK 1

/* Radio Band */
#define BPL_RADIO_BAND_5G 5
#define BPL_RADIO_BAND_2G 2

/* Platform Operating Mode */
#define BPL_OPER_MODE_GATEWAY 0
#define BPL_OPER_MODE_GATEWAY_WISP 1
#define BPL_OPER_MODE_WDS_EXTENDER 2
#define BPL_OPER_MODE_WDS_REPEATER 3
#define BPL_OPER_MODE_L2NAT_CLIENT 4

/* Platform Management Mode */
#define BPL_MGMT_MODE_MULTIAP_CONTROLLER_AGENT 0 /* EasyMesh controller and agent */
#define BPL_MGMT_MODE_MULTIAP_CONTROLLER 1       /* EasyMesh controller */
#define BPL_MGMT_MODE_MULTIAP_AGENT 2            /* EasyMesh agent */
#define BPL_MGMT_MODE_NOT_MULTIAP 3              /* Non EasyMesh */

/* Platform Certification Mode */
#define BPL_CERTIFICATION_MODE_OFF 0
#define BPL_CERTIFICATION_MODE_ON 1

/* Platform Information */
#define BPL_USER_PASS_LEN (64 + 1)           /* Maximal length of USER_PASS */
#define BPL_SERIAL_NUMBER_LEN (64 + 1)       /* Maximal length of SERIAL_NUMBER */
#define BPL_LOAD_STEER_ON_VAPS_LEN (127 + 1) /* Maximal length of LOAD_STEER_ON_VAPS string */
#define BPL_COUNTRY_CODE_LEN (2 + 1)         /* Maximal length of "2 letters country code" */

/* Platform notification types */
#define BPL_NOTIF_VER_MISMATCH 1 /* Version mismatch detected */
#define BPL_NOTIF_ERROR 2        /* Error notification */
#define BPL_NOTIF_WPS_START 3    /* WPS button pressed */
#define BPL_NOTIF_MNS_OPER 4     /* MNS operation request */
#define BPL_NOTIF_WPS_COMPLETE 5 /* Set WPS Status */

/* Wi-Fi Security Mode Strings */
#define BPL_WLAN_SEC_NONE_STR "None"
#define BPL_WLAN_SEC_WEP64_STR "WEP-64"
#define BPL_WLAN_SEC_WEP128_STR "WEP-128"
#define BPL_WLAN_SEC_WPA_PSK_STR "WPA-Personal"
#define BPL_WLAN_SEC_WPA2_PSK_STR "WPA2-Personal"
#define BPL_WLAN_SEC_WPA_WPA2_PSK_STR "WPA-WPA2-Personal"

/* Gateway database */
#define BPL_GW_DB_MANAGE_MODE_LEN (127 + 1) /* Maximal length of MANAGEMENT MODE string */
#define BPL_GW_DB_OPER_MODE_LEN (127 + 1)   /* Maximal length of OPERATING MODE string */

/* Default values */
constexpr int DEFAULT_STOP_ON_FAILURE_ATTEMPTS         = 1;
constexpr int DEFAULT_RDKB_EXTENSIONS                  = 0;
constexpr int DEFAULT_DFS_REENTRY                      = 1;
constexpr int DEFAULT_BAND_STEERING                    = 0;
constexpr int DEFAULT_CLIENT_ROAMING                   = 0;
constexpr int DEFAULT_ROAMING_HYSTERESIS_PERCENT_BONUS = 10;
constexpr std::chrono::milliseconds DEFAULT_STEERING_DISASSOC_TIMER_MSEC{200};

// by-default the persistent DB is disabled to allow backwards compatability
// if the parameter is not configured in the prplmesh config and set to 1, DB is disabled
constexpr int DEFAULT_PERSISTENT_DB = 0;
// The default value in seconds for the interval between periodic commits of persistent DB data.
constexpr unsigned int DEFAULT_COMMIT_CHANGES_INTERVAL_VALUE_SEC = 10;
// the DB of clients is limited in size to prevent high memory consumption
// this is configurable to enable flexibility and support for low-memory platforms
// by default, the number of clients's configuration to be cached is limited to 256
constexpr int DEFAULT_CLIENTS_PERSISTENT_DB_MAX_SIZE = 256;
// the DB of client's steer history is limited in size to prevent high memory consumption
// this is configurable to enable flexibility and support for low-memory platforms
// by default, the number of steer history entries is limited to 24
constexpr int DEFAULT_STEER_HISTORY_PERSISTENT_DB_MAX_SIZE = 24;
// the persistent data of clients has aging limit
// by default, the limit is 365 days scaled to minutes, but it is configurable via the UCI
constexpr int DEFAULT_MAX_TIMELIFE_DELAY_MINUTES = 365 * 24 * 60;
// the timelife of unfriendly-devices is set separately and can be shorter than the timelife
// TODO: add description of "unfriendly-device" and how it is determined
// by default, the limit is 1 day scaled to minutes, but it is configurable via the UCI
constexpr int DEFAULT_UNFRIENDLY_DEVICE_MAX_TIMELIFE_DELAY_MINUTES = 1 * 24 * 60;
// the persistent DB's aging mechanism needs to be checked periodically.
// by default, the interval in which the aging mechanism needs to be checked is once per hour.
constexpr int DEFAULT_PERSISTENT_DB_AGING_INTERVAL_SEC = 3600;
// by default zwdfs functionality is disabled
constexpr int DEFAULT_ZWDFS_ENABLE = 0;
// Channel ranking used to determine best channel candidate.
// Using threshold to avoid high frequency channel switch.
// By default best channel ranking threshold is 0.
constexpr int DEFAULT_BEST_CHANNEL_RANKING_TH = 0;

// Link metrics tasks send request with this interval.
constexpr std::chrono::seconds DEFAULT_LINK_METRICS_REQUEST_INTERVAL_VALUE_SEC{60};

// Default Linux Lan interface names. It needs to be space separeted.
constexpr char DEFAULT_LINUX_LAN_INTERFACE_NAMES[] = "eth0_1 eth0_2 eth0_3 eth0_4";

// Default DHCP tasks process lease information with this interval.
constexpr std::chrono::seconds DEFAULT_DHCP_MONITOR_INTERVAL_VALUE_SEC{300};

// Default policy for report unsuccessful associations.
constexpr int DEFAULT_UNSUCCESSFUL_ASSOC_REPORT_POLICY = 1;

// Default value in attempts per minute for maximum rate for reporting unsuccessful association attempts
constexpr int DEFAULT_UNSUCCESSFUL_ASSOC_MAX_REPORTING_RATE = 30;

/****************************************************************************/
/******************************* Structures *********************************/
/****************************************************************************/

/* Error Notification */
struct BPL_ERROR {

    /* Error code of type BPL_ERR_... */
    int code;

    /* Custom string data reported by the module */
    char data[BPL_ERROR_STRING_LEN];
};

/* Wi-Fi Credentials */
struct BPL_WIFI_CREDENTIALS {

    /* Wi-Fi SSID */
    char ssid[BPL_SSID_LEN];

    /* Wi-Fi KeyPassphrase */
    char pass[BPL_PASS_LEN];

    /* Wi-Fi Security Mode */
    char sec[BPL_SEC_LEN];
};

/* WPS Trigger params */
struct BPL_WPS_PARAMS {

    /* Wi-Fi interface name */
    char ifname[BPL_IFNAME_LEN];

    /* wps type (0-pbc, 1-pin) */
    int wps_type;
};

enum BPL_WPS_TYPE { BPL_WPS_PBC = 0, BPL_WPS_PIN };
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *BPL_WPS_TYPE_str(BPL_WPS_TYPE enum_value) {
    switch (enum_value) {
    case BPL_WPS_PBC: return "BPL_WPS_PBC";
    case BPL_WPS_PIN: return "BPL_WPS_PIN";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, BPL_WPS_TYPE value) { return out << BPL_WPS_TYPE_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end

/* MNS operation params */
struct BPL_MNS_PARAMS {
    /* operation of type BPL_MNS_OP_... */
    int op;

    /* data */
    char data[BPL_MNS_DATA_LEN];
};

enum BPL_MNS_OP {
    BPL_MNS_OP_RESET = 0,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *BPL_MNS_OP_str(BPL_MNS_OP enum_value) {
    switch (enum_value) {
    case BPL_MNS_OP_RESET: return "BPL_MNS_OP_RESET";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, BPL_MNS_OP value) { return out << BPL_MNS_OP_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end

/* Interface state for the platform*/
struct BPL_NOTIF_WPS_COMPLETE_NOTIFICATION {

    /* Wi-Fi SSID */
    char ssid[BPL_SSID_LEN];

    /* Wi-Fi KeyPassphrase */
    char pass[BPL_PASS_LEN];

    /* Wi-Fi Security Mode */
    char sec[BPL_SEC_LEN];

    /* Wi-Fi interface name */
    char ifname[BPL_IFNAME_LEN];

    /* WPS Status success=0/fail=1*/
    int status;
};

/* WLAN params */
struct BPL_WLAN_PARAMS {

    /* Wi-Fi AP enable */
    int enabled;

    /* Wi-Fi Channel (0 for ACS) */
    int channel;

    /* Sub band DFS channel enable */
    bool sub_band_dfs;

    /* country code */
    char country_code[BPL_COUNTRY_CODE_LEN];
};

/**
 * a structure to couple together radio-number (also correlates to slave-number) and interface name
 */
struct BPL_WLAN_IFACE {
    int radio_num;
    char ifname[BPL_IFNAME_LEN];
};

/****************************************************************************/
/******************************** Functions *********************************/
/****************************************************************************/

/**
 * Returns the beerocks state.
 *
 * @return 1 if enabled.
 * @return 0 if disabled.
 * @return -1 Error.
 */
int cfg_is_enabled();

/**
 * Returns whether the current platform is configured as Master.
 *
 * @return 1 Master.
 * @return 0 IRE.
 * @return -1 Error.
 */
int cfg_is_master();

/**
 * Returns whether the current platform is configured as Gateway.
 *
 * @return valid possibilities:
 *   BPL_OPER_MODE_GATEWAY,
 *   BPL_OPER_MODE_GATEWAY_WISP,
 *   BPL_OPER_MODE_WDS_EXTENDER,
 *   BPL_OPER_MODE_WDS_REPEATER,
 *   BPL_OPER_MODE_L2NAT_CLIENT
 * @return -1 Error.
 */
int cfg_get_operating_mode();

/**
 * Returns the current management mode configuration.
 *
 * @return valid possibilities:
 *   BPL_MGMT_MODE_MULTIAP_CONTROLLER_AGENT,
 *   BPL_MGMT_MODE_MULTIAP_CONTROLLER,
 *   BPL_MGMT_MODE_MULTIAP_AGENT,
 *   BPL_MGMT_MODE_NOT_MULTIAP
 * @return -1 Error.
 */
int cfg_get_management_mode();

/**
 * Returns certification mode value.
 *
 * @return BPL_CERTIFICATION_MODE_ON Certification mode is ON.
 * @return BPL_CERTIFICATION_MODE_OFF Certification mode is OFF.
 * @return -1 Error.
 */
int cfg_get_certification_mode();

/**
 * Returns the comma-separated list of VAPs to steer on.
 *
 * @param [int] num_of_interfaces Max num of interfaces.
 * @param [out] load_steer_on_vaps VAPs to steer on (up to 128 bytes in length).
 *
 * @return 0 Success.
 * @return -1 Error.
 */
int cfg_get_load_steer_on_vaps(int num_of_interfaces,
                               char load_steer_on_vaps[BPL_LOAD_STEER_ON_VAPS_LEN]);

/**
 * Returns the maximum number of failures allowed on agent before stopping its execution.
 *
 * @return Maximum number of failures allowed or 0 to retry indefinitely.
 * @return -1 Error.
 */
int cfg_get_stop_on_failure_attempts();

/**
 * Returns whether the platform is in onboarding state.
 *
 * @return 1 Platform is in onboarding state.
 * @return 0 Platform is NOT in onboarding state.
 * @return -1 Error.
 */
int cfg_is_onboarding();

/**
 * Checks the state of the RDKB Extensions feature.
 *
 * @return 1 Enabled.
 * @return 0 Disabled.
 * @return -1 Error.
 */
int cfg_get_rdkb_extensions();

/**
 * @brief Returns whether Band Steering feature is enabled or not.
 *
 * @param [out] enable true if the Band Steering is enabled and false otherwise.
 * @return true on success, otherwise false.
 */
bool cfg_get_band_steering(bool &band_steering);

/**
 * @brief Sets Band Steering in configuration.
 *
 * @param[in] band_steering  true for enabled band_steering.
 * @return true on success, otherwise false
 */
bool cfg_set_band_steering(bool band_steering);

/**
 * @brief Returns whether Client Roaming feature is enabled or not.
 *
 * @param [out] enable true if the Client Roaming is enabled and false otherwise.
 * @return true on success, otherwise false.
 */
bool cfg_get_client_roaming(bool &client_roaming);

/**
 * @brief Sets Client Roaming in configuration.
 *
 * @param[in] client_roaming  true for enabled client_roaming.
 * @return true on success, otherwise false
 */
bool cfg_set_client_roaming(bool client_roaming);

/**
 * Returns miscellaneous Wi-Fi parameters.
 *
 * @param [in] iface Interface name for the requested parameters.
 * @param [out] WLAN parameters structure.
 *
 * NOTE: NULL output parameters should not be filled.
 *
 * @return 0 Success.
 * @return -1 Error.
 */
int cfg_get_wifi_params(const char *iface, struct BPL_WLAN_PARAMS *wlan_params);

/**
 * Returns backhaul vaps configuration.
 *
 * @param [out] max_vaps an int.
 * @param [out] network_enabled 1 if network is enabled or 0 otherwise.
 * @param [out] preferred_radio_band BPL_RADIO_BAND_5G or BPL_RADIO_BAND_2G or BPL_RADIO_BAND_AUTO
 *
 * @return 0 Success.
 * @return -1 Error.
 */
int cfg_get_backhaul_params(int *max_vaps, int *network_enabled, int *preferred_radio_band);

/**
 * Returns backhaul vaps list.
 *
 * @param [out] backhaul_vaps_buf buffer for backhaul vaps list.
 * @param [in]  buf_len buffer length.
 *
 * @return 0 Success.
 * @return -1 Error.
 */
int cfg_get_backhaul_vaps(char *backhaul_vaps_buf, const int buf_len);

/**
 * Returns the platform Wi-Fi settings.
 *
 * @param [in] radio_dir radio direction (BPL_RADIO_FRONT/BPL_RADIO_BACK)
 * @param [out] ssid SSID (up to 32 bytes in length).
 * @param [out] pass Password (up to 64 bytes in length).
 * @param [out] sec Security Mode (up to 32 bytes in length).
 *
 * NOTE: NULL output parameters should not be filled.
 *
 * @return 0 Success.
 * @return -1 Error.
 */
int cfg_get_beerocks_credentials(const int radio_dir, char ssid[BPL_SSID_LEN],
                                 char pass[BPL_PASS_LEN], char sec[BPL_SEC_LEN]);

/**
 * @brief Returns the platform SDL policy which is represented by 'mem_only_psk' flag.
 * 'mem_only_psk' flag sets whether the platform shall save the credentials in some encrypted DB so
 * the wpa-supplicant will get from there in runtime, without supply it on `connect` API, or not.
 *
 * @return mem_only_psk flag on success.
 * @return -1 Error.
 */
int cfg_get_security_policy();

/**
 * Set platform onboarding state.
 *
 * @param [in] enable, 0= disable, 1 = enable.
 *
 * @return 0 Success.
 * @return -1 Error.
 */
int cfg_set_onboarding(int enable);

/**
 * Notify the platform on onboarding completed.
 *
 * @param [in] ssid SSID (up to 32 bytes in length).
 * @param [in] pass Password (up to 64 bytes in length).
 * @param [in] sec Security Mode (up to 32 bytes in length).
 * @param [in] iface_name Interface name (up to 32 bytes in length).
 * @param [in] success Success of onboarding (0 - failure, 1 - success).
 */
int cfg_notify_onboarding_completed(const char ssid[BPL_SSID_LEN], const char pass[BPL_PASS_LEN],
                                    const char sec[BPL_SEC_LEN],
                                    const char iface_name[BPL_IFNAME_LEN], const int success);

/**
 * Notify the platform about an error.
 *
 * @param [in] error_code Error code (of type BPL_ERR...)
 * @param [in] error_str "printf" style formatted string.
 *
 * @return 0 Success.
 * @return -1 Error.
 */
int cfg_notify_error(int code, const char str[BPL_ERROR_STRING_LEN]);

/**
 * Returns the platform administrator password.
 *
 * @param [out] pass Password (up to 64 bytes in length).
 *
 * @return 0 Success.
 * @return -1 Error.
 */
int cfg_get_administrator_credentials(char pass[BPL_USER_PASS_LEN]);

/**
 * Returns the STA interface for the specified radio id.
 *
 * @param [in] iface Interface name for the requested parameters.
 * @param [out] sta_iface name of STA interface (up to 32 bytes in length).
 *
 * @return 0 Success.
 * @return -1 Error, or no sta_iface is configured.
 */
int cfg_get_sta_iface(const char iface[BPL_IFNAME_LEN], char sta_iface[BPL_IFNAME_LEN]);

/**
 * Returns the HOSTAP interface for the specified radio id.
 *
 * @param [in] radio_num radio number in prplmesh UCI for the requested parameters.
 * @param [out] hostap_iface name of HOSTAP interface (up to 32 bytes in length).
 *
 * @return 0 Success.
 * @return -1 Error, or no hostap_iface is configured.
 */
int cfg_get_hostap_iface(int32_t radio_num, char hostap_iface[BPL_IFNAME_LEN]);

/**
 * Returns all the HOSTAP interfaces available in prplmesh config file
 *
 * @param [out] interfaces list of HOSTAP interfaces of type BPL_WLAN_IFACE.
 * @param [int/out] num_of_interfaces in:max num of interfaces, out:actual num of interfaces.
 *
 * @return 0 Success.
 * @return -1 Error, or no hostap_iface is configured.
 */
int cfg_get_all_prplmesh_wifi_interfaces(BPL_WLAN_IFACE *interfaces, int *num_of_interfaces);

/**
 * @brief Returns whether the zwdfs feature is enabled.
 *
 * @param [out] enable true if the zwdfs is enabled, otherwise false.
 * @return true on success, otherwise false.
 */
bool cfg_get_zwdfs_enable(bool &enable);

/**
 * @brief Returns best channel ranking threshold.
 *
 * @param [out] threshold Ranking value used to determine best channel candidate.
 * Threshold will be used to avoid high frequency channel switch.
 * @return true on success, otherwise false.
 */
bool cfg_get_best_channel_rank_threshold(uint32_t &threshold);

/**
 * @brief Returns whether the persistent DB is enabled.
 *
 * @param [out] enable true if the DB is enabled and false otherwise.
 * @return true on success, otherwise false.
 */
bool cfg_get_persistent_db_enable(bool &enable);

/**
 * @brief Returns commit_changes_interval (seconds) value.
 *
 * @param[out] interval_sec The interval in seconds between periodic persistent data commit operations.
 * @return true on success, otherwise false.
 */
bool cfg_get_persistent_db_commit_changes_interval(unsigned int &interval_sec);

/**
 * @brief Returns the max number of clients in the persistent DB.
 *
 * @param [out] max_size Max number of clients the persistent-db supports.
 * @return true on success, otherwise false.
 */
bool cfg_get_clients_persistent_db_max_size(int &max_size);

/**
 * @brief Returns the max number of steer history entries in the persistent DB.
 *
 * @param [out] max_size Max number of steer history entries the persistent-db supports.
 * @return true on success, otherwise false.
 */
bool cfg_get_steer_history_persistent_db_max_size(size_t &max_size);

/**
 * @brief Returns the max time-life delay of clients (used for aging of client's persistent data).
 *
 * @param [out] max_timelife_delay_minutes Max clients' timelife delay.
 * @return true on success, otherwise false.
 */
bool cfg_get_max_timelife_delay_minutes(int &max_timelife_delay_minutes);

/**
 * @brief Returns the max time-life delay for unfriendly clients.
 *
 * @param [out] unfriendly_device_max_timelife_delay_minutes Max unfriendly clients' timelife delay.
 * @return true on success, otherwise false.
 */
bool cfg_get_unfriendly_device_max_timelife_delay_minutes(
    int &unfriendly_device_max_timelife_delay_minutes);

/**
 * @brief Returns the interval to check the persistent DB aging
 *
 * @param [out] persistent_db_aging_interval Interval for checking persistent DB aging.
 * @return true on success, otherwise false
 */
bool cfg_get_persistent_db_aging_interval(int &persistent_db_aging_interval_sec);

/**
 * @brief Returns configured WPA Control Path for the given interface.
 *
 * @param [in] Interface name
 * @param [out] WPA Control Path
 * @return true on success, otherwise false.
 */
bool bpl_cfg_get_wpa_supplicant_ctrl_path(const std::string &iface, std::string &wpa_ctrl_path);

/**
 * @brief Returns configured Hostapd Control Path for the given interface.
 *
 * @param [in] Interface name
 * @param [out] Hostapd Control Path
 * @return true on success, otherwise false.
 */
bool bpl_cfg_get_hostapd_ctrl_path(const std::string &iface, std::string &hostapd_ctrl_path);

/**
 * @brief Reads wireless settings (SSIDs and WiFi credentials) for all fronthaul interfaces.
 *
 * This method is intended to be used from the controller, to import existing wireless settings and
 * later apply them to the whole network.
 *
 * @param [out] wireless_settings List of wireless network configurations.
 * @return true on success and false otherwise.
 */
bool bpl_cfg_get_wireless_settings(std::list<son::wireless_utils::sBssInfoConf> &wireless_settings);

/**
 * @brief Reads wireless network configuration for the given interface.
 *
 * @param [in] iface Interface name.
 * @param [out] configuration Wireless network configuration.
 * @return true on success and false otherwise.
 */
bool bpl_cfg_get_wifi_credentials(const std::string &iface,
                                  son::wireless_utils::sBssInfoConf &configuration);

/**
 * @brief Reads link metrics request interval configuration for periodic requests from agents.
 *
 * @param [out] link_metrics_request_interval_sec Interval for periodic link metrics request.
 * @return true on success, otherwise false
 */
bool cfg_get_link_metrics_request_interval(std::chrono::seconds &link_metrics_request_interval_sec);

/**
 * @brief Sets link metrics request interval configuration for periodic requests from agents.
 *
 * @param [in] link_metrics_request_interval_sec Interval for periodic link metrics request to set.
 * @return true on success, otherwise false
 */
bool cfg_set_link_metrics_request_interval(std::chrono::seconds &link_metrics_request_interval_sec);

/**
 * @brief Reads policy setting for report unsuccessful associations.
 *
 * @param [out] unsuccessful_assoc_report_policy Policy setting for report unsuccessful associations.
 * @return true on success, otherwise false
 */
bool cfg_get_unsuccessful_assoc_report_policy(bool &unsuccessful_assoc_report_policy);

/**
 * @brief Sets policy setting for report unsuccessful associations.
 *
 * @param [in] unsuccessful_assoc_report_policy Policy setting for report unsuccessful associations to set.
 * @return true on success, otherwise false
 */
bool cfg_set_unsuccessful_assoc_report_policy(bool &unsuccessful_assoc_report_policy);

/**
 * @brief Reads maximum rate for reporting unsuccessful association attempts.
 *
 * @param [out] max_reporting_rate Maximum reporting rate.value in attempts per minute.
 * @return true on success, otherwise false
 */
bool cfg_get_unsuccessful_assoc_max_reporting_rate(int &max_reporting_rate);

/**
 * @brief Sets maximum rate for reporting unsuccessful association attempts.
 *
 * @param [in] max_reporting_rate Maximum reporting rate.value in attempts per minute.
 * @return true on success, otherwise false
 */
bool cfg_set_unsuccessful_assoc_max_reporting_rate(int &max_reporting_rate);

/**
 * @brief Reads lan interfaces names from bridge configuration.
 *
 * @param [out] lan_iface_list lan interfaces name list
 * @return true on success, otherwise false
 */
bool bpl_get_lan_interfaces(std::vector<std::string> &lan_iface_list);

/**
 * @brief Writes wireless network configuration for the given interface.
 *
 * @param [in] iface Interface name.
 * @param [in] configuration Wireless network configuration.
 * @return true on success and false otherwise.
 */
bool bpl_cfg_set_wifi_credentials(const std::string &iface,
                                  const son::wireless_utils::sBssInfoConf &configuration);

/**
 * @brief Reads mandatory interfaces configuration.
 *
 * @param[out] mandatory_interfaces Comma-separated list of interfaces which are mandatory and need to create son_slaves for even if currently they are down.
 * @return true on success, otherwise false
 */
bool bpl_cfg_get_mandatory_interfaces(std::string &mandatory_interfaces);

/**
 * @brief Returns wire backhaul interface.
 *
 * @param [out] iface Wire backhaul interface name.
 * @return true on success, otherwise false.
 */
bool bpl_cfg_get_backhaul_wire_iface(std::string &iface);

/**
 * @brief Reads roaming hysteresis percent bonus.
 *
 * Bonus (in %) given to current BSS as hysteresis. Setting this to 0 gives no hysteresis at all,
 * setting to 20 means that an alternative AP has to be estimated to be 20% better than the current one before it's considered.
 * Maximum value is 10000.
 *
 * @param[out] roaming_hysteresis_percent_bonus roaming hysteresis percentage.
 * @return true on success, otherwise false
 */
bool cfg_get_roaming_hysteresis_percent_bonus(int &roaming_hysteresis_percent_bonus);

/**
 * @brief Sets roaming hysteresis percentage in configuration.
 *
 * @param[in] roaming_hysteresis_percent_bonus roaming hysteresis percentage.
 * @return true on success, otherwise false
 */
bool cfg_set_roaming_hysteresis_percent_bonus(int roaming_hysteresis_percent_bonus);

/**
 * @brief Reads steering disassociation timer in milliseconds.
 *
 * It is the time before the STA is forcefully disassociated.
 * When STA is triggered to steer, it is not allowed to return to the original BSS in this time period.
 * Value should be exceed 32-bit. So maximum value is 4 billion, which corresponds to about a month and a half.
 *
 * @param[out] steering_disassoc_timer_msec  steering disassociation timer in milliseconds.
 * @return true on success, otherwise false
 */
bool cfg_get_steering_disassoc_timer_msec(std::chrono::milliseconds &steering_disassoc_timer_msec);

/**
 * @brief Sets steering disassociation timer in milliseconds in configuration.
 *
 * @param[in] steering_disassoc_timer_msec  steering disassociation timer in milliseconds.
 * @return true on success, otherwise false
 */
bool cfg_set_steering_disassoc_timer_msec(std::chrono::milliseconds &steering_disassoc_timer_msec);

/**
 * @brief Reads Clients stats/measurements mode.
 *
 * @param[out] clients_measurement_mode Clients measurements mode. @see eClientsMeasurementMode.
 * @return true on success, otherwise false
 */
bool cfg_get_clients_measurement_mode(eClientsMeasurementMode &clients_measurement_mode);

bool cfg_get_radio_stats_enable(bool &radio_stats_enable);

/**
 * @brief Get radio's monitored BSSIDs by radio's interface.
 * 
 * @param [in] iface Radio interface name.
 * @param [out] monitored_BSSs Set of BSSIDs to monitor.
 * @return true on success, otherwise false.
 */
bool bpl_cfg_get_monitored_BSSs_by_radio_iface(const std::string &iface,
                                               std::set<std::string> &monitored_BSSs);

/**
 * @brief Get a string identifying the particular device that is unique for the indicated model
 * and manufacturer.
 * 
 * @note It is the manufacturer responsability to override implementation of this function and
 * and return a correct string.
 * 
 * @param [out] serial_number  Serial number of the device.
 * @return true on success, otherwise false.
 */
bool get_serial_number(std::string &serial_number);

/**
 * @brief Get a string identifying the Wi-Fi chip vendor a radio.
 * 
 * @note It is the manufacturer responsability to override implementation of this function and
 * and return a correct string.
 * 
 * @param [in] ruid  Radio UID of the radio to get the chipset vendor from.
 * @param [out] chipset_vendor  Chipset vendor of the radio specified by @a ruid.
 * @return true on success, otherwise false.
 */
bool get_ruid_chipset_vendor(const sMacAddr &ruid, std::string &chipset_vendor);

/**
 * @brief Get the maximum service prioritization rules supported by the Multi-AP Agent.
 * 
 * @note It is the manufacturer responsability to override implementation of this function and
 * and return a correct string.
 * 
 * @param [out] max_prioritization_rules The maximum service prioritization rules supported by the
 * Multi-AP Agent. If not supported set to zero.
 * @return true on success, otherwise false.
 */
bool get_max_prioritization_rules(uint32_t &max_prioritization_rules);

} // namespace bpl
} // namespace beerocks

#endif /* _BPL_CFG_H_ */
