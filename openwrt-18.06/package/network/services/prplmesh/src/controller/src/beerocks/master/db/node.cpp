/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "node.h"

#include <easylogging++.h>

using namespace beerocks;
using namespace son;

node::node(beerocks::eType type_, const std::string &mac_)
    : mac(mac_), capabilities(&m_sta_24ghz_capabilities) // deafult value
{
    type                           = type_;
    m_sta_5ghz_capabilities.valid  = false;
    m_sta_24ghz_capabilities.valid = false;
}

namespace son {
std::ostream &operator<<(std::ostream &os, eTriStateBool value)
{
    if (value == eTriStateBool::FALSE) {
        os << "False";
    } else if (value == eTriStateBool::TRUE) {
        os << "True";
    } else {
        os << "Not-Configured";
    }
    return os;
}

} // namespace son

void node::clear_node_stats_info() { stats_info = std::make_shared<sta_stats_params>(); }

beerocks::eType node::get_type() { return type; }

bool node::set_type(beerocks::eType type_)
{
    //only allow TYPE_CLIENT to TYPE_IRE_BACKHAUL change
    if (type_ == type) {
        return true;
    } else if ((type == beerocks::TYPE_CLIENT) && (type_ == beerocks::TYPE_IRE_BACKHAUL)) {
        type = type_;
        return true;
    } else {
        LOG(ERROR) << "Not expected to happen: node = " << mac << ", old type = " << int(type)
                   << ", new type = " << int(type_);
    }
    return false;
}

bool node::link_metrics_data::add_transmitter_link_metric(
    std::shared_ptr<ieee1905_1::tlvTransmitterLinkMetric> tx_link_metric_data)
{
    //  interface_pair_info_length() returns the length in bytes (number of elements * sizeof(sInterfacePairInfo).
    size_t info_size = tx_link_metric_data->interface_pair_info_length() /
                       sizeof(ieee1905_1::tlvTransmitterLinkMetric::sInterfacePairInfo);

    for (size_t i = 0; i < info_size; i++) {
        auto info_tuple = tx_link_metric_data->interface_pair_info(i);

        if (!std::get<0>(info_tuple)) {
            LOG(ERROR) << "add_transmitter_link_metric getting operating class entry has failed!";
            return false;
        }
        auto &InterfacePairInfo = std::get<1>(info_tuple);
        transmitterLinkMetrics.push_back(InterfacePairInfo);

        LOG(DEBUG) << "adding tlvTransmitterLinkMetric data to list"
                   << " phy_rate = " << int(InterfacePairInfo.link_metric_info.phy_rate);
    }
    return true;
}

bool node::link_metrics_data::add_receiver_link_metric(
    std::shared_ptr<ieee1905_1::tlvReceiverLinkMetric> RxLinkMetricData)
{
    //  interface_pair_info_length() returns the length in bytes (number of elements * sizeof(sInterfacePairInfo).
    size_t info_size = RxLinkMetricData->interface_pair_info_length() /
                       sizeof(ieee1905_1::tlvReceiverLinkMetric::sInterfacePairInfo);

    for (size_t i = 0; i < info_size; i++) {
        auto info_tuple = RxLinkMetricData->interface_pair_info(i);

        if (!std::get<0>(info_tuple)) {
            LOG(ERROR) << "add_receiver_link_metric getting operating class entry has failed!";
            return false;
        }
        auto &InterfacePairInfo = std::get<1>(info_tuple);
        receiverLinkMetrics.push_back(InterfacePairInfo);

        LOG(DEBUG) << "adding tlvReceiverLinkMetric data to list"
                   << " rssi_db = " << int(InterfacePairInfo.link_metric_info.rssi_db);
    }
    return true;
}

bool node::ap_metrics_data::add_ap_metric_data(std::shared_ptr<wfa_map::tlvApMetrics> ApMetricData)
{
    bssid                               = ApMetricData->bssid();
    channel_utilization                 = ApMetricData->channel_utilization();
    number_of_stas_currently_associated = ApMetricData->number_of_stas_currently_associated();

    //copy all fields to database vector
    estimated_service_info_fields.clear();
    std::copy_n(ApMetricData->estimated_service_info_field(),
                ApMetricData->estimated_service_info_field_length(),
                std::back_inserter(estimated_service_info_fields));
    if (ApMetricData->estimated_service_parameters().include_ac_bk) {
        include_ac_bk = true;
    }
    if (ApMetricData->estimated_service_parameters().include_ac_vo) {
        include_ac_vo = true;
    }
    if (ApMetricData->estimated_service_parameters().include_ac_vi) {
        include_ac_vi = true;
    }
    return true;
}

std::vector<sMacAddr> node::get_unused_interfaces(const std::vector<sMacAddr> &new_interfaces)
{
    auto interfaces_mac_list = get_interfaces_mac();

    // Fastest way is checking that they are equal. If they are, nothing to be erased.
    if (interfaces_mac_list == new_interfaces)
        return {};

    // Loop through active interface and remove active ones to left only unused interfaces.
    for (auto &element : new_interfaces) {
        interfaces_mac_list.erase(
            std::remove(interfaces_mac_list.begin(), interfaces_mac_list.end(), element),
            interfaces_mac_list.end());
    }

    return interfaces_mac_list;
}

std::shared_ptr<prplmesh::controller::db::Interface> node::add_interface(const sMacAddr &mac)
{
    auto it =
        std::find_if(m_interfaces.begin(), m_interfaces.end(),
                     [mac](const std::shared_ptr<prplmesh::controller::db::Interface> &interface) {
                         return interface->m_mac == mac;
                     });
    if (it == m_interfaces.end()) {
        m_interfaces.emplace_back(
            std::make_shared<prplmesh::controller::db::Interface>(mac, *this));
        return m_interfaces.back();
    } else {
        return *it;
    }
}

std::shared_ptr<prplmesh::controller::db::Interface> node::get_interface(const sMacAddr &mac)
{
    auto it =
        std::find_if(m_interfaces.begin(), m_interfaces.end(),
                     [mac](const std::shared_ptr<prplmesh::controller::db::Interface> &interface) {
                         return interface->m_mac == mac;
                     });
    if (it == m_interfaces.end()) {
        return nullptr;
    } else {
        return *it;
    }
}

void node::remove_interface(const sMacAddr &mac)
{
    auto it =
        std::find_if(m_interfaces.begin(), m_interfaces.end(),
                     [mac](const std::shared_ptr<prplmesh::controller::db::Interface> &interface) {
                         return interface->m_mac == mac;
                     });
    if (it != m_interfaces.end()) {
        m_interfaces.erase(it);
    }
}

std::vector<sMacAddr> node::get_interfaces_mac()
{
    std::vector<sMacAddr> result{};

    for (const auto &interface : m_interfaces) {
        result.push_back(interface->m_mac);
    }

    return result;
}

std::shared_ptr<prplmesh::controller::db::Interface::sNeighbor>
node::add_neighbor(const sMacAddr &interface_mac, const sMacAddr &neighbor_mac, bool flag_ieee1905)
{
    auto interface = get_interface(interface_mac);
    if (!interface) {
        LOG(ERROR) << "Failed to get interface with mac:" << interface_mac;
        return nullptr;
    }

    return interface->m_neighbors.add(neighbor_mac, flag_ieee1905);
}
