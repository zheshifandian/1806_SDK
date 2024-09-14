/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _PERIODIC_OPERATION_POOL_H_
#define _PERIODIC_OPERATION_POOL_H_

#include "periodic_operation.h"
#include <memory>
#include <unordered_map>
namespace son {

class periodic_operation_pool {

public:
    periodic_operation_pool() {}
    ~periodic_operation_pool() {}

    /**
     * @brief Add a new operation to the operation pool
     *  An operation will run each time it's interval lapses
     * @param new_operation Shared pointer of new operation to be added.
     * @return true is successful, false otherwise
     */
    bool add_operation(std::shared_ptr<periodic_operation> new_operation);
    /**
     * @brief Checks if the operation identified by the ID number is still alive
     * which means that the operation is either running or is pending to run.
     * @param id Unique identifier number
     * @return true is successful, false otherwise
     */
    bool is_operation_alive(int id);
    /**
     * @brief Kills the operation associated with the given ID
     * If the operation is currently running it would not stop, but would not run again afterwards.
     * @param id Unique identifier number
     */
    void kill_operation(int id);
    /**
     * @brief Iterates over the operation pool triggering those who lapsed their interval.
     * 
     */
    void run_operations();

private:
    std::unordered_map<int, std::shared_ptr<periodic_operation>> periodic_operations;
};

} // namespace son

#endif
