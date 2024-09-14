/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_event_loop.h>
#include <bcl/network/buffer_impl.h>
#include <bcl/network/netlink_event_listener_impl.h>

using namespace beerocks;

namespace beerocks {
namespace net {

static constexpr size_t netlink_buffer_size = 8192;

NetlinkEventListenerImpl::NetlinkEventListenerImpl(std::shared_ptr<Socket::Connection> connection,
                                                   std::shared_ptr<EventLoop> event_loop)
    : m_connection(connection), m_event_loop(event_loop)
{
    EventLoop::EventHandlers handlers;
    handlers.name    = "Netlink Event Listener";
    handlers.on_read = [&](int fd, EventLoop &loop) -> bool {
        BufferImpl<netlink_buffer_size> buffer;
        if (m_connection->receive(buffer) > 0) {
            parse(buffer);
        }

        return true;
    };

    m_event_loop->register_handlers(m_connection->socket()->fd(), handlers);
}

NetlinkEventListenerImpl::~NetlinkEventListenerImpl()
{
    m_event_loop->remove_handlers(m_connection->socket()->fd());
}

void NetlinkEventListenerImpl::parse(const Buffer &buffer) const
{
    const nlmsghdr *msg_hdr = reinterpret_cast<const nlmsghdr *>(buffer.data());
    size_t length           = buffer.length();

    while (NLMSG_OK(msg_hdr, length)) {

        notify_netlink_event(msg_hdr);

        msg_hdr = NLMSG_NEXT(msg_hdr, length);
    }
}

} // namespace net
} // namespace beerocks
