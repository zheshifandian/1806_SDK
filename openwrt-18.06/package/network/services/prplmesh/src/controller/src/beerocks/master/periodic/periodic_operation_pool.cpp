/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "periodic_operation_pool.h"
#include <easylogging++.h>

using namespace son;

bool periodic_operation_pool::add_operation(std::shared_ptr<periodic_operation> new_operation)
{
    LOG(TRACE) << "inserting new operation, id=" << int(new_operation->id)
               << " operation name=" << new_operation->operation_name;
    return (periodic_operations.insert(std::make_pair(new_operation->id, new_operation))).second;
}

bool periodic_operation_pool::is_operation_alive(int id)
{
    auto it = periodic_operations.find(id);
    return (it != periodic_operations.end() && it->second != nullptr);
}

void periodic_operation_pool::kill_operation(int id)
{
    auto it = periodic_operations.find(id);
    if (it != periodic_operations.end() && it->second != nullptr) {
        LOG(DEBUG) << "killing operation " << it->second->operation_name << ", id " << it->first;
        periodic_operations.erase(it);
    }
}

void periodic_operation_pool::run_operations()
{
    for (auto oper : periodic_operations) {
        oper.second->work();
    }
}
