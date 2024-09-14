/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _CLIENT_ASSOCIATION_TASK_H_
#define _CLIENT_ASSOCIATION_TASK_H_

#include "../db/db.h"
#include "task.h"
#include "task_pool.h"

namespace son {

class client_association_task : public task {
public:
    client_association_task(db &database_, ieee1905_1::CmduMessageTx &cmdu_tx_, task_pool &tasks_,
                            const std::string &task_name_ = std::string("client_association_task"));
    bool handle_ieee1905_1_msg(const sMacAddr &src_mac,
                               ieee1905_1::CmduMessageRx &cmdu_rx) override;

protected:
    void work() override;

private:
    db &m_database;
    ieee1905_1::CmduMessageTx &m_cmdu_tx;
    task_pool &m_tasks;

    /**
     * @brief If STA associate first time send Client Capability Query message.
     * Save mac address and timestamp of associated STA.
     * 
     * @param src_mac MAC address of Agent which reported new client association.
     * @param cmdu_rx TOPOLOGY_NOTIFICATION_MESSAGE.
     * @return True if client's associated first time, false otherwise.
     */
    bool verify_sta_association(const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Fill up Client Info TLV with all needed information, add this TLV
     * to the Client Capability Query Message, send this message to the Agent.
     * 
     * @param src_mac MAC address of Agent which reported new client association.
     * @param cmdu_rx TOPOLOGY_NOTIFICATION_MESSAGE.
     * @return True if message was successfully sent, false otherwise.
     */
    bool send_sta_capability_query(const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx);

    bool handle_cmdu_1905_client_capability_report_message(const sMacAddr &src_mac,
                                                           ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Adds AssociationEventData data model object
     * 
     * @param src_mac STA MAC.
     * @param bssid BSSID of BSS with which STA associated.
     * @return True on success, false otherwise.
     */
    bool dm_add_sta_association_event(const sMacAddr &sta_mac, const sMacAddr &bssid);

    /**
     * @brief Creates additional sub-objects STA HT(VHT)Capabilities
     * to AssociationEventData data model object.
     *
     * @param src_mac STA MAC.
     * @param bssid BSSID of BSS with which STA associated.
     * @return True on success, false otherwise.
     */
    bool dm_add_sta_association_event_caps(const sMacAddr &sta_mac, const sMacAddr &bssid);
};

} // namespace son

#endif
