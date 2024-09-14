/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "persistent_database_aging.h"
#include <easylogging++.h>
#define OPERATION_LOG(a) (LOG(a) << "operation " << operation_name << " id " << id << ": ")

using namespace son;

persistent_database_aging_operation::persistent_database_aging_operation(
    std::chrono::seconds period_interval_sec_, db &database_, const std::string &operation_name_)
    : periodic_operation(period_interval_sec_, operation_name_), m_database(database_)
{
    m_database.assign_persistent_db_aging_operation_id(id);
    last_aging_check = std::chrono::system_clock::time_point::min();
}

void persistent_database_aging_operation::periodic_operation_function()
{
    last_aging_check = std::chrono::system_clock::now();
    auto clients     = m_database.get_clients_with_persistent_data_configured();
    std::vector<sMacAddr> aged_clients;
    std::copy_if(
        clients.begin(), clients.end(), std::back_inserter(aged_clients),
        [&](const sMacAddr &client_mac) {
            auto client = m_database.get_station(client_mac);
            if (!client) {
                OPERATION_LOG(ERROR) << "client " << client_mac << " not found";
                return false;
            }

            // Client timelife delay
            // If a client has a predetermined timelife delay use that.
            // Otherwise use the Max timelife delay/unfriendly_device_max_timelife_delay_sec according
            // to the client's unfriendliness status.
            const auto max_timelife_delay_sec =
                std::chrono::seconds(m_database.config.max_timelife_delay_minutes * 60);
            const auto unfriendly_device_max_timelife_delay_sec = std::chrono::seconds(
                m_database.config.unfriendly_device_max_timelife_delay_minutes * 60);

            auto timelife_delay_seconds = (client->is_unfriendly == eTriStateBool::TRUE)
                                              ? unfriendly_device_max_timelife_delay_sec
                                              : max_timelife_delay_sec;

            std::chrono::minutes temp_timelife = client->time_life_delay_minutes;

            if ((temp_timelife == std::chrono::minutes::zero())) {
                return false;
            } else if (temp_timelife > std::chrono::minutes::zero()) {
                timelife_delay_seconds =
                    std::chrono::duration_cast<std::chrono::seconds>(temp_timelife);
            }

            // Calculate client expiry due time
            auto expiry_due = client->parameters_last_edit + timelife_delay_seconds;

            // If the expiry due is less then the last aging check, the client is considered aged.
            return expiry_due < last_aging_check;
        });

    if (aged_clients.size() > 0) {
        OPERATION_LOG(TRACE) << "Found " << aged_clients.size()
                             << " aged clients, clearing persistent data";
    }
    for (const auto &aged_client : aged_clients) {
        m_database.clear_client_persistent_db(aged_client);
    }
}
