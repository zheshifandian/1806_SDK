/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_cmdu_client_factory_factory.h>

#include <bcl/beerocks_cmdu_client_factory_impl.h>
#include <bcl/network/cmdu_parser_stream_impl.h>
#include <bcl/network/cmdu_serializer_stream_impl.h>

#include <bcl/beerocks_backport.h>

#include <easylogging++.h>

namespace beerocks {

std::unique_ptr<CmduClientFactory>
create_cmdu_client_factory(const std::string &uds_path,
                           std::shared_ptr<beerocks::EventLoop> event_loop)
{
    // Create parser for CMDU messages received through a stream-oriented socket.
    auto cmdu_parser = std::make_shared<beerocks::net::CmduParserStreamImpl>();
    LOG_IF(!cmdu_parser, FATAL) << "Unable to create CMDU parser!";

    // Create serializer for CMDU messages to be sent through a stream-oriented socket.
    auto cmdu_serializer = std::make_shared<beerocks::net::CmduSerializerStreamImpl>();
    LOG_IF(!cmdu_serializer, FATAL) << "Unable to create CMDU serializer!";

    // Create CMDU client factory to create CMDU clients when requested
    return std::make_unique<beerocks::CmduClientFactoryImpl>(uds_path, cmdu_parser, cmdu_serializer,
                                                             event_loop);
}

} // namespace beerocks
