/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "periodic_operation.h"
#include <easylogging++.h>
#define OPERATION_LOG(a) (LOG(a) << "operation " << operation_name << " id " << id << ": ")

using namespace son;

int periodic_operation::latest_id = 1;

periodic_operation::periodic_operation(std::chrono::seconds period_interval_sec_,
                                       const std::string &operation_name_)
    : id(latest_id++), operation_name(operation_name_), interval_sec(period_interval_sec_)
{
    m_last_work_timestamp = std::chrono::steady_clock::now();
}

periodic_operation::~periodic_operation() {}

bool periodic_operation::work()
{
    auto now          = std::chrono::steady_clock::now();
    auto interval_sec = std::chrono::seconds(this->interval_sec);
    if (m_last_work_timestamp + interval_sec < now) {
        this->periodic_operation_function();
        m_last_work_timestamp = now;
        return true;
    }
    return false;
}
