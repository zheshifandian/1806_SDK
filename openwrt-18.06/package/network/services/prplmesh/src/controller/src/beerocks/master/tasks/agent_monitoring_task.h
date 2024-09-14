/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _AGENT_MONITORING_TASK_H_
#define _AGENT_MONITORING_TASK_H_

#include "../db/db.h"
#include "task.h"
#include "task_pool.h"

#include <tlvf/WSC/m1.h>
#include <tlvf/wfa_map/tlvApOperationalBSS.h>
#include <tlvf/wfa_map/tlvAssociatedClients.h>

namespace son {
class agent_monitoring_task : public task {

public:
    agent_monitoring_task(db &database_, ieee1905_1::CmduMessageTx &cmdu_tx_, task_pool &tasks_,
                          const std::string &task_name_ = std::string("agent_monitoring"));
    bool handle_ieee1905_1_msg(const sMacAddr &src_mac,
                               ieee1905_1::CmduMessageRx &cmdu_rx) override;

    static bool add_profile_2default_802q_settings_tlv(db &database,
                                                       ieee1905_1::CmduMessageTx &cmdu_tx,
                                                       std::shared_ptr<WSC::m1> m1);
    static bool add_traffic_policy_tlv(db &database, ieee1905_1::CmduMessageTx &cmdu_tx,
                                       std::shared_ptr<WSC::m1> m1);

protected:
    void work() override;
    virtual void handle_event(int event_type, void *obj) override;

private:
    db &database;
    ieee1905_1::CmduMessageTx &cmdu_tx;
    task_pool &tasks;

    struct sBssStats {
        uint32_t unicast_bytes_sent;
        uint32_t unicast_bytes_received;
        uint32_t multicast_bytes_sent;
        uint32_t multicast_bytes_received;
        uint32_t broadcast_bytes_sent;
        uint32_t broadcast_bytes_received;
    };

    struct sRadioStats {
        uint8_t utilization;
        uint8_t transmit;
        uint8_t receive_self;
        uint8_t receive_other;
        uint8_t noise;
    };

    /**
    * key = BSSID, value = latest BSS statistics
    */
    beerocks::mac_map<sBssStats> m_bss_stats;

    /**
    * key = RUID, value = latest Radio statistics
    */
    beerocks::mac_map<sRadioStats> m_radio_stats;

    bool m_ap_autoconfig_renew_sent = false;

    /**
     * @brief Key = Agents mac, value = list with paths to AgentConnectedEvent
     * NBAPI object.
     */
    std::unordered_map<sMacAddr, std::queue<std::string>> m_agents;

    /**
     * @brief Queue with paths to NBAPI AgentConnectedEvent objects.
     */
    std::queue<std::string> m_disconnected;

    /**
     * @brief Map with key=ruid and value BSSes that were configured
     * for radio with ruid (key) in M2.
     */
    std::unordered_map<sMacAddr, std::list<wireless_utils::sBssInfoConf>> m_bss_configured;

    /*
    * The maximum amount of NBAPI AgentConnected (Disconnected) events per one Agent.
    */
    const uint8_t MAX_EVENT_HISTORY_SIZE = 7;

    /**
     * @brief Recive Topology Response message, checks that Agent configured BSSes
     * reported in M2 message. If it's so agent monitoring should start.
     * @param src_mac MAC address of agent.
     * @param cmdu_rx is a Topology Response message.
     */
    bool start_agent_monitoring(const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
    * @brief Sends topology query, policy configuration,
    * AP capability query, start dynamic channel selection task.
    *
    * @param mac MAC address of agent.
    * @param m1 M1 message.
    * @param cmdu_rx AP Autoconfiguration WSC message.
    */
    bool start_task(const sMacAddr &src_mac, std::shared_ptr<WSC::m1> m1,
                    ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Sends 'CHANNEL_SELECTION_REQUEST_MESSAGE' without any TLVs included,
     * to get Operating Channel Report back from agents.
     *
     * If a Channel Selection Request message contains zero Channel Preference TLVs,
     * it is indicating the highest preference for all channels and operating classes supported
     * by all of the Multi-AP Agent’s radios.
     *
     * Operating Channel Report should be sent back from the Agent after this empty message.
     * That way operating classes are registered to data model.
     *
     * @param dst_mac Destination MAC address.
     * @param cmdu_tx CMDU to be transmitted.
     * @return True on success, false otherwise.
    */
    bool send_tlv_empty_channel_selection_request(const sMacAddr &dst_mac,
                                                  ieee1905_1::CmduMessageTx &cmdu_tx);

    /**
     * @brief Sends Tlv metric resporting policy within 'MULTI_AP_POLICY_CONFIG_REQUEST_MESSAGE'
     *
     * @param dst_mac Destination MAC address.
     * @param m1 M1.
     * @param cmdu_rx AP AUTOCONFIGURATION WSC MESSAGE.
     * @param cmdu_tx CMDU to be transmitted.
     * @return True on success, false otherwise.
    */
    bool send_tlv_metric_reporting_policy(const sMacAddr &dst_mac, std::shared_ptr<WSC::m1> m1,
                                          ieee1905_1::CmduMessageRx &cmdu_rx,
                                          ieee1905_1::CmduMessageTx &cmdu_tx);

    /**
     * @brief Add NBAPI AgentConnected event and its sub-objects: Radios, BSSes, STAs to data model.
     *
     * @param device_mac Mac address of Agent for which AgentConnected event will be created.
     * @param ap_op_bss_tlv AP Operational BSS TLV.
     ​* @return Path to NBAPI AgentConnected object, empty string otherwise.
    ​ */
    std::string
    dm_add_agent_connected_event(const sMacAddr &device_mac,
                                 std::shared_ptr<wfa_map::tlvApOperationalBSS> &ap_op_bss_tlv,
                                 ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Store in 'bss_stats' last received BSS statistics.
     * @param cmdu_rx The AP Metrics Response message.
     */
    void save_bss_statistics(ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Store in 'radio_stats' last received radio statistics.
     * @param src_mac Source mac.
     * @param cmdu_rx The AP Metrics Response message.
     */
    void save_radio_statistics(const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Search for appropriate BSS from bss list of tlvAssociatedClients,
     * for each STA from its list of clients create NBAPI
     * STA object and attach this object to AgentConnected.
     *
     * @param obj_path Path to object to which STA will be attached.
     * @param bssid ​BSSID to search in the list.
     * @param assoc_client_tlv tlvAssociatedClients.
    ​ */
    void dm_add_sta_to_agent_connected_event(
        const std::string &obj_path, const sMacAddr &bssid,
        std::shared_ptr<wfa_map::tlvAssociatedClients> &assoc_client_tlv);

    /**
     * @brief Add Neighbor of AgentConnected event to NBAPI data model.
     * Ex. Device.WiFi.DataElements.AgentConnectedEvent.AgentConnected.1.Neighbor.3
     *
     * @param event_path Path to AgentConnected event to which neighbor will be added.
     * @param cmdu_rx Topology Response message.
     * @return True on success, false otherwise.
     */
    bool dm_add_neighbor_to_agent_connected_event(const std::string &event_path,
                                                  ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Adds NBAPI AgentDisconnectedEvent object, all its sub-objects and set values
     * for it's parameters parameters.
     * @param src_mac MAC address of agent.
     * @return True on success, false otherwise.
     */
    bool dm_add_agent_disconnected_event(const sMacAddr &agent_mac);

    /**
     * @brief Set values for parameters of NBAPI AgentDisconnected, adds its sub-objects.
     * @param agent_discon_path Path to NBAPI AgentDisconnectedEvent object.
     * @param src_mac MAC address of agent.
     * @return True on success, false otherwise.
     */
    bool dm_set_agent_disconnected_event_params(const std::string &agent_discon_path,
                                                const sMacAddr &agent_mac);
};

} // namespace son

#endif
