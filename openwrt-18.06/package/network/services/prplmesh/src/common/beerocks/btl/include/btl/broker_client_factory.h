/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BTL_BROKER_CLIENT_FACTORY_H_
#define BTL_BROKER_CLIENT_FACTORY_H_

#include <btl/broker_client.h>

#include <memory>

namespace beerocks {
namespace btl {

/**
 * Broker client factory interface.
 *
 * Classes that need to dynamically create broker client objects must use a factory to do so,
 * instead of creating the clients directly (to avoid dependencies with real implementations). Such
 * factory will be provided as a dependency so it can be mocked while unit testing. A broker client
 * factory mock will return broker client mocks whenever an expectation of a call to
 * `create_instance()` is satisfied.
 */
class BrokerClientFactory {
public:
    /**
     * Default destructor.
     */
    virtual ~BrokerClientFactory() = default;

    /**
     * @brief Creates an instance of a broker client.
     *
     * The broker client created is already connected to the broker server.
     *
     * @return Broker client instance.
     */
    virtual std::unique_ptr<BrokerClient> create_instance() = 0;
};

} // namespace btl

} // namespace beerocks

#endif // BTL_BROKER_CLIENT_FACTORY_H_
