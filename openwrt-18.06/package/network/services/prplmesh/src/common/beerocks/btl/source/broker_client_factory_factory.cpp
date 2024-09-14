/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <btl/broker_client_factory_factory.h>

#include <btl/broker_client_factory_impl.h>
#include <btl/message_parser_stream_impl.h>
#include <btl/message_serializer_stream_impl.h>

#include <bcl/beerocks_backport.h>

#include <easylogging++.h>

namespace beerocks {
namespace btl {

std::unique_ptr<BrokerClientFactory>
create_broker_client_factory(const std::string &uds_path,
                             std::shared_ptr<beerocks::EventLoop> event_loop)
{
    // Create parser for broker messages received through a stream-oriented socket.
    auto message_parser = std::make_shared<beerocks::btl::MessageParserStreamImpl>();
    LOG_IF(!message_parser, FATAL) << "Unable to create message parser!";

    // Create serializer for broker messages to be sent through a stream-oriented socket.
    auto message_serializer = std::make_shared<beerocks::btl::MessageSerializerStreamImpl>();
    LOG_IF(!message_serializer, FATAL) << "Unable to create message serializer!";

    // Create broker client factory to create broker clients when requested
    return std::make_unique<beerocks::btl::BrokerClientFactoryImpl>(uds_path, message_parser,
                                                                    message_serializer, event_loop);
}

} // namespace btl

} // namespace beerocks
