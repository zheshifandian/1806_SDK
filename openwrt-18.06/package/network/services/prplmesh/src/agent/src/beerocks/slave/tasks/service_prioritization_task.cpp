/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "service_prioritization_task.h"
#include "../agent_db.h"
#include "../son_slave_thread.h"

#include <tlvf/wfa_map/tlvDscpMappingTable.h>
#include <tlvf/wfa_map/tlvProfile2ErrorCode.h>

namespace beerocks {

ServicePrioritizationTask::ServicePrioritizationTask(slave_thread &btl_ctx,
                                                     ieee1905_1::CmduMessageTx &cmdu_tx)
    : Task(eTaskType::SERVICE_PRIORITIZATION), m_btl_ctx(btl_ctx), m_cmdu_tx(cmdu_tx)
{
}

bool ServicePrioritizationTask::handle_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx,
                                            uint32_t iface_index, const sMacAddr &dst_mac,
                                            const sMacAddr &src_mac, int fd,
                                            std::shared_ptr<beerocks_header> beerocks_header)
{
    switch (cmdu_rx.getMessageType()) {
    case ieee1905_1::eMessageType::SERVICE_PRIORITIZATION_REQUEST_MESSAGE: {
        handle_service_prioritization_request(cmdu_rx, src_mac);
        break;
    }
    default: {
        // Message was not handled, therefore return false.
        return false;
    }
    }
    return true;
}

void ServicePrioritizationTask::handle_service_prioritization_request(
    ieee1905_1::CmduMessageRx &cmdu_rx, const sMacAddr &src_mac)
{
    const auto mid = cmdu_rx.getMessageId();

    LOG(DEBUG) << "Received SERVICE_PRIORITIZATION_REQUEST_MESSAGE, mid=" << std::hex << mid;

    m_cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE);

    LOG(DEBUG) << "sending ACK message back to controller";
    m_btl_ctx.send_cmdu_to_controller({}, m_cmdu_tx);

    auto service_prioritization_rules =
        cmdu_rx.getClassList<wfa_map::tlvServicePrioritizationRule>();

    // Split rules to lists of rules to remove and rules to add.
    // If rule is being added, but it already exist only overwite it.
    std::vector<std::shared_ptr<wfa_map::tlvServicePrioritizationRule>> rules_to_remove;
    std::vector<std::shared_ptr<wfa_map::tlvServicePrioritizationRule>> rules_to_add;
    auto db = AgentDB::get();
    for (auto &rule : service_prioritization_rules) {
        // Remove
        if (!rule->rule_params().bits_field1.add_remove) {
            rules_to_remove.push_back(rule);
            continue;
        }

        auto rule_found_it = db->service_prioritization.rules.find(rule->rule_params().id);

        // Overwrite existing rule
        if (rule_found_it != db->service_prioritization.rules.end()) {
            rule_found_it->second = rule->rule_params();
            continue;
        }

        // Rule to Add
        rules_to_add.push_back(rule);
    }

    // Prepare error response message, in case we will need to fill it.
    if (!m_cmdu_tx.create(0, ieee1905_1::eMessageType::ERROR_RESPONSE_MESSAGE)) {
        LOG(ERROR) << "CMDU creation has failed";
        return;
    }

    for (const auto &rule_to_remove : rules_to_remove) {
        auto rule_found_it =
            db->service_prioritization.rules.find(rule_to_remove->rule_params().id);
        if (rule_found_it != db->service_prioritization.rules.end()) {
            db->service_prioritization.rules.erase(rule_found_it);
            continue;
        }
        // If we were asked to remove a rule we don't have, add Profile-2 Error Code TLV.
        auto profile2_error_code_tlv = m_cmdu_tx.addClass<wfa_map::tlvProfile2ErrorCode>();
        if (!profile2_error_code_tlv) {
            LOG(ERROR) << "addClass has failed";
            return;
        }
        profile2_error_code_tlv->reason_code() =
            wfa_map::tlvProfile2ErrorCode::eReasonCode::SERVICE_PRIORITIZATION_RULE_NOT_FOUND;
        profile2_error_code_tlv->set_service_prioritization_rule_id(
            rule_to_remove->rule_params().id);
    }

    for (const auto &rule_to_add : rules_to_add) {
        // '1' is the current maximum allowed ruled specified in the
        // tlvProfile2ApCapability::max_prioritization_rules
        if (db->service_prioritization.rules.size() >= db->device_conf.max_prioritization_rules) {
            auto profile2_error_code_tlv = m_cmdu_tx.addClass<wfa_map::tlvProfile2ErrorCode>();
            if (!profile2_error_code_tlv) {
                LOG(ERROR) << "addClass has failed";
                return;
            }
            profile2_error_code_tlv->reason_code() = wfa_map::tlvProfile2ErrorCode::
                NUMBER_OF_SERVICE_PRIORITIZATION_RULES_EXCEEDED_THE_MAXIMUM_SUPPORTED;
            profile2_error_code_tlv->set_service_prioritization_rule_id(
                rule_to_add->rule_params().id);
            break;
        }

        db->service_prioritization.rules[rule_to_add->rule_params().id] =
            rule_to_add->rule_params();
    }

    // If added Profile2ErrorCode TLVs, send the ERROR_RESPONSE_MESSAGE.
    if (m_cmdu_tx.getClass<wfa_map::tlvProfile2ErrorCode>()) {
        m_btl_ctx.send_cmdu_to_controller({}, m_cmdu_tx);
    }

    auto dscp_mapping_table_tlv = cmdu_rx.getClass<wfa_map::tlvDscpMappingTable>();
    if (dscp_mapping_table_tlv) {
        auto dscp_mapping_table = dscp_mapping_table_tlv->dscp_pcp_mapping(0);
        std::copy(dscp_mapping_table, dscp_mapping_table + 64,
                  db->service_prioritization.dscp_mapping_table.begin());
    }

    // TODO: Configure Rules (PPM-1874).
}

} // namespace beerocks
