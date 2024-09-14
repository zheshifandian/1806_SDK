/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _PERIODIC_OPERATION_H_
#define _PERIODIC_OPERATION_H_

#include <chrono>
#include <string>

namespace son {

class periodic_operation {
public:
    /**
     * @brief Construct a new periodic operation object
     * 
     * @param period_interval_sec_ Interval for operation between one operation and the next.
     * @param operation_name_ Name for the operation, used in logs.
     */
    periodic_operation(std::chrono::seconds period_interval_sec_,
                       const std::string &operation_name_ = std::string());
    virtual ~periodic_operation();
    bool work();

    // Unique identifier for the operation, used in logs.
    const int id;
    // Name for operation, used in logs.
    std::string operation_name;

protected:
    // Interval for operation between one operation and the next.
    std::chrono::seconds interval_sec;
    virtual void periodic_operation_function() = 0;

private:
    // Last timestamp that the periodic operation function was called.
    std::chrono::steady_clock::time_point m_last_work_timestamp;
    // Unique identifier that is used as the index for the ID member.
    static int latest_id;
};

} //  namespace son

#endif
