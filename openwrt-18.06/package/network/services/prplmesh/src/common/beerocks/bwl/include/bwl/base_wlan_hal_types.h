/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BWL_BASE_WLAN_HAL_TYPES_H_
#define _BWL_BASE_WLAN_HAL_TYPES_H_

#include <bcl/beerocks_defines.h>
#include <bcl/beerocks_message_structs.h>
#include <bcl/network/net_struct.h>

#include <stdint.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace bwl {

enum class HALType {
    Invalid = 0,

    AccessPoint,
    Monitor,
    Station
};

enum class IfaceType {
    Unsupported = 0x00,
    Intel       = 0x01,
};

enum class HALState {
    Uninitialized = 0, /*!< Not initialized */
    Initializing  = 1, /*!< Initializing in progress */
    Operational   = 2, /*!< Initialized and active */
    Failed        = 3  /*!< Failed to initialize */
};

enum class AntMode { Invalid = 0, ANT_1X1, ANT_2X2, ANT_3X3, ANT_4X4 };

struct VAPElement {
    /**
     * Basic Service Set (i.e.: VAP name, e.g.: wlan0.0, wlan0.1, wlan0.2, ...).
     */
    std::string bss;
    std::string ssid;
    std::string mac;
    bool fronthaul;
    bool backhaul;
    bool profile1_backhaul_sta_association_disallowed = false;
    bool profile2_backhaul_sta_association_disallowed = false;

    bool operator==(const VAPElement &other) const
    {
        return (bss == other.bss && ssid == other.ssid && mac == other.mac);
    }

    bool operator!=(const VAPElement &other) const { return !(*this == other); }
};

enum class ChanSwReason { Unknown = 0, Radar = 1, CoEx_20 = 2, CoEx_40 = 3 };

// WLAN Security Types
enum class WiFiSec { Invalid = 0, None, WEP_64, WEP_128, WPA_PSK, WPA2_PSK, WPA_WPA2_PSK };

inline std::ostream &operator<<(std::ostream &out, const bwl::WiFiSec &sec)
{
    switch (sec) {
    case bwl::WiFiSec::Invalid:
        out << "Invalid";
        break;
    case bwl::WiFiSec::None:
        out << "None";
        break;
    case bwl::WiFiSec::WEP_64:
        out << "WEP-64";
        break;
    case bwl::WiFiSec::WEP_128:
        out << "WEP-128";
        break;
    case bwl::WiFiSec::WPA_PSK:
        out << "WPA-Personal";
        break;
    case bwl::WiFiSec::WPA2_PSK:
        out << "WPA2-Personal";
        break;
    case bwl::WiFiSec::WPA_WPA2_PSK:
        out << "WPA-WPA2-Personal";
        break;
    }
    return out;
}

enum eRadioState : uint8_t {
    UNINITIALIZED,
    DISABLED,
    COUNTRY_UPDATE,
    ACS,
    ACS_DONE,
    HT_SCAN,
    DFS,
    ENABLED,
    UNKNOWN
};

// clang-format off
static const std::unordered_map<eRadioState, const char *, std::hash<int>> eRadioState_string = {
  { eRadioState::UNINITIALIZED,  "UNINITIALIZED"  },
  { eRadioState::DISABLED,       "DISABLED"       },
  { eRadioState::COUNTRY_UPDATE, "COUNTRY_UPDATE" },
  { eRadioState::ACS,            "ACS"            },
  { eRadioState::ACS_DONE,       "ACS_DONE"       },
  { eRadioState::HT_SCAN,        "HT_SCAN"        },
  { eRadioState::DFS,            "DFS"            },
  { eRadioState::ENABLED,        "ENABLED"        },
  { eRadioState::UNKNOWN,        "UNKNOWN"        },
};
// clang-format on

inline std::ostream &operator<<(std::ostream &out, eRadioState radio_state)
{
    return out << eRadioState_string.at(radio_state);
}

struct sChannelInfo {
    int8_t tx_power_dbm;
    beerocks::eDfsState dfs_state;
    // Key: eWiFiBandwidth, Value: Rank
    std::map<beerocks::eWiFiBandwidth, int32_t> bw_info_list;
};

struct RadioInfo {
    std::string iface_name;
    IfaceType iface_type               = IfaceType::Unsupported;
    eRadioState radio_state            = eRadioState::UNKNOWN;
    int wifi_ctrl_enabled              = 0; // Hostapd / wpa_supplicant
    bool tx_enabled                    = false;
    bool is_5ghz                       = false;
    int channel                        = 0;
    int bandwidth                      = 0;
    int channel_ext_above              = 0;
    int vht_center_freq                = 0;
    bool is_dfs_channel                = false;
    int ant_num                        = 0;
    int tx_power                       = 0;
    beerocks::eFreqType frequency_band = beerocks::eFreqType::FREQ_UNKNOWN; /**< Frequency band */
    beerocks::eWiFiBandwidth max_bandwidth =
        beerocks::eWiFiBandwidth::BANDWIDTH_UNKNOWN; /**< Maximum supported bandwidth */
    bool ht_supported     = false;                   /**< Is HT supported flag */
    uint8_t ht_capability = 0;                       /**< HT capabilities */

    /**< 16-byte attribute containing the MCS set as defined in 802.11n */
    std::array<uint8_t, beerocks::message::HT_MCS_SET_SIZE> ht_mcs_set;
    bool vht_supported      = false; /**< Is VHT supported flag */
    uint16_t vht_capability = 0;     /**< VHT capabilities */

    /**< 32-byte attribute containing the MCS set as defined in 802.11ac */
    std::array<uint8_t, beerocks::message::VHT_MCS_SET_SIZE> vht_mcs_set;

    bool he_supported      = false; /**< Is HE supported flag */
    uint16_t he_capability = 0;     /**< HE capabilities */
    /**< 32-byte attribute containing the MCS set as defined in 802.11ax */
    std::array<uint8_t, beerocks::message::HE_MCS_SET_SIZE> he_mcs_set;

    ChanSwReason last_csa_sw_reason = ChanSwReason::Unknown;
    // Key = channel
    std::map<uint8_t, sChannelInfo> channels_list;
    std::unordered_map<int, VAPElement> available_vaps; // key = vap_id
};

struct hal_conf_t {
    bool ap_acs_enabled = false;
    std::string wpa_ctrl_path;
    std::set<std::string> monitored_BSSs;
};

//sta_wlan_hal
typedef struct {
    beerocks::net::sScanResult result;
    char iface_name[beerocks::message::IFACE_NAME_LENGTH];
    uint16_t rx_phy_rate_100kb;
    uint16_t tx_phy_rate_100kb;
    int8_t rx_rssi;
    uint8_t rx_snr;
    int8_t rx_packets;
    uint8_t src_module;
    int8_t vap_id;
} sNodeRssiMeasurement;

typedef struct {
    uint8_t multi_ap_profile;
    uint16_t multi_ap_primary_vlan_id;
} sACTION_BACKHAUL_CONNECTED_NOTIFICATION;

typedef struct {
    uint32_t disconnect_reason;
    sMacAddr bssid;
} sACTION_BACKHAUL_DISCONNECT_REASON_NOTIFICATION;

typedef struct {
    sNodeRssiMeasurement params;
} sACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE;

//ap_wlan_hal

enum eWiFiStandard : uint8_t {
    STANDARD_NONE = 0x00,
    STANDARD_A    = 0x01,
    STANDARD_B    = 0x02,
    STANDARD_G    = 0x04,
    STANDARD_N    = 0x08,
    STANDARD_AC   = 0x10,
    STANDARD_AX   = 0x20,
};

enum eWiFiMCS : uint8_t {
    MCS_0 = 0,
    MCS_1,
    MCS_2,
    MCS_3,
    MCS_4,
    MCS_5,
    MCS_6,
    MCS_7,
    MCS_8,
    MCS_9,
    MCS_MAX,
};

typedef struct {
    sNodeRssiMeasurement params;
} sACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE;

typedef struct {
    sMacAddr client_mac;
    sMacAddr bssid;
    uint8_t rx_snr;
    uint8_t blocked;   // True if response blocked.
    uint8_t broadcast; // True if broadcast probe.
} sSteeringEvProbeReq;

typedef struct {
    sSteeringEvProbeReq params;
} sACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION;

typedef struct {
    sMacAddr client_mac;
    sMacAddr bssid;
    uint8_t rx_snr;
    uint8_t reason;
    uint8_t blocked; // True if response blocked.
    uint8_t reject;  // True Auth Req is rejected by AP and Auth response is sent to STA with reason
} sSteeringEvAuthFail;

typedef struct {
    sSteeringEvAuthFail params;
} sACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_NOTIFICATION;

typedef struct {
    sMacAddr mac;
    sMacAddr bssid;
    beerocks::message::sRadioCapabilities capabilities;
    int8_t vap_id;
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t multi_ap_profile;
    size_t association_frame_length;
    uint8_t association_frame[beerocks::message::ASSOCIATION_MAX_LENGTH];
} sClientAssociationParams;

typedef struct {
    sMacAddr mac;
    int8_t vap_id;
    uint8_t reason;
    uint8_t source;
    uint8_t type;
} sClientDisconnectionParams;

typedef struct {
    sClientDisconnectionParams params;
} sACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION;

typedef struct {
    sClientAssociationParams params;
} sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION;

typedef struct {
    sMacAddr mac;
    int8_t vap_id;
} sACTION_MONITOR_CLIENT_ASSOCIATED_NOTIFICATION;

typedef struct {
    sMacAddr mac;
} sACTION_MONITOR_CLIENT_DISCONNECTED_NOTIFICATION;

typedef struct {
    sMacAddr mac;
    sMacAddr source_bssid;
    sMacAddr target_bssid;
    uint8_t status_code;
} sNodeBssSteerResponse;

typedef struct {
    sNodeBssSteerResponse params;
} sACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE;

typedef struct {
    uint32_t timeout;
    uint32_t frequency;
    uint16_t center_frequency1;
    uint16_t center_frequency2;
    uint8_t success;
    uint8_t channel;
    uint8_t bandwidth;
    uint8_t reserved1;
} sDfsCacCompleted;

typedef struct {
    sDfsCacCompleted params;
} sACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION;

typedef struct {
    uint8_t channel;
    uint8_t secondary_channel;
    beerocks::eWiFiBandwidth bandwidth;
    uint16_t cac_duration_sec;
} sCacStartedNotificationParams;

typedef struct {
    sCacStartedNotificationParams params;
} sACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION;

typedef struct {
    uint32_t frequency;
    uint8_t channel;
    uint8_t bandwidth; //beerocks::eWiFiBandwidth
    uint16_t vht_center_frequency;
} sDfsChannelAvailable;

typedef struct {
    sDfsChannelAvailable params;
} sACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION;

typedef struct {
    int8_t vap_id;
} sHOSTAP_DISABLED_NOTIFICATION;

typedef struct {
    int8_t vap_id;
} sHOSTAP_ENABLED_NOTIFICATION;

#define SSID_MAX_SIZE beerocks::message::WIFI_SSID_MAX_LENGTH
// ASSOCIATION_MAX_LENGTH is defined to be the maximum of the binary frame
// but since it is represented as hex here. we need two hex characters per byte + a terminating \0.
#define ASSOCIATION_FRAME_SIZE (2 * beerocks::message::ASSOCIATION_MAX_LENGTH + 1)
#define MAC_ADDR_SIZE 18
constexpr size_t MAX_TEMP_BUFFER_SIZE = 64;
constexpr char BSS_IFNAME_PREFIX[]    = "wlan";

/**
 * @brief Supported 802.11 management frame types.
 */
enum class eManagementFrameType {
    ASSOCIATION_REQUEST   = 0x00, /**< Association Request */
    REASSOCIATION_REQUEST = 0x01, /**< Re-association Request */
    BTM_QUERY             = 0x02, /**< BSS transition query */
    WNM_REQUEST           = 0x03, /**< 802.11v transition request */
    ANQP_REQUEST          = 0x04  /**< Access Network Query Protocol request */
};

/**
 * @brief 802.11 management frame notification event.
 * 
 * This structure contains the payload of a 802.11 management frame received
 * from a station. This frame can analyzed in the agent or tunnelled to the controller.
 */
struct sMGMT_FRAME_NOTIFICATION {
    sMacAddr mac;              /**< The MAC address of the station */
    sMacAddr bssid;            /**< The MAC address of the AP */
    eManagementFrameType type; /**< The type of the management frame */
    std::vector<uint8_t> data; /**< Frame body */
};

/**
 * @brief 802.11 management frame notification event.
 * 
 * This structure contains the payload of a 802.11 management frame received
 * from a station. This frame can analyzed in the agent or tunnelled to the controller.
 */
struct sSTA_MISMATCH_PSK {
    sMacAddr sta_mac; /**< The MAC address of the station */
};
} // namespace bwl

#endif // _BWL_BASE_WLAN_HAL_TYPES_H_
