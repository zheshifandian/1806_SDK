/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BWL_802_11_DEFS_H_
#define _BWL_802_11_DEFS_H_

#include <tlvf/common/sMacAddr.h>

#include <stdint.h>

namespace bwl {

/**
 * @brief 802.11 management frame
 * More info here: https://en.wikipedia.org/wiki/802.11_Frame_Types
 */
struct s80211MgmtFrame {

    /**
     * @brief Management frame types
     */
    enum class eType : uint8_t {
        ASSOC_REQ    = 0,  /*<< Association Request */
        ASSOC_RESP   = 1,  /*<< Association Response */
        REASSOC_REQ  = 2,  /*<< Re-association Request */
        REASSOC_RESP = 3,  /*<< Re-association Response */
        PROBE_REQ    = 4,  /*<< Probe Request */
        PROBE_RESP   = 5,  /*<< Probe Response */
        BEACON       = 8,  /*<< Beacon */
        ATIM         = 9,  /*<< Announcement Traffic Indication Message */
        DISASSOC     = 10, /*<< Disassociation */
        AUTH         = 11, /*<< Authentication */
        DEAUTH       = 12, /*<< De-authentication */
        ACTION       = 13  /*<< Action */
    };

    // Header
    struct sHeader {
        // Frame Control
        union uFrameControl {
            uint16_t val; // Raw 16-bit value

            // Bitfield
            struct sFrameControl {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                uint16_t protocol_version : 2; // Protocol version (currently is 0)
                uint16_t type : 2;             // Frame type (00 - Mgmt, 01 - Control, 10 - Data)
                uint16_t subtype : 4;          // Frame subtype (e.g. Assoc Req, Action etc.)
                uint16_t to_ds : 1;            // If destined to Distribution System
                uint16_t from_ds : 1;          // If originated from Distribution System
                uint16_t more_fragments : 1;   // If the frame is fragmented
                uint16_t retry : 1;            // Retransmission of the earlier frame
                uint16_t power_mgmt : 1;       // If power management is enabled
                uint16_t more_data : 1;        // TBD
                uint16_t protected_frame : 1;  // If the data is encrypted
                uint16_t order : 1;            // QoS
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                uint16_t subtype : 4;          // Frame subtype (e.g. Assoc Req, Action etc.)
                uint16_t type : 2;             // Frame type (00 - Mgmt, 01 - Control, 10 - Data)
                uint16_t protocol_version : 2; // Protocol version (currently is 0)
                uint16_t order : 1;            // QoS
                uint16_t protected_frame : 1;  // If the data is encrypted
                uint16_t more_data : 1;        // TBD
                uint16_t power_mgmt : 1;       // If power management is enabled
                uint16_t retry : 1;            // Retransmission of the earlier frame
                uint16_t more_fragments : 1;   // If the frame is fragmented
                uint16_t from_ds : 1;          // If originated from Distribution System
                uint16_t to_ds : 1;            // If destined to Distribution System
#endif
            } __attribute__((packed)) bits;
        } __attribute__((packed)) frame_control;

        uint16_t duration; // Duration / ID
        sMacAddr da;       // Destination MAC Address
        sMacAddr sa;       // Source MAC Address
        sMacAddr bssid;    // BSSID (AP MAC Address)
        uint16_t seq_ctrl; // Sequence number

    } __attribute__((packed)) header;

    union uBody {
        // Action Frame
        struct sAction {
            /**
             * @brief Action frame categories (IEEE Std 802.11-2016, 9.4.1.11, Table 9-76) 
             */
            enum class eCategory {
                SPECTRUM_MGMT             = 0,
                QOS                       = 1,
                DLS                       = 2,
                BLOCK_ACK                 = 3,
                PUBLIC                    = 4,
                RADIO_MEASUREMENT         = 5,
                FT                        = 6,
                HT                        = 7,
                SA_QUERY                  = 8,
                PROTECTED_DUAL            = 9,
                WNM                       = 10,
                UNPROTECTED_WNM           = 11,
                TDLS                      = 12,
                MESH                      = 13,
                MULTIHOP                  = 14,
                SELF_PROTECTED            = 15,
                DMG                       = 16,
                WMM                       = 17,
                FST                       = 18,
                ROBUST_AV_STREAMING       = 19,
                UNPROTECTED_DMG           = 20,
                VHT                       = 21,
                FILS                      = 26,
                VENDOR_SPECIFIC_PROTECTED = 126,
                VENDOR_SPECIFIC           = 127
                /* Note: 128-255 used to report errors by setting category | 0x80 */
            };

            /**
             * @brief 802.11 action frame codes
             * Listing only the codes that are currently used by prplMesh
             */
            enum class eCode {
                WNM_BSS_TRANS_MGMT_QUERY = 6,  /**< WNM BTM Query (WNM Category [10]) */
                ANQP_REQ                 = 10, /**< ANQP Request (Public Category [4]) */
                WNM_NOTIFICATION_REQ     = 26  /**< WNM Notification Request (WNM Category [10]) */
            };

            uint8_t category; // Action frame category
            uint8_t code;     // Action frame code

        } __attribute__((packed)) action;
    } __attribute__((packed)) body;
} __attribute__((packed));

/*
 * @brief channel width number from the standard IEEE 802.11-2020
 * (Table 9-175 HT/VHT Operation Information subfields).
 * (values 5-255 are reserved)
 */
enum eChanWidthNr : uint8_t {
    NR_CHAN_WIDTH_20    = 0,
    NR_CHAN_WIDTH_40    = 1,
    NR_CHAN_WIDTH_80    = 2,
    NR_CHAN_WIDTH_160   = 3,
    NR_CHAN_WIDTH_80P80 = 4,
    NR_CHAN_WIDTH_MAX,
    NR_CHAN_WIDTH_UNKNOWN = 0xFF,
};

#define BIT(x) (1U << (x))

/* HT Capabilities Info field within HT Capabilities element */
#define HT_CAP_INFO_LDPC_CODING_CAP ((uint16_t)BIT(0))
#define HT_CAP_INFO_SUPP_CHANNEL_WIDTH_SET ((uint16_t)BIT(1))
#define HT_CAP_INFO_SMPS_MASK ((uint16_t)(BIT(2) | BIT(3)))
#define HT_CAP_INFO_SMPS_STATIC ((uint16_t)0)
#define HT_CAP_INFO_SMPS_DYNAMIC ((uint16_t)BIT(2))
#define HT_CAP_INFO_SMPS_DISABLED ((uint16_t)(BIT(2) | BIT(3)))
#define HT_CAP_INFO_GREEN_FIELD ((uint16_t)BIT(4))
#define HT_CAP_INFO_SHORT_GI20MHZ ((uint16_t)BIT(5))
#define HT_CAP_INFO_SHORT_GI40MHZ ((uint16_t)BIT(6))
#define HT_CAP_INFO_TX_STBC ((uint16_t)BIT(7))
#define HT_CAP_INFO_RX_STBC_MASK ((uint16_t)(BIT(8) | BIT(9)))
#define HT_CAP_INFO_RX_STBC_1 ((uint16_t)BIT(8))
#define HT_CAP_INFO_RX_STBC_12 ((uint16_t)BIT(9))
#define HT_CAP_INFO_RX_STBC_123 ((uint16_t)(BIT(8) | BIT(9)))
#define HT_CAP_INFO_DELAYED_BA ((uint16_t)BIT(10))
#define HT_CAP_INFO_MAX_AMSDU_SIZE ((uint16_t)BIT(11))
#define HT_CAP_INFO_DSSS_CCK40MHZ ((uint16_t)BIT(12))
/* B13 - Reserved (was PSMP support during P802.11n development) */
#define HT_CAP_INFO_40MHZ_INTOLERANT ((uint16_t)BIT(14))
#define HT_CAP_INFO_LSIG_TXOP_PROTECT_SUPPORT	((uint16_t) BIT(15)

/* Radio Measurement capabilities (from RM Enabled Capabilities element)
 * IEEE Std 802.11-2016, 9.4.2.45, Table 9-157 */
/* byte 1 (out of 5) */
#define WLAN_RRM_CAPS_LINK_MEASUREMENT BIT(0)
#define WLAN_RRM_CAPS_NEIGHBOR_REPORT BIT(1)
#define WLAN_RRM_CAPS_BEACON_REPORT_PASSIVE BIT(4)
#define WLAN_RRM_CAPS_BEACON_REPORT_ACTIVE BIT(5)
#define WLAN_RRM_CAPS_BEACON_REPORT_TABLE BIT(6)
/* byte 2 (out of 5) */
#define WLAN_RRM_CAPS_CHANNEL_LOAD BIT(1)
#define WLAN_RRM_CAPS_NOISE_HISTOGRAM BIT(2)
#define WLAN_RRM_CAPS_STATISTICS_MEASUREMENT BIT(3)
#define WLAN_RRM_CAPS_LCI_MEASUREMENT BIT(4)
/* byte 5 (out of 5) */
#define WLAN_RRM_CAPS_FTM_RANGE_REPORT BIT(2)

} // namespace bwl

#endif // _BWL_802_11_DEFS_H_
