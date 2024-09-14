/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "tlvf_utils.h"

#include "agent_db.h"

#include <bcl/son/son_wireless_utils.h>
#include <easylogging++.h>

#include <tlvf/wfa_map/tlvApRadioBasicCapabilities.h>

using namespace beerocks;

/**
 * @brief Get a list of supported operating classes.
 *
 * @param channels_list List of supported channels.
 * @return std::vector<uint8_t> A vector of supported operating classes.
 */
static std::vector<uint8_t> get_supported_operating_classes(
    const std::unordered_map<uint8_t, beerocks::AgentDB::sRadio::sChannelInfo> &channels_list)
{
    std::vector<uint8_t> operating_classes;
    //TODO handle regulatory domain operating classes
    for (const auto &oper_class : son::wireless_utils::operating_classes_list) {
        for (const auto &channel_info_element : channels_list) {
            auto channel       = channel_info_element.first;
            auto &channel_info = channel_info_element.second;
            bool found         = false;
            for (const auto &bw_info : channel_info.supported_bw_list) {
                if (son::wireless_utils::has_operating_class_channel(oper_class.second, channel,
                                                                     bw_info.bandwidth)) {
                    operating_classes.push_back(oper_class.first);
                    found = true;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
    }
    return operating_classes;
}

/**
 * @brief Get the maximum transmit power of operating class.
 *
 * @param channels_list List of supported channels.
 * @param operating_class Operating class to find max tx for.
 * @return Max tx power for requested operating class.
 */
static int8_t get_operating_class_max_tx_power(
    const std::unordered_map<uint8_t, beerocks::AgentDB::sRadio::sChannelInfo> &channels_list,
    uint8_t operating_class)
{
    int8_t max_tx_power = 0;
    auto oper_class_it  = son::wireless_utils::operating_classes_list.find(operating_class);
    if (oper_class_it == son::wireless_utils::operating_classes_list.end()) {
        LOG(ERROR) << "Operating class does not exist: " << operating_class;
        return beerocks::eGlobals::RSSI_INVALID;
    }
    const auto &oper_class = oper_class_it->second;

    for (const auto &channel_info_element : channels_list) {
        auto channel       = channel_info_element.first;
        auto &channel_info = channel_info_element.second;
        for (const auto &bw_info : channel_info.supported_bw_list) {
            if (son::wireless_utils::has_operating_class_channel(oper_class, channel,
                                                                 bw_info.bandwidth)) {
                max_tx_power = std::max(max_tx_power, channel_info.tx_power_dbm);
            }
        }
    }
    return max_tx_power;
}

/**
 * @brief Get a list of permanent non operable channels for operating class.
 *
 * @param channels_list List of supported channels.
 * @param operating_class Operating class to find non operable channels on.
 * @return std::vector<uint8_t> A vector of non operable channels.
 */
std::vector<uint8_t> get_operating_class_non_oper_channels(
    const std::unordered_map<uint8_t, beerocks::AgentDB::sRadio::sChannelInfo> &channels_list,
    uint8_t operating_class)
{
    std::vector<uint8_t> non_oper_channels;
    auto oper_class_it = son::wireless_utils::operating_classes_list.find(operating_class);
    if (oper_class_it == son::wireless_utils::operating_classes_list.end()) {
        LOG(ERROR) << "Operating class does not exist: " << operating_class;
        return {};
    }
    const auto &oper_class = oper_class_it->second;

    for (const auto &op_class_channel : oper_class.channels) {
        bool found = false;
        for (const auto &channel_info_element : channels_list) {
            auto &channel_info = channel_info_element.second;
            for (const auto &bw_info : channel_info.supported_bw_list) {
                auto channel = channel_info_element.first;
                if (operating_class == 128 || operating_class == 129 || operating_class == 130) {
                    channel =
                        son::wireless_utils::get_5g_center_channel(channel, bw_info.bandwidth);
                }
                if (op_class_channel == channel && oper_class.band == bw_info.bandwidth) {
                    found = true;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        if (!found) {
            non_oper_channels.push_back(op_class_channel);
        }
    }
    return non_oper_channels;
}

bool tlvf_utils::add_ap_radio_basic_capabilities(ieee1905_1::CmduMessageTx &cmdu_tx,
                                                 const sMacAddr &ruid)
{
    std::vector<uint8_t> operating_classes;

    auto radio_basic_caps = cmdu_tx.addClass<wfa_map::tlvApRadioBasicCapabilities>();
    if (!radio_basic_caps) {
        LOG(ERROR) << "Error creating TLV_AP_RADIO_BASIC_CAPABILITIES";
        return false;
    }
    radio_basic_caps->radio_uid() = ruid;

    auto db    = AgentDB::get();
    auto radio = db->get_radio_by_mac(ruid);
    if (!radio) {
        LOG(ERROR) << "ruid not found: " << ruid;
        return false;
    }

    int num_bsses = std::count_if(radio->front.bssids.begin(), radio->front.bssids.end(),
                                  [](beerocks::AgentDB::sRadio::sFront::sBssid b) {
                                      return b.mac != net::network_utils::ZERO_MAC;
                                  });
    LOG(DEBUG) << "Radio reports " << num_bsses << " BSSes.";

    radio_basic_caps->maximum_number_of_bsss_supported() = num_bsses;
    operating_classes = get_supported_operating_classes(radio->channels_list);
    LOG(DEBUG) << "Filling Supported operating classes on radio " << radio->front.iface_name << ":";

    for (auto op_class : operating_classes) {
        auto operationClassesInfo = radio_basic_caps->create_operating_classes_info_list();
        if (!operationClassesInfo) {
            LOG(ERROR) << "Failed creating operating classes info list";
            return false;
        }

        operationClassesInfo->operating_class() = op_class;
        operationClassesInfo->maximum_transmit_power_dbm() =
            get_operating_class_max_tx_power(radio->channels_list, op_class);

        auto non_oper_channels =
            get_operating_class_non_oper_channels(radio->channels_list, op_class);
        if (!non_oper_channels.empty()) {
            // Create list of statically non-oper channels
            operationClassesInfo->alloc_statically_non_operable_channels_list(
                non_oper_channels.size());
            uint8_t idx = 0;
            for (auto non_oper : non_oper_channels) {
                *operationClassesInfo->statically_non_operable_channels_list(idx) = non_oper;
                idx++;
            }
        }

        LOG(DEBUG) << "OpClass=" << op_class
                   << ", max_tx_dbm=" << operationClassesInfo->maximum_transmit_power_dbm()
                   << ", non_operable_channels=" << [&](std::vector<uint8_t>) {
                          std::string out;
                          for (auto non_oper_ch : non_oper_channels) {
                              out.append(std::to_string(non_oper_ch)).append(",");
                          }
                          out.pop_back();
                          return out;
                      }(non_oper_channels);

        if (!radio_basic_caps->add_operating_classes_info_list(operationClassesInfo)) {
            LOG(ERROR) << "add_operating_classes_info_list failed";
            return false;
        }
    }

    return true;
}
