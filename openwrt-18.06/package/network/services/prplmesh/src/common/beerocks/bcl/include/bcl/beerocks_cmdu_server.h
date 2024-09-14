/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_CMDU_SERVER_H_
#define _BEEROCKS_CMDU_SERVER_H_

#include <tlvf/CmduMessageRx.h>
#include <tlvf/CmduMessageTx.h>
#include <tlvf/common/sMacAddr.h>

namespace beerocks {

/**
 * @brief The CMDU server is a component to send and receive CMDU messages between local processes.
 *
 * This component acts as the server side end-point of the connection between communicating
 * processes while the @see CmduClient plays the client role.
 *
 * Users of this component register a handler to process received CMDU messages and send CMDU
 * messages in response.
 */
class CmduServer {
public:
    /**
     * @brief Client-connected event handler function.
     *
     * @param fd File descriptor of the accepted socket connection.
     */
    using ClientConnectedHandler = std::function<void(int fd)>;

    /**
     * @brief Client-disconnected event handler function.
     *
     * @param fd File descriptor of the disconnected socket.
     */
    using ClientDisconnectedHandler = std::function<void(int fd)>;

    /**
     * @brief CMDU-received event handler function.
     *
     * Note: parameters iface_index, dst_mac and src_mac are only filled in if CMDU was originally
     * sent by a remote process and then forwarded by the local process that receives it to this
     * process.
     *
     * @param fd File descriptor of the socket connection the CMDU was received through.
     * @param iface_index Index of the network interface that the CMDU message was received on.
     * @param dst_mac Destination MAC address.
     * @param src_mac Source MAC address.
     * @param cmdu_rx The CMDU message received.
     */
    using CmduReceivedHandler =
        std::function<void(int fd, uint32_t iface_index, const sMacAddr &dst_mac,
                           const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx)>;

    /**
     * Set of event handler functions, one function to handle each possible event happened.
     * Handlers are grouped into a struct to facilitate passing them as a single parameter to the
     * method used to set the handlers.
     * Event handlers are optional and if not set for a given event, that event will be silently
     * ignored.
     */
    struct EventHandlers {
        /**
         * Handler function called back by the CMDU server to deal with client-connected event.
         */
        ClientConnectedHandler on_client_connected;

        /**
         * Handler function called back by the CMDU server to deal with client-disconnected event.
         */
        ClientDisconnectedHandler on_client_disconnected;

        /**
         * Handler function called back by the CMDU server to deal with CMDU-received event.
         */
        CmduReceivedHandler on_cmdu_received;
    };

    /**
     * Default destructor.
     */
    virtual ~CmduServer() = default;

    /**
     * @brief Sets the event handler functions.
     *
     * Sets the callback functions to be executed whenever an event occurs on this server.
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
     * @brief Disconnects a client socket connection.
     *
     * Closes the given client socket connection and frees allocated resources.
     *
     * @param fd File descriptor of the socket connection to disconnect.
     * @return true on success and false otherwise.
     */
    virtual bool disconnect(int fd) = 0;

    /**
     * @brief Sends a CMDU message.
     *
     * Sends a CMDU message to a client through the given socket connection.
     *
     * @param fd File descriptor of the socket connection to send CMDU through.
     * @param cmdu_tx The CMDU message to send.
     * @return true on success and false otherwise.
     */
    virtual bool send_cmdu(int fd, ieee1905_1::CmduMessageTx &cmdu_tx) = 0;

    /**
     * @brief Forwards a CMDU message that was sent by a remote process.
     *
     * Forwards a received CMDU message to a client through the given socket connection.
     *
     * The CMDU message was originally sent by a remote process running in a different device
     * (interface index, source and destination MAC addresses provide routing information).
     *
     * @param fd File descriptor of the socket connection to send CMDU through.
     * @param iface_index Index of the network interface that the CMDU message was received on.
     * @param dst_mac Destination MAC address.
     * @param src_mac Source MAC address.
     * @param cmdu_rx The received CMDU message to forward.
     * @return true on success and false otherwise.
     */
    virtual bool forward_cmdu(int fd, uint32_t iface_index, const sMacAddr &dst_mac,
                              const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx) = 0;

    /**
     * @brief Set the client name on EventLoop file descriptor handler.
     *
     * At a client connection it is not possible to know who that client is. This function allows
     * setting the file descriptor name in the EventLoop handlers later on when the client
     * identified so in case of EventLoop error or a client disconnect event it will be possible
     * to know on which client the event has occurred.
     *
     * @param fd File descriptor of the socket used by the connection.
     * @param client_name Client name
     * @return true on success, false otherwise.
     */
    virtual bool set_client_name(int fd, const std::string &client_name) = 0;

protected:
    /**
     * @brief Notifies a client-connected event.
     *
     * @param fd File descriptor of the socket connection.
     */
    void notify_client_connected(int fd) const
    {
        if (m_handlers.on_client_connected) {
            m_handlers.on_client_connected(fd);
        }
    }

    /**
     * @brief Notifies a client-disconnected event.
     *
     * @param fd File descriptor of the socket connection.
     */
    void notify_client_disconnected(int fd) const
    {
        if (m_handlers.on_client_disconnected) {
            m_handlers.on_client_disconnected(fd);
        }
    }

    /**
     * @brief Notifies a CMDU-received event.
     *
     * @param fd File descriptor of the socket connection the CMDU was received through.
     * @param iface_index Index of the network interface that the CMDU message was received on.
     * @param dst_mac Destination MAC address.
     * @param src_mac Source MAC address.
     * @param cmdu_rx The CMDU message received.
     */
    void notify_cmdu_received(int fd, uint32_t iface_index, const sMacAddr &dst_mac,
                              const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx) const
    {
        if (m_handlers.on_cmdu_received) {
            m_handlers.on_cmdu_received(fd, iface_index, dst_mac, src_mac, cmdu_rx);
        }
    }

private:
    /**
     * Set of event handler functions that are called back whenever a new event occurs on this
     * server.
     */
    EventHandlers m_handlers;
};

} // namespace beerocks

#endif // _BEEROCKS_CMDU_SERVER_H_
