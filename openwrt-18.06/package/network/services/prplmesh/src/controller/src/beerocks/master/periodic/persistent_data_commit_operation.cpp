/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "persistent_data_commit_operation.h"
#include <easylogging++.h>
#define OPERATION_LOG(a) (LOG(a) << "operation " << operation_name << " id " << id << ": ")

using namespace son;

persistent_data_commit_operation::persistent_data_commit_operation(
    db &database, std::chrono::seconds period_interval_sec_, const std::string &operation_name_)
    : periodic_operation(period_interval_sec_, "persistent data commit operation"),
      m_database(database)
{
    m_database.assign_persistent_db_data_commit_operation_id(id);
}

void persistent_data_commit_operation::periodic_operation_function()
{
    if (!m_database.is_commit_to_persistent_db_required()) {
        return;
    }
    if (!m_database.commit_persistent_db_changes()) {
        return;
    }

    OPERATION_LOG(TRACE) << "Successfully committed changes to the persistent DB";
}
