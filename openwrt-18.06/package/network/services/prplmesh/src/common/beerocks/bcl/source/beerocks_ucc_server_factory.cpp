/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_ucc_server_factory.h>

#include <bcl/beerocks_ucc_parser_stream_impl.h>
#include <bcl/beerocks_ucc_serializer_stream_impl.h>
#include <bcl/beerocks_ucc_server_impl.h>
#include <bcl/network/sockets_impl.h>

#include <easylogging++.h>

namespace beerocks {

std::unique_ptr<beerocks::UccServer>
UccServerFactory::create_instance(uint16_t port, std::shared_ptr<EventLoop> event_loop)
{
    // Create parser for UCC command strings received through a stream-oriented socket.
    auto parser = std::make_shared<beerocks::UccParserStreamImpl>();
    LOG_IF(!parser, FATAL) << "Unable to create UCC parser!";

    // Create serializer for UCC reply strings to be sent through a stream-oriented socket.
    auto serializer = std::make_shared<beerocks::UccSerializerStreamImpl>();
    LOG_IF(!serializer, FATAL) << "Unable to create UCC serializer!";

    // Create TCP server socket to connect with remote clients
    auto server_socket = beerocks::net::TcpServerSocket::create_instance(port);
    LOG_IF(!server_socket, FATAL) << "Unable to create UCC server socket!";

    // Create server to exchange UCC commands and replies with clients connected through the socket
    return std::make_unique<UccServerImpl>(std::move(server_socket), parser, serializer,
                                           event_loop);
}

} // namespace beerocks
