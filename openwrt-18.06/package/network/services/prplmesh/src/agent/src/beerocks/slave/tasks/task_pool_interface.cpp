/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "task_pool_interface.h"
#include <iostream>

namespace beerocks {

// helper for debug
std::ostream &operator<<(std::ostream &o, const eTaskEvent &task_event)
{
    switch (task_event) {
    case eTaskEvent::CAC_STARTED_NOTIFICATION:
        o << "CAC_STARTED_NOTIFICATION";
        break;
    case eTaskEvent::CAC_COMPLETED_NOTIFICATION:
        o << "CAC_COMPLETED_NOTIFICATION";
        break;
    case eTaskEvent::SWITCH_CHANNEL_NOTIFICATION_EVENT:
        o << "SWITCH_CHANNEL_NOTIFICATION_EVENT";
        break;
    case eTaskEvent::SWITCH_CHANNEL_DURATION_TIME:
        o << "SWITCH_CHANNEL_DURATION_TIME";
        break;
    case eTaskEvent::SWITCH_CHANNEL_REQUEST:
        o << "SWITCH_CHANNEL_REQUEST";
        break;
    case eTaskEvent::SWITCH_CHANNEL_REPORT:
        o << "SWITCH_CHANNEL_REPORT";
        break;
    }
    return o;
}

} // namespace beerocks
