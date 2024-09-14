/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_TIMER_FACTORY_H_
#define _BEEROCKS_TIMER_FACTORY_H_

#include <bcl/network/timer.h>

#include <memory>

namespace beerocks {

/**
 * Timer factory interface.
 *
 * Classes that need to create timer objects must use a factory to do so, instead of creating the
 * timers directly (to avoid dependencies on OS resources, like a timer file descriptor). Such
 * factory will be provided as a dependency so it can be mocked while unit testing. A timer factory
 * mock will return timer mocks whenever an expectation of a call to `create_instance()` is
 * satisfied.
 */
class TimerFactory {
public:
    /**
     * Default destructor.
     */
    virtual ~TimerFactory() = default;

    /**
     * @brief Creates an instance of a timer.
     *
     * @return Timer instance.
     */
    virtual std::unique_ptr<beerocks::net::Timer<>> create_instance() = 0;
};

} // namespace beerocks

#endif // _BEEROCKS_TIMER_FACTORY_H_
