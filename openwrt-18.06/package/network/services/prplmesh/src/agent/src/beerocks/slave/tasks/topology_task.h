/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _TOPOLOGY_TASK_H_
#define _TOPOLOGY_TASK_H_

#include "task.h"

#include <tlvf/CmduMessageTx.h>

namespace beerocks {

// Forward declaration for BackhaulManager context saving
class BackhaulManager;

class TopologyTask : public Task {
public:
    TopologyTask(BackhaulManager &btl_ctx, ieee1905_1::CmduMessageTx &cmdu_tx);
    ~TopologyTask() {}

    void work() override;

    enum eEvent : uint8_t {
        AGENT_RADIO_STATE_CHANGED,
        AGENT_DEVICE_INITIALIZED,
    };

    void handle_event(uint8_t event_enum_value, const void *event_obj) override;

    bool handle_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx, uint32_t iface_index,
                     const sMacAddr &dst_mac, const sMacAddr &src_mac, int fd,
                     std::shared_ptr<beerocks_header> beerocks_header) override;

private:
    /* 1905.1 message handlers: */

    /**
    * @brief Handles 1905 Topology Discovery message.
    * 
    * @param[in] cmdu_rx Received CMDU.
    * @param iface_index Index of the network interface that the CMDU message was received on.
    * @param dst_mac Destination MAC address.
    * @param[in] src_mac MAC address of the message sender.
    */
    void handle_topology_discovery(ieee1905_1::CmduMessageRx &cmdu_rx, uint32_t iface_index,
                                   const sMacAddr &dst_mac, const sMacAddr &src_mac);

    /**
    * @brief Handles 1905 Topology Query message.
    * 
    * @param[in] cmdu_rx Received CMDU.
    * @param[in] src_mac MAC address of the message sender.
    */
    void handle_topology_query(ieee1905_1::CmduMessageRx &cmdu_rx, const sMacAddr &src_mac);

    /**
     * @brief Handles Vendor Specific messages. 
     * 
     * @param[in] cmdu_rx Received CMDU.
     * @param[in] src_mac MAC address of the message sender.
     * @param[in] beerocks_header Shared pointer to beerocks header.
     * @return true, if the message has been handled, otherwise false.
     */
    bool handle_vendor_specific(ieee1905_1::CmduMessageRx &cmdu_rx, const sMacAddr &src_mac,
                                std::shared_ptr<beerocks_header> beerocks_header);

    /* Vendor specific message handlers: */

    /**
     * @brief Handles Vendor Specific Client Associated message. 
     * 
     * @param[in] cmdu_rx Received CMDU.
     * @param[in] beerocks_header Shared pointer to beerocks header.
     */
    void handle_vs_client_associated(ieee1905_1::CmduMessageRx &cmdu_rx,
                                     std::shared_ptr<beerocks_header> beerocks_header);

    /**
     * @brief Handles Vendor Specific Client Disassociated message. 
     * 
     * @param[in] cmdu_rx Received CMDU.
     * @param[in] beerocks_header Shared pointer to beerocks header.
     */
    void handle_vs_client_disassociated(ieee1905_1::CmduMessageRx &cmdu_rx,
                                        std::shared_ptr<beerocks_header> beerocks_header);

    /* Helper functions */
    void send_topology_discovery();
    void send_topology_notification();

    /**
     * @brief Add and fill device information tlv.
     * 
     * @return true on success, otherwise false.
     */
    bool add_device_information_tlv();

    /**
     * @brief Add and fill 1905_neighbor_device tlv for each know neighbor.
     * 
     * @return true on success, otherwise false.
     */
    bool add_1905_neighbor_device_tlv();

    /**
     * @brief Add and fill supported service tlv.
     * 
     * @return true on success, otherwise false.
     */
    bool add_supported_service_tlv();

    /**
     * @brief Add and fill AP operational BSS tlv.
     * 
     * @return true on success, otherwise false.
     */
    bool add_ap_operational_bss_tlv();

    /**
     * @brief Add and fill associated_clients tlv.
     * 
     * @return true on success, otherwise false.
     */
    bool add_associated_clients_tlv();

    std::chrono::steady_clock::time_point m_periodic_discovery_timestamp;

    bool m_pending_to_send_topology_notification = false;
    std::chrono::steady_clock::time_point m_topology_notification_timeout =
        std::chrono::steady_clock::now();

    BackhaulManager &m_btl_ctx;
    ieee1905_1::CmduMessageTx &m_cmdu_tx;
};

} // namespace beerocks

#endif // _TOPOLOGY_TASK_H_
