/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_cmdu_client_factory_impl.h>
#include <bcl/beerocks_cmdu_client_impl.h>

#include <bcl/beerocks_backport.h>
#include <bcl/network/sockets_impl.h>

#include <easylogging++.h>

namespace beerocks {

CmduClientFactoryImpl::CmduClientFactoryImpl(
    const std::string &uds_path, std::shared_ptr<beerocks::net::CmduParser> cmdu_parser,
    std::shared_ptr<beerocks::net::CmduSerializer> cmdu_serializer,
    std::shared_ptr<beerocks::EventLoop> event_loop)
    : m_uds_path(uds_path), m_cmdu_parser(cmdu_parser), m_cmdu_serializer(cmdu_serializer),
      m_event_loop(event_loop)
{
    LOG_IF(m_uds_path.empty(), FATAL) << "UDS path is an empty string!";
    LOG_IF(!m_cmdu_parser, FATAL) << "CMDU parser is a null pointer!";
    LOG_IF(!m_cmdu_serializer, FATAL) << "CMDU serializer is a null pointer!";
    LOG_IF(!m_event_loop, FATAL) << "Event loop is a null pointer!";
}

std::unique_ptr<CmduClient> CmduClientFactoryImpl::create_instance()
{
    // Create UDS socket
    auto socket = std::make_shared<beerocks::net::UdsSocket>();

    // Create UDS client socket to connect to server socket
    using UdsClientSocket = beerocks::net::ClientSocketImpl<beerocks::net::UdsSocket>;
    auto client_socket    = std::make_unique<UdsClientSocket>(socket);

    // Connect client socket to server socket
    beerocks::net::UdsAddress address(m_uds_path);
    auto connection = client_socket->connect(address);
    if (!connection) {
        LOG(ERROR) << "Unable to connect client socket to '" << address.path() + "'";
        return nullptr;
    }

    LOG(DEBUG) << "CMDU client created with fd = " << connection->socket()->fd();
    auto cmdu_client = std::make_unique<CmduClientImpl>(std::move(connection), m_cmdu_parser,
                                                        m_cmdu_serializer, m_event_loop);
    if (!cmdu_client) {
        LOG(ERROR) << "Unable to create CMDU client connected to '" << address.path() + "'";
        return nullptr;
    }

    return cmdu_client;
}

} // namespace beerocks
