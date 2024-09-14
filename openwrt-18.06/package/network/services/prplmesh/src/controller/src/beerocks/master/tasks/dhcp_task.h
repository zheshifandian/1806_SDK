/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _DHCP_TASK_H_
#define _DHCP_TASK_H_

#include "../db/db.h"
#include "task.h"
#include "task_pool.h"
#include <bcl/beerocks_timer_manager.h>
#include <bcl/network/file_descriptor.h>

namespace son {
class DhcpTask : public task {
public:
    DhcpTask(db &database_, std::shared_ptr<beerocks::TimerManager> timer_manager);
    ~DhcpTask() override;

    enum events {
        STA_CONNECTED,
    };

protected:
    void work() override{};
    void handle_event(int event_type, void *obj) override;

private:
    db &m_database; //!< Database object.
    std::shared_ptr<beerocks::TimerManager>
        m_timer_manager; //!< Timer manager to add/remove timers.

    //! File descriptor of the timer to get DHCP IPv4/IPv6 leases periodically.
    int m_periodic_timer{beerocks::net::FileDescriptor::invalid_descriptor};
    bool m_successful_init; //!< Flag for successful initialized ubus context.

    const std::chrono::milliseconds DELAY_AFTER_STA_CONNECTED_MS{5000};

    /**
     * @brief Start periodical DHCP IPv4/IPv6 lease poll with timer.
     *
     * Periodic lease polls are triggered with timer.
     * Leases are read from bpl::dhcp_get_leases() within timer handler.
     * After reading leases, database objects are filled.
     *
     * Period of this timer is config.dhcp_monitor_interval_seconds.
     * Timer delay is passed as argument to control timers delay.
     *
     * If periodical timer is already started, removes and adds again.
     * Timer restarted with new delay_ms.
     *
     * @param delay_ms delay timeout time in miliseconds.
     */
    void start_periodic_lease_poll(const std::chrono::milliseconds &delay_ms);

    /**
     * @brief Handles periodic lease poll timer timeout.
     *
     * It calls bpl::dhcp_get_leases() and related datamodel methods.
     */
    void handle_timer_timeout();
};

} // namespace son

#endif
