/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_CMDU_CLIENT_H_
#define _BEEROCKS_CMDU_CLIENT_H_

#include <tlvf/CmduMessageRx.h>
#include <tlvf/CmduMessageTx.h>
#include <tlvf/common/sMacAddr.h>

namespace beerocks {

/**
 * @brief The CMDU client is a component to send and receive CMDU messages between local processes.
 *
 * This component acts as the client side end-point of the connection between communicating
 * processes while the @see CmduServer plays the server role.
 *
 * Users of this component send CMDU messages and (optionally) register a handler to process
 * received CMDU messages.
 */
class CmduClient {
public:
    /**
     * @brief CMDU-received event handler function.
     *
     * Note: parameters iface_index, dst_mac and src_mac are only filled in if CMDU was originally
     * sent by a remote process and then forwarded by the local process that receives it to this
     * process.
     *
     * @param iface_index Index of the network interface that the CMDU message was received on.
     * @param dst_mac Destination MAC address.
     * @param src_mac Source MAC address.
     * @param cmdu_rx The CMDU message received.
     */
    using CmduReceivedHandler =
        std::function<void(uint32_t iface_index, const sMacAddr &dst_mac, const sMacAddr &src_mac,
                           ieee1905_1::CmduMessageRx &cmdu_rx)>;

    /**
     * @brief Connection-closed event handler function.
     *
     * If this handler is set, it will be called back whenever the connection to the CMDU server
     * gets closed (e.g.: when the server goes down because the remote process dies). The handler
     * may, for example, implement a recovery mechanism that includes reconnecting to the server
     * and creating another client.
     */
    using ConnectionClosedHandler = std::function<void()>;

    /**
     * Set of event handler functions, one function to handle each possible event happened.
     * Handlers are grouped into a struct to facilitate passing them as a single parameter to the
     * method used to set the handlers.
     * Event handlers are optional and if not set for a given event, that event will be silently
     * ignored.
     */
    struct EventHandlers {
        /**
         * Handler function called back by the CMDU client to deal with CMDU-received event.
         */
        CmduReceivedHandler on_cmdu_received;

        /**
         * Handler function called back by the CMDU client to deal with connection-closed event.
         */
        ConnectionClosedHandler on_connection_closed;
    };

    /**
     * Default destructor.
     */
    virtual ~CmduClient() = default;

    /**
     * @brief Sets the event handler functions.
     *
     * Sets the callback functions to be executed whenever an event occurs on this client.
     * The event handler functions are all optional and if any of them is not set, the corresponding
     * event will be silently ignored.
     *
     * @param handlers Event handler functions.
     */
    void set_handlers(const EventHandlers &handlers) { m_handlers = handlers; }

    /**
     * @brief Clears previously set event handler functions.
     */
    void clear_handlers() { m_handlers = {}; }

    /**
     * @brief Sends a CMDU message to the connected server.
     *
     * @param cmdu_tx The CMDU message to send.
     * @return true on success and false otherwise.
     */
    virtual bool send_cmdu(ieee1905_1::CmduMessageTx &cmdu_tx) const = 0;

    /**
     * @brief Forwards a CMDU message that was sent by another thread or process.
     *
     * Forwards a received CMDU message to a server.
     *
     * The CMDU message was originally sent by a remote entity running in a different thread or
     * process.
     *
     * @param cmdu_rx The received CMDU message to forward.
     * @return true on success and false otherwise.
     */
    virtual bool forward_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx) = 0;

    /**
     * @brief Get the file descriptor of the client.
     * 
     * @return Client file descriptor.
     */
    virtual int get_fd() = 0;

protected:
    /**
     * @brief Notifies a CMDU-received event.
     *
     * @param iface_index Index of the network interface that the CMDU message was received on.
     * @param dst_mac Destination MAC address.
     * @param src_mac Source MAC address.
     * @param cmdu_rx The CMDU message received.
     */
    void notify_cmdu_received(uint32_t iface_index, const sMacAddr &dst_mac,
                              const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx) const
    {
        if (m_handlers.on_cmdu_received) {
            m_handlers.on_cmdu_received(iface_index, dst_mac, src_mac, cmdu_rx);
        }
    }

    /**
     * @brief Notifies a connection-closed event.
     */
    void notify_connection_closed() const
    {
        if (m_handlers.on_connection_closed) {
            m_handlers.on_connection_closed();
        }
    }

private:
    /**
     * Set of event handler functions that are called back whenever a new event occurs on this
     * client.
     */
    EventHandlers m_handlers;
};

} // namespace beerocks

#endif // _BEEROCKS_CMDU_CLIENT_H_
