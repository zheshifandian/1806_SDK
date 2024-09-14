/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_EVENT_LOOP_MOCK_H_
#define _BEEROCKS_EVENT_LOOP_MOCK_H_

#include <bcl/beerocks_event_loop.h>

#include <gmock/gmock.h>

namespace beerocks {

class EventLoopMock : public EventLoop {
public:
    MOCK_METHOD(bool, register_handlers, (int fd, const EventHandlers &handlers), (override));
    MOCK_METHOD(bool, remove_handlers, (int fd), (override));
    MOCK_METHOD(int, run, (), (override));
    MOCK_METHOD(bool, set_handler_name, (int fd, const std::string &name), (override));
};

} // namespace beerocks

#endif // _BEEROCKS_EVENT_LOOP_MOCK_H_
