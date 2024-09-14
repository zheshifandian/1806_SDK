/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BTL_BROKER_CLIENT_FACTORY_FACTORY_H_
#define BTL_BROKER_CLIENT_FACTORY_FACTORY_H_

#include <btl/broker_client_factory.h>

#include <bcl/beerocks_event_loop.h>

namespace beerocks {
namespace btl {

/**
 * @brief Creates an instance of a broker client factory.
 *
 * A broker client factory creates broker client instances connected to broker server running in
 * the transport process.
 *
 * @param uds_address Unix Domain Socket address where the broker server is listening for connection
 * requests and hence the broker client has to connect to.
 * @param event_loop Application event loop used by the process to wait for I/O events.
 */
std::unique_ptr<BrokerClientFactory>
create_broker_client_factory(const std::string &uds_path,
                             std::shared_ptr<beerocks::EventLoop> event_loop);

} // namespace btl

} // namespace beerocks

#endif // BTL_BROKER_CLIENT_FACTORY_FACTORY_H_
