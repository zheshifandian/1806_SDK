/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BTL_BROKER_CLIENT_FACTORY_IMPL_H_
#define BTL_BROKER_CLIENT_FACTORY_IMPL_H_

#include <btl/broker_client_factory.h>

#include <btl/broker_client.h>
#include <btl/message_parser.h>
#include <btl/message_serializer.h>

#include <bcl/beerocks_event_loop.h>

namespace beerocks {
namespace btl {

/**
 * Implementation of the broker client factory interface.
 * This implementation creates instances of BrokerClientImpl class.
 */
class BrokerClientFactoryImpl : public BrokerClientFactory {
public:
    /**
     * @brief Class constructor.
     *
     * @param uds_path UDS path of the broker server that created client instances are connected to.
     * @param message_parser Message parser used to get transport messages out of a byte array
     * received through a socket connection.
     * @param message_serializer Message serializer used to put transport messages into a byte
     * array ready to be sent through a socket connection.
     * @param event_loop Application event loop used by the process to wait for I/O events.
     */
    BrokerClientFactoryImpl(const std::string &uds_path,
                            std::shared_ptr<MessageParser> message_parser,
                            std::shared_ptr<MessageSerializer> message_serializer,
                            std::shared_ptr<beerocks::EventLoop> event_loop);

    /**
     * @brief Creates an instance of a broker client.
     *
     * @see BrokerClientFactory::create_instance
     */
    std::unique_ptr<BrokerClient> create_instance() override;

private:
    /**
     * UDS path of the broker server that created client instances are connected to.
     */
    std::string m_uds_path;

    /**
     * Message parser used to get transport messages out of a byte array received through a socket
     * connection.
     */
    std::shared_ptr<MessageParser> m_message_parser;

    /**
     * Message serializer used to put transport messages into a byte array ready to be sent through
     * a socket connection.
     */
    std::shared_ptr<MessageSerializer> m_message_serializer;

    /**
     * Application event loop used by the process to wait for I/O events.
     */
    std::shared_ptr<EventLoop> m_event_loop;
};

} // namespace btl

} // namespace beerocks

#endif // BTL_BROKER_CLIENT_FACTORY_IMPL_H_
