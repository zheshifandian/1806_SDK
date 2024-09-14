/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_TIMER_FACTORY_IMPL_H_
#define _BEEROCKS_TIMER_FACTORY_IMPL_H_

#include <bcl/beerocks_timer_factory.h>

#include <bcl/network/timer_impl.h>

namespace beerocks {

/**
 * Implementation of the timer factory interface.
 */
class TimerFactoryImpl : public TimerFactory {
public:
    /**
     * @brief Creates an instance of a timer.
     *
     * This implementation creates instances of TimerImpl class using milliseconds as time units.
     *
     * @see TimerFactory::create_instance
     */
    std::unique_ptr<beerocks::net::Timer<>> create_instance() override
    {
        return std::make_unique<beerocks::net::TimerImpl<>>();
    }
};

} // namespace beerocks

#endif // _BEEROCKS_TIMER_FACTORY_IMPL_H_
