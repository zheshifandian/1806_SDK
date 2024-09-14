/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BCL_TRANSACTION_H_
#define _BCL_TRANSACTION_H_

#include <functional>
#include <stack>

namespace beerocks {

/**
 * This class models an SQL-transaction like object. 
 * 
 * A transaction, which is made of a series of steps, must be executed following the all-or-none 
 * principle, that is, it must be executed in its entirety, or not executed at all.
 * 
 * The all-or-none principle states that if all steps are successful, then commit the transaction 
 * (do everything). If any of the steps fails, then rollback the entire transaction (do nothing).
 * 
 * The class is intended to be used together with the RAII programming idiom (destructor does
 * rollback and commit is explicit).
 */
class Transaction {
public:
    /**
     * @brief Class destructor.
     * 
     * Rollback all steps not yet commited.
     */
    ~Transaction() { rollback(); }

    /**
     * @brief Adds a rollback action to undo a transaction step not commited.
     *
     * Each transaction step may have a rollback action, which must be executed to undo such step in
     * case the transaction is not commited.
     * 
     * @param action Rollback action.
     */
    void add_rollback_action(const std::function<void()> &action)
    {
        m_rollback_actions.push(action);
    }

    /**
     * @brief Commits transaction.
     *
     * Removes all pending rollback actions, which do not have to be executed because the 
     * transaction has completed successfully.
     */
    void commit()
    {
        while (!m_rollback_actions.empty()) {
            m_rollback_actions.pop();
        }
    }

    /**
     * @brief Rolls back transaction.
     *
     * Executes the rollback actions registered to undo the steps executed so far because the 
     * transaction has failed. Actions are executed in the reverse order of insertion.
     */
    void rollback()
    {
        while (!m_rollback_actions.empty()) {
            const auto &action = m_rollback_actions.top();
            action();
            m_rollback_actions.pop();
        }
    }

private:
    /**
     * Stack of rollback actions to undo all transaction steps executed so far.
     */
    std::stack<std::function<void()>> m_rollback_actions;
};

} // namespace beerocks

#endif // _BCL_TRANSACTION_H_
