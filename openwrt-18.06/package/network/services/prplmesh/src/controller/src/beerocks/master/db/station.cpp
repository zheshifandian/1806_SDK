/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "station.h"

#include <bcl/beerocks_defines.h>
#include <chrono>
#include <easylogging++.h>

namespace prplmesh {
namespace controller {
namespace db {

void Station::assign_client_locating_task_id(int new_task_id, bool new_connection)
{
    if (new_connection) {
        m_client_locating_task_id_new_connection = new_task_id;
    } else {
        m_client_locating_task_id_exist_connection = new_task_id;
    }
}

int Station::get_client_locating_task_id(bool new_connection)
{
    if (new_connection) {
        return m_client_locating_task_id_new_connection;
    }
    return m_client_locating_task_id_exist_connection;
}

class Station::beacon_measurement {
public:
    beacon_measurement(const std::string &ap_mac_, uint8_t rcpi_, uint8_t rsni_) : ap_mac(ap_mac_)
    {
        rcpi      = rcpi_; // received channel power indication (convertable to rssi)
        rsni      = rsni_; // received signal noise indication (SNR)
        timestamp = std::chrono::steady_clock::now();
    }
    const std::string ap_mac;
    uint8_t rcpi = beerocks::RCPI_INVALID;
    uint8_t rsni = 0;
    std::chrono::steady_clock::time_point timestamp;
};

class Station::rssi_measurement {
public:
    rssi_measurement(const std::string &ap_mac_, int8_t rssi_, int8_t packets_) : ap_mac(ap_mac_)
    {
        rssi      = rssi_;
        packets   = packets_;
        timestamp = std::chrono::steady_clock::now();
    }
    const std::string ap_mac;
    int8_t rssi = beerocks::RSSI_INVALID;
    int8_t packets;
    std::chrono::steady_clock::time_point timestamp;
};

bool Station::get_beacon_measurement(const std::string &ap_mac_, uint8_t &rcpi, uint8_t &rsni)
{
    auto it = m_beacon_measurements.find(ap_mac_);
    if (it == m_beacon_measurements.end()) {
        LOG(ERROR) << "ap_mac " << ap_mac_ << " does not exist!";
        rcpi = beerocks::RCPI_INVALID;
        rsni = 0;
        return false;
    }
    rcpi = it->second->rcpi;
    rsni = it->second->rsni;
    return true;
}

void Station::set_beacon_measurement(const std::string &ap_mac_, uint8_t rcpi, uint8_t rsni)
{
    auto it = m_beacon_measurements.find(ap_mac_);
    if (it == m_beacon_measurements.end()) {
        std::shared_ptr<beacon_measurement> m =
            std::make_shared<beacon_measurement>(ap_mac_, rcpi, rsni);
        m_beacon_measurements.insert(std::make_pair(ap_mac_, m));
    } else {
        it->second->rcpi      = rcpi;
        it->second->rsni      = rsni;
        it->second->timestamp = std::chrono::steady_clock::now();
    }
}

bool Station::get_cross_rx_rssi(const std::string &ap_mac_, int8_t &rssi, int8_t &packets)
{
    auto it = m_cross_rx_rssi.find(ap_mac_);
    if (it == m_cross_rx_rssi.end()) {
        rssi    = beerocks::RSSI_INVALID;
        packets = -1;
        return false;
    }
    rssi    = it->second->rssi;
    packets = it->second->packets;
    return true;
}

void Station::set_cross_rx_rssi(const std::string &ap_mac_, int8_t rssi, int8_t packets)
{
    auto it = m_cross_rx_rssi.find(ap_mac_);
    if (it == m_cross_rx_rssi.end()) {
        std::shared_ptr<rssi_measurement> m =
            std::make_shared<rssi_measurement>(ap_mac_, rssi, packets);
        m_cross_rx_rssi.insert(std::make_pair(ap_mac_, m));
    } else {
        it->second->rssi      = rssi;
        it->second->timestamp = std::chrono::steady_clock::now();
        it->second->packets   = packets;
    }
}

void Station::clear_cross_rssi()
{
    m_cross_rx_rssi.clear();
    m_beacon_measurements.clear();
}

void Station::set_bss(std::shared_ptr<Agent::sRadio::sBss> bss) { m_bss = bss; }

std::shared_ptr<Agent::sRadio::sBss> Station::get_bss() { return m_bss.lock(); }

} // namespace db
} // namespace controller
} // namespace prplmesh
