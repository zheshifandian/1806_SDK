/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "dhcp_task.h"

#include "bpl/bpl_dhcp.h"
#include <easylogging++.h>

using namespace beerocks;
using namespace son;

DhcpTask::DhcpTask(db &database_, std::shared_ptr<beerocks::TimerManager> timer_manager)
    : task("dhcp task"), m_database(database_), m_timer_manager(timer_manager)
{
    LOG_IF(!m_timer_manager, FATAL) << "Timer manager is a null pointer!";

    m_database.assign_dhcp_task_id(id);

    if (!bpl::dhcp_manual_procedure_init()) {
        LOG(INFO) << "DHCP procedure init failed. Periodical calls are stopped.";
        m_successful_init = false;
    } else {
        m_successful_init = true;
        start_periodic_lease_poll(m_database.config.dhcp_monitor_interval_seconds);
    }
}

DhcpTask::~DhcpTask()
{
    if (m_periodic_timer != beerocks::net::FileDescriptor::invalid_descriptor) {
        m_timer_manager->remove_timer(m_periodic_timer);
    }

    bpl::dhcp_manual_procedure_destroy();
}

void DhcpTask::handle_timer_timeout()
{
    bpl::leases_pair_t leases;

    if (!bpl::dhcp_get_leases(leases)) {
        LOG(ERROR) << "DHCP get leases failed.";
        return;
    }

    for (const auto &ipv6 : leases.second) {
        m_database.set_sta_dhcp_v6_lease(ipv6.second.mac, ipv6.second.host_name,
                                         ipv6.second.ip_address);
    }

    // To give precedence ipv4 hostname in database in case of host name conflict,
    // ipv4 leases are proccessed later.
    for (const auto &ipv4 : leases.first) {
        m_database.set_sta_dhcp_v4_lease(ipv4.first, ipv4.second.host_name, ipv4.second.ip_address);
    }
}

void DhcpTask::start_periodic_lease_poll(const std::chrono::milliseconds &delay_ms)
{

    // Remove old periodic timer.
    if (m_periodic_timer != beerocks::net::FileDescriptor::invalid_descriptor) {
        m_timer_manager->remove_timer(m_periodic_timer);
    }

    m_periodic_timer = m_timer_manager->add_timer("DHCP Periodic", delay_ms,
                                                  m_database.config.dhcp_monitor_interval_seconds,
                                                  [this](int fd, beerocks::EventLoop &loop) {
                                                      this->handle_timer_timeout();
                                                      return true;
                                                  });
}

void DhcpTask::handle_event(int event_type, void *obj)
{
    if (event_type == STA_CONNECTED) {

        // Events should be handled in case of successful init.
        if (m_successful_init) {
            start_periodic_lease_poll(DELAY_AFTER_STA_CONNECTED_MS);
        }
    }
}
