/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_cmdu_server_factory.h>

#include <bcl/beerocks_cmdu_server_impl.h>
#include <bcl/network/cmdu_parser_stream_impl.h>
#include <bcl/network/cmdu_serializer_stream_impl.h>

#include <easylogging++.h>

namespace beerocks {

std::unique_ptr<CmduServer>
CmduServerFactory::create_instance(std::shared_ptr<beerocks::net::UdsAddress> uds_address,
                                   std::shared_ptr<EventLoop> event_loop)
{
    // Create parser for CMDU messages received through a stream-oriented socket.
    auto cmdu_parser = std::make_shared<beerocks::net::CmduParserStreamImpl>();
    LOG_IF(!cmdu_parser, FATAL) << "Unable to create CMDU parser!";

    // Create serializer for CMDU messages to be sent through a stream-oriented socket.
    auto cmdu_serializer = std::make_shared<beerocks::net::CmduSerializerStreamImpl>();
    LOG_IF(!cmdu_serializer, FATAL) << "Unable to create CMDU serializer!";

    // Create UDS server socket to connect with remote clients
    auto server_socket = beerocks::net::UdsServerSocket::create_instance(*uds_address);
    LOG_IF(!server_socket, FATAL) << "Unable to create UDS server socket!";

    // Create server to exchange CMDU messages with clients connected through a UDS socket
    return std::make_unique<CmduServerImpl>(std::move(server_socket), cmdu_parser, cmdu_serializer,
                                            event_loop);
}

} // namespace beerocks
