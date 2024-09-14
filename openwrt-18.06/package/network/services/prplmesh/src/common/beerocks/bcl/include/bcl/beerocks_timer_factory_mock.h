/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_TIMER_FACTORY_MOCK_H_
#define _BEEROCKS_TIMER_FACTORY_MOCK_H_

#include <bcl/beerocks_timer_factory.h>

#include <gmock/gmock.h>

namespace beerocks {

class TimerFactoryMock : public TimerFactory {
public:
    // Google Mock cannot mock a factory method that returns a non copyable return value.
    // To work around this, we add an indirection through a proxy method.
    // Production code will use the overridden method and unit tests will set expectations in the
    // mocked helper method instead.
    virtual std::unique_ptr<beerocks::net::Timer<>> create_instance() override
    {
        return std::unique_ptr<beerocks::net::Timer<>>(create_instance_proxy());
    };
    MOCK_METHOD(beerocks::net::Timer<> *, create_instance_proxy, ());
};

} // namespace beerocks

#endif // _BEEROCKS_TIMER_FACTORY_MOCK_H_
