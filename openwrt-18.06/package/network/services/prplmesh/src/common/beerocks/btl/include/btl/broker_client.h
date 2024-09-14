/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BTL_BROKER_CLIENT_H_
#define BTL_BROKER_CLIENT_H_

#include <tlvf/CmduMessageRx.h>
#include <tlvf/CmduMessageTx.h>
#include <tlvf/common/sMacAddr.h>

namespace beerocks {
namespace btl {

/**
 * @brief The broker client is a component to exchange messages with the transport process.
 *
 * This component acts as the client side end-point of the connection while the broker server
 * running in the transport process plays the server role.
 *
 * The broker client can be used to send CMDU messages to other devices in the network by sending
 * them to the local transport process, which in turn forwards such CMDU messages to remote
 * transport processes, running in other devices.
 *
 * The broker client can also be used to receive CMDU messages sent by other devices in the
 * network. Setting up the reception of CMDU messages is a two-step process. First, a CMDU-
 * received event handler function must be registered to tell the broker client where to call back
 * when a CMDU message is received. And second, a subscribe request must be sent to the server,
 * specifying the types of CMDU messages that the caller is interested in. From this moment on, the
 * broker server will forward all the CMDU messages that it receives from the network and that pass
 * the specified filter to the broker client, which in turn will call back the specified CMDU-
 * received event handler.
 *
 * Connection to the broker server can be unexpectedly closed if, for example, the transport process
 * dies. A connection-closed event handler function can be registered to get notified of this
 * situation and start a recovery procedure.
 */
class BrokerClient {
public:
    /**
     * @brief CMDU-received event handler function.
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
         * Handler function called back by the broker client to deal with CMDU-received event.
         *
         * If this handler is set, it will be called back whenever a CMDU message is received at the
         * server and forwarded to the client.
         */
        CmduReceivedHandler on_cmdu_received;

        /**
         * Handler function called back by the broker client to deal with connection-closed event.
         *
         * If this handler is set, it will be called back whenever the connection to the broker
         * server is closed (e.g.: when the server goes down because the transport process dies).
         * The handler may, for example, implement a recovery mechanism that includes reconnecting
         * to the server and creating another client.
         */
        ConnectionClosedHandler on_connection_closed;
    };

    /**
     * @brief Class destructor
     */
    virtual ~BrokerClient() = default;

    /**
     * @brief Sets the event handler functions.
     *
     * Sets the callback functions to be executed whenever an event occurs on this this component.
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
     * @brief Subscribes for the reception of a set of CMDU messages.
     *
     * Builds a subscription message including the filter for message types and sends it to the
     * server. Before calling this method, a handler to process received CMDU messages should have
     * been installed.
     *
     * @param msg_types List of CMDU types the caller is interested in. The maximum number of types
     * that can be specified is defined by
     * beerocks::transport::messages::SubscribeMessage::MAX_SUBSCRIBE_TYPES.
     * @return true on success and false otherwise
     */
    virtual bool subscribe(const std::set<ieee1905_1::eMessageType> &msg_types) = 0;

    /**
     * @brief Configures the transport process to use given network bridge.
     *
     * Builds a configuration message which include an interface name.
     * If @a iface_name is a bridge interface, the transport process will read the network
     * interfaces currently in the bridge and monitor the bridge for changes as interfaces are added
     * to and/or removed from the bridge.
     * If @a iface_name is not a bridge interface, the Tranport will add or remove it from the
     * its interfaces list according to the value of @a add.
     *
     * @param iface_name The interface name.
     * @param bridge_name The interfce bridge name if this interface is in a bridge.
     * If the interface is a bridge or outside a bridge, it should be empty.
     * @param is_bridge Is @a iface_name is a bridge interface.
     * @param add true to add the interface, otherwise to remove.
     *
     * @return true on success and false otherwise.
     */
    virtual bool configure_interfaces(const std::string &iface_name, const std::string &bridge_name,
                                      bool is_bridge, bool add) = 0;

    /**
     * @brief Configures the transport process to bind a given local bridge al_mac address.
     *
     * Builds a configuration message with the al_mac address of the bridge and sends it
     * to the server. The transport process will set the shipped mac address value.
     *
     * @param al_mac Mac address of the bridge to use in the transport process.
     *
     * @return true on success and false otherwise
     */
    virtual bool configure_al_mac(const sMacAddr &al_mac) = 0;

    /**
     * @brief Sends a CDMU message to the transport process for dispatching.
     *
     * Finalizes CMDU if not already finalized. Then builds a message including the CMDU and sends
     * it to the server, which in turn will send it to destination (or multicast).
     *
     * @param cmdu_tx CMDU message to send.
     * @param dst_mac Destination MAC address (must not be empty).
     * @param src_mac Source MAC address (must not be empty).
     * @param iface_index Index of the network interface to use (set to 0 to send on all available
     * interfaces).
     * @return true on success and false otherwise.
     */
    virtual bool send_cmdu(ieee1905_1::CmduMessageTx &cmdu_tx, const sMacAddr &dst_mac,
                           const sMacAddr &src_mac, uint32_t iface_index = 0) = 0;

    /**
     * @brief Forwards a CMDU message to the transport process for dispatching.
     *
     * Forwards a CMDU message received through a UDS socket to the transport process which in turn
     * will send it to destination (or multicast).
     *
     * @param cmdu_rx The received CMDU message to forward.
     * @param dst_mac Destination MAC address (must not be empty).
     * @param src_mac Source MAC address (must not be empty).
     * @param iface_index Index of the network interface to use (set to 0 to send on all available
     * interfaces).
     * @return true on success and false otherwise.
     */
    virtual bool forward_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx, const sMacAddr &dst_mac,
                              const sMacAddr &src_mac, uint32_t iface_index = 0) = 0;

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
     * component.
     */
    EventHandlers m_handlers;
};

} // namespace btl

} // namespace beerocks

#endif //BTL_BROKER_CLIENT_H_
