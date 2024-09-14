/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_CMDU_CLIENT_FACTORY_IMPL_H_
#define _BEEROCKS_CMDU_CLIENT_FACTORY_IMPL_H_

#include <bcl/beerocks_cmdu_client_factory.h>

#include <bcl/beerocks_event_loop.h>
#include <bcl/network/cmdu_parser.h>
#include <bcl/network/cmdu_serializer.h>

namespace beerocks {

/**
 * Implementation of the CMDU client factory interface.
 * This implementation creates instances of CmduClientImpl class.
 */
class CmduClientFactoryImpl : public CmduClientFactory {
public:
    /**
     * @brief Class constructor.
     *
     * @param uds_path UDS path of the CMDU server that created client instances are connected to.
     * @param cmdu_parser CMDU parser used to get CMDU messages out of a byte array received
     * through a socket connection.
     * @param cmdu_serializer CMDU serializer used to put CMDU messages into a byte array ready to
     * be sent through a socket connection.
     * @param event_loop Application event loop used by the process to wait for I/O events.
     */
    CmduClientFactoryImpl(const std::string &uds_path,
                          std::shared_ptr<beerocks::net::CmduParser> cmdu_parser,
                          std::shared_ptr<beerocks::net::CmduSerializer> cmdu_serializer,
                          std::shared_ptr<beerocks::EventLoop> event_loop);

    /**
     * @brief Creates an instance of a CMDU client.
     *
     * @see CmduClientFactory::create_instance
     */
    std::unique_ptr<CmduClient> create_instance() override;

private:
    /**
     * UDS path of the CMDU server that created client instances are connected to.
     */
    std::string m_uds_path;

    /**
     * CMDU parser used to get CMDU messages out of a byte array received through a socket
     * connection.
     */
    std::shared_ptr<beerocks::net::CmduParser> m_cmdu_parser;

    /**
     * CMDU serializer used to put CMDU messages into a byte array ready to be sent through a
     * socket connection.
     */
    std::shared_ptr<beerocks::net::CmduSerializer> m_cmdu_serializer;

    /**
     * Application event loop used by the process to wait for I/O events.
     */
    std::shared_ptr<EventLoop> m_event_loop;
};

} // namespace beerocks

#endif // _BEEROCKS_CMDU_CLIENT_FACTORY_IMPL_H_
