/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef AGENT_H
#define AGENT_H

#include <bcl/beerocks_defines.h>
#include <bcl/beerocks_mac_map.h>
#include <memory>
#include <string>
#include <tlvf/common/sMacAddr.h>
#include <tlvf/tlvftypes.h>
#include <tlvf/wfa_map/tlvProfile2ApCapability.h>
#include <tlvf/wfa_map/tlvProfile2MultiApProfile.h>

// Forward declaration of son::node
namespace son {
class node;
class db;
} // namespace son

namespace prplmesh {
namespace controller {
namespace db {

class Station;

/** All information about an agent in the controller database. */
class Agent {
public:
    Agent()              = delete;
    Agent(const Agent &) = delete;
    explicit Agent(const sMacAddr &al_mac_) : al_mac(al_mac_) {}

    /** AL-MAC address of the agent. */
    const sMacAddr al_mac;

    std::string dm_path; /**< data model path */

    /**
     * @brief Agents supported profile information.
     *
     * Default is MULTIAP_PROFILE_1.
     *
     * This parameter is reported with Profile-2 Multi AP Profile TLV.
     */
    wfa_map::tlvProfile2MultiApProfile::eMultiApProfile profile =
        wfa_map::tlvProfile2MultiApProfile::eMultiApProfile::MULTIAP_PROFILE_1;

    /**
     * @brief Byte counters unit types related TLVs.
     *
     * Multi-AP Agent onboards to a Controller created with Profile-1 settings and
     * byte counters unit is set to BYTES.
     *
     * This affects counters in AP Extended Metrics and Associated STA Traffic Stats TLV.
     *
     * This parameter is reported with Profile-2 AP Capability TLV.
     */
    wfa_map::tlvProfile2ApCapability::eByteCounterUnits byte_counter_units =
        wfa_map::tlvProfile2ApCapability::eByteCounterUnits::BYTES;

    /**
     * @brief Max Total Number of unique VLAN identifiers the Multi-AP Agent supports.
     *
     * This parameter is reported with Profile-2 AP Capability TLV.
     * For Profile-1, it is initialized as zero due to not supporting the TLV.
     */
    uint8_t max_total_number_of_vids = 0;

    bool is_gateway = false;

    std::string manufacturer;

    beerocks::eNodeState state = beerocks::STATE_CONNECTED;

    struct sRadio {
        sRadio()               = delete;
        sRadio(const sRadio &) = delete;
        explicit sRadio(const sMacAddr &radio_uid_) : radio_uid(radio_uid_) {}

        /** Radio UID. */
        const sMacAddr radio_uid;

        std::string dm_path; /**< data model path */

        bool is_acs_enabled = false;

        class s_ap_stats_params {
        public:
            int active_sta_count                 = 0;
            uint32_t rx_packets                  = 0;
            uint32_t tx_packets                  = 0;
            uint32_t rx_bytes                    = 0;
            uint32_t tx_bytes                    = 0;
            uint32_t errors_sent                 = 0;
            uint32_t errors_received             = 0;
            uint32_t retrans_count               = 0;
            int8_t noise                         = 0;
            uint8_t channel_load_percent         = 0;
            uint8_t total_client_tx_load_percent = 0;
            uint8_t total_client_rx_load_percent = 0;
            uint16_t stats_delta_ms              = 0;
            std::chrono::steady_clock::time_point timestamp;
        };
        std::shared_ptr<s_ap_stats_params> stats_info;

        struct sBss {
            sBss()             = delete;
            sBss(const sBss &) = delete;
            explicit sBss(const sMacAddr &bssid_, sRadio &radio_,
                          int vap_id_ = beerocks::eBeeRocksIfaceIds::IFACE_ID_INVALID)
                : bssid(bssid_), radio(radio_), vap_id(vap_id_)
            {
            }

            /** BSSID of the BSS. */
            const sMacAddr bssid;
            sRadio &radio;

            /**
             * @brief VAP ID.
             *
             * Only exists on prplmesh devices. -1 if not set.
             */
            const int vap_id;

            /** SSID of the BSS - empty if unconfigured. */
            std::string ssid;

            /** True if this is a backhaul bss. */
            bool backhaul = false;

            /** Stations (backhaul or fronthaul) connected to this BSS. */
            beerocks::mac_map<Station> connected_stations;
        };

        /** BSSes configured/reported on this radio. */
        beerocks::mac_map<sBss> bsses;
    };

    struct sBackhaul {
        // Local radio the backhaul station use. If `nullptr` the backhaul is wired.
        std::shared_ptr<Agent::sRadio> wireless_backhaul_radio;

        // The BSSID which the backhaul station connected to. Could be an AP or LAN interface.
        sMacAddr bssid;

        // The Agent node that the backhaul station is connected to.
        std::weak_ptr<Agent> parent_agent;
    } backhaul;

    /** Radios reported on this agent. */
    beerocks::mac_map<sRadio> radios;

    friend class ::son::db;

private:
    /**
     * @brief The last time that the Agent was contacted via the Multi-AP control protocol.
     */
    std::chrono::system_clock::time_point last_contact_time;
};

} // namespace db
} // namespace controller
} // namespace prplmesh

#endif // AGENT_H
