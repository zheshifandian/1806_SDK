/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _PERSISTENT_DATABASE_AGING_OPERATION_H_
#define _PERSISTENT_DATABASE_AGING_OPERATION_H_

#include "../db/db.h"
#include "periodic_operation.h"

namespace son {
class persistent_database_aging_operation : public periodic_operation {
public:
    persistent_database_aging_operation(
        std::chrono::seconds period_interval_sec_, db &database_,
        const std::string &operation_name_ = std::string("persistent_database_aging"));
    virtual ~persistent_database_aging_operation() {}

protected:
    virtual void periodic_operation_function() override;

private:
    db &m_database;
    std::chrono::system_clock::time_point last_aging_check;
};
} // namespace son
#endif
