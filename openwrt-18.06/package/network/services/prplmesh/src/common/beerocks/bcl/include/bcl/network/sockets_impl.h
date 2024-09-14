/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_SOCKETS_IMPL_H_
#define BCL_NETWORK_SOCKETS_IMPL_H_

#include "file_descriptor_impl.h"
#include "sockets.h"

#include <bcl/beerocks_backport.h>
#include <bcl/beerocks_string_utils.h>
#include <bcl/network/network_utils.h>

#include <tlvf/common/sMacAddr.h>
#include <tlvf/tlvftypes.h>

#include <linux/rtnetlink.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <sys/un.h>

namespace beerocks {
namespace net {

/**
 * Abstract base class for all types of sockets: Raw, UDP, TCP, UDS, ...
 * This implementation class aggregates a FileDescriptor implementation so it has a file
 * descriptor. Methods overridden from FileDescriptor interface delegate on the aggregated
 * implementation.
 * Derived classes provide the file descriptor obtained with a call to socket(), using different
 * family, type and protocol parameters.
 * This class aggregates a FileDescriptor instead of inheriting from one of its implementations to
 * follow the principle of "Favor Aggregation over Inheritance".
 * See https://wiki.c2.com/?UseCompositionAndInterfacesWithoutClassInheritance
 */
class SocketAbstractImpl : public Socket {
public:
    /**
     * @brief Returns the socket file descriptor.
     *
     * @return Socket file descriptor.
     */
    int fd() override { return m_descriptor.fd(); }

    /**
     * @brief Sets the SO_REUSEADDR socket option.
     *
     * Indicates that the rules used in validating addresses supplied in a bind(2) call should allow
     * reuse of local addresses. For AF_INET sockets this means that a socket may bind, except when
     * there is an active listening socket bound to the address. When the listening socket is bound
     * to INADDR_ANY with a specific port then it is not possible to bind to this port for any local
     * address.
     *
     * What exactly does SO_REUSEADDR do?
     * On TCP server sockets, is both the simplest and the most effective option for reducing the
     * "address already in use" error when calling bind() to a specific port.
     * This socket option tells the kernel that even if this port is busy (in the TIME_WAIT state),
     * go ahead and reuse it anyway. If it is busy, but in another state, you will still get an
     * address already in use error. It is useful if your server has been shut down, and then
     * restarted right away while sockets are still active on its port.
     *
     * @param reuseaddr Flag set to true to allow reuse of local addresses and to false to disallow.
     * @return true on success and false otherwise.
     */
    bool setsockopt_reuseaddr(bool reuseaddr)
    {
        int optval = reuseaddr ? 1 : 0;
        if (0 != ::setsockopt(fd(), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
            LOG(ERROR) << "Unable to set the SO_REUSEADDR socket option for fd: " << fd()
                       << ", error: " << strerror(errno);
            return false;
        }

        return true;
    }

protected:
    /**
     * @brief Class constructor.
     *
     * Constructor is protected so only derived classes can call it.
     */
    explicit SocketAbstractImpl(int fd) : m_descriptor(fd) {}

private:
    /**
     * File descriptor (i.e.: wrapper to `int fd` that closes descriptor on destructor)
     */
    FileDescriptorImpl m_descriptor;
};

class UdsAddress : public Socket::Address {
public:
    explicit UdsAddress(const std::string &path = "")
    {
        m_address.sun_family = AF_UNIX;
        string_utils::copy_string(m_address.sun_path, path.c_str(), sizeof(m_address.sun_path));
        m_name.assign(path);
    }

    std::string path() const { return m_address.sun_path; }

    const struct sockaddr *sockaddr() const override
    {
        return reinterpret_cast<const struct sockaddr *>(&m_address);
    }
    const socklen_t &length() const override { return m_length; }
    socklen_t size() const override { return m_size; }
    const std::string &name() const override { return m_name; }

    static std::shared_ptr<UdsAddress> create_instance(const std::string &path)
    {
        // When no longer required, the UDS socket pathname should be deleted using unlink or remove.
        auto deleter = [path](UdsAddress *p) {
            if (p) {
                delete p;
            }
            unlink(path.c_str());
        };

        // Remove given path in case it exists
        unlink(path.c_str());

        // Create UDS address from given path (using custom deleter)
        return std::shared_ptr<UdsAddress>(new UdsAddress(path), deleter);
    }

private:
    sockaddr_un m_address  = {};
    socklen_t m_length     = sizeof(m_address);
    const socklen_t m_size = sizeof(m_address);
};

class InternetAddress : public Socket::Address {
public:
    explicit InternetAddress(uint16_t port = 0, uint32_t address = INADDR_ANY,
                             const std::string &name = {})
    {
        m_address.sin_family      = AF_INET;
        m_address.sin_addr.s_addr = address;
        m_address.sin_port        = htons(port);
        m_name                    = name + " (" + net::network_utils::ipv4_to_string(address) + ")";
    }

    const struct sockaddr *sockaddr() const override
    {
        return reinterpret_cast<const struct sockaddr *>(&m_address);
    }
    const socklen_t &length() const override { return m_length; }
    socklen_t size() const override { return m_size; }

    uint16_t port() const { return ntohs(m_address.sin_port); }
    uint32_t address() const { return m_address.sin_addr.s_addr; }
    const std::string &name() const override { return m_name; }

private:
    sockaddr_in m_address  = {};
    socklen_t m_length     = sizeof(m_address);
    const socklen_t m_size = sizeof(m_address);
};

class LinkLevelAddress : public Socket::Address {
public:
    LinkLevelAddress(uint32_t iface_index, const sMacAddr &mac)
    {
        m_address.sll_family  = AF_PACKET;
        m_address.sll_ifindex = iface_index;
        m_address.sll_halen   = sizeof(sMacAddr);
        tlvf::mac_to_array(mac, m_address.sll_addr);
        m_name = "iface_idx " + std::to_string(iface_index) + ": " + tlvf::mac_to_string(mac);
    }

    const struct sockaddr *sockaddr() const override
    {
        return reinterpret_cast<const struct sockaddr *>(&m_address);
    }
    const socklen_t &length() const override { return m_length; }
    socklen_t size() const override { return m_size; }
    const std::string &name() const override { return m_name; }

private:
    sockaddr_ll m_address  = {};
    socklen_t m_length     = sizeof(m_address);
    const socklen_t m_size = sizeof(m_address);
};

class NetlinkAddress : public Socket::Address {
public:
    explicit NetlinkAddress(uint32_t groups = 0)
    {
        m_address.nl_family = AF_NETLINK;
        m_address.nl_groups = groups;
        m_name              = std::string("nl group ") + std::to_string(groups);
    }

    const struct sockaddr *sockaddr() const override
    {
        return reinterpret_cast<const struct sockaddr *>(&m_address);
    }
    const socklen_t &length() const override { return m_length; }
    socklen_t size() const override { return m_size; }
    const std::string &name() const override { return m_name; }

private:
    sockaddr_nl m_address  = {};
    socklen_t m_length     = sizeof(m_address);
    const socklen_t m_size = sizeof(m_address);
};

/**
 * This class is a wrapper for the socket file descriptor obtained with the accept() system call.
 */
class ConnectedSocket : public SocketAbstractImpl {
public:
    explicit ConnectedSocket(int fd) : SocketAbstractImpl(fd) {}
};

class RawSocket : public SocketAbstractImpl {
public:
    explicit RawSocket(uint16_t protocol = ETH_P_ALL)
        : SocketAbstractImpl(socket(AF_PACKET, SOCK_RAW, htons(protocol)))
    {
    }
};

class UdpSocket : public SocketAbstractImpl {
public:
    UdpSocket() : SocketAbstractImpl(socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) {}
};

class TcpSocket : public SocketAbstractImpl {
public:
    TcpSocket() : SocketAbstractImpl(socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) {}
};

class UdsSocket : public SocketAbstractImpl {
public:
    /**
     * Class constructor.
     *
     * Creates a Unix Domain Socket for exchanging data between processes executing on the same
     * host operating system.
     *
     * Valid socket types in the UNIX domain are:
     * - SOCK_STREAM (compare to TCP) – for a stream-oriented socket.
     * - SOCK_DGRAM (compare to UDP) – for a datagram-oriented socket that preserves message
     * boundaries (as on most UNIX implementations, UNIX domain datagram sockets are always
     * reliable and don't reorder datagrams).
     * - SOCK_SEQPACKET (compare to SCTP) – for a sequenced-packet socket that is connection-
     * oriented, preserves message boundaries, and delivers messages in the order that they were
     * sent.
     *
     * Stream socket allows for reading an arbitrary number of bytes, but still preserving byte
     * sequence. In other words, a sender might write 4K of data to the socket, and the receiver
     * can consume that data byte by byte. The other way around is true too - the sender can write
     * several small messages to the socket that the receiver can consume in one read. Stream
     * socket does not preserve message boundaries.
     *
     * Datagram socket, on the other hand, does preserve these boundaries - one write by the
     * sender always corresponds to one read by the receiver (even if the receiver's buffer given
     * to read(2) or recv(2) is smaller than that message).
     *
     * So if your application protocol has small messages with known upper bound on message size
     * you are better off with SOCK_DGRAM since that's easier to manage.
     *
     * If your protocol calls for arbitrary long message payloads, or is just an unstructured
     * stream (like raw audio or something), then pick SOCK_STREAM and do the required buffering.
     *
     * @param type Socket type (i.e.: communication style).
     */
    explicit UdsSocket(int type = SOCK_STREAM) : SocketAbstractImpl(socket(AF_UNIX, type, 0)) {}
};

class NetlinkSocket : public SocketAbstractImpl {
protected:
    explicit NetlinkSocket(uint16_t protocol)
        : SocketAbstractImpl(socket(AF_NETLINK, SOCK_RAW, protocol))
    {
    }
};

class NetlinkRouteSocket : public NetlinkSocket {
public:
    NetlinkRouteSocket() : NetlinkSocket(NETLINK_ROUTE) {}
};

/**
 * This class implements the Socket::Connection interface with methods that wrap the system calls
 * to send and receive both bytes and packets in stream-oriented and packet-oriented sockets
 * respectively.
 */
class SocketConnectionImpl : public Socket::Connection {
public:
    /**
     * @brief Class constructor.
     *
     * @param socket Underlying socket used by this connection.
     */
    explicit SocketConnectionImpl(std::shared_ptr<Socket> socket) : m_socket(socket) {}

    /**
     * @brief Returns the underlying socket used by this connection.
     *
     * @see Connection::socket
     */
    std::shared_ptr<Socket> socket() override { return m_socket; }

    /**
     * @brief Receives data through the socket connection.
     *
     * @see Connection::receive
     *
     * This implementation uses the recv() system call.
     */
    int receive(Buffer &buffer) override
    {
        if (buffer.length() >= buffer.size()) {
            // Buffer is full
            return -1;
        }

        int result = ::recv(m_socket->fd(), buffer.data() + buffer.length(),
                            buffer.size() - buffer.length(), MSG_DONTWAIT);
        if (result > 0) {
            buffer.length() += static_cast<size_t>(result);
        }

        return result;
    }

    /**
     * @brief Receives data through the socket connection.
     *
     * @see Connection::receive_from
     *
     * This implementation uses the recvfrom() system call.
     */
    int receive_from(Buffer &buffer, Socket::Address &address) override
    {
        address.length() = address.size();

        int result = ::recvfrom(m_socket->fd(), buffer.data(), buffer.size(), MSG_DONTWAIT,
                                address.sockaddr(), &address.length());
        if (result >= 0) {
            buffer.length() = static_cast<size_t>(result);
        }

        return result;
    }

    /**
     * @brief Sends data through the socket connection.
     *
     * @see Connection::send
     *
     * This implementation uses the send() system call.
     */
    int send(const Buffer &buffer) override
    {
        return ::send(m_socket->fd(), buffer.data(), buffer.length(), MSG_NOSIGNAL);
    }

    /**
     * @brief Sends data through the socket connection.
     *
     * @see Connection::send_to
     *
     * This implementation uses the sendto() system call.
     */
    int send_to(const Buffer &buffer, const Socket::Address &address) override
    {
        return ::sendto(m_socket->fd(), buffer.data(), buffer.length(), 0, address.sockaddr(),
                        address.length());
    }

private:
    /**
     * Connected socket used by this connection object.
     */
    std::shared_ptr<Socket> m_socket;
};

class ServerSocketAbstractImpl : public ServerSocket {
public:
    /**
     * @brief Returns the underlying socket used by this server.
     *
     * @see ServerSocket::socket
     */
    std::shared_ptr<Socket> socket() override { return m_socket; }

    /**
     * @brief Binds address to the socket.
     *
     * This method is a wrapper around the `bind` system call.
     *
     * @param address Socket address to assign to the socket.
     * @return true on success and false otherwise.
     */
    bool bind(const Socket::Address &address)
    {
        if (0 != ::bind(m_socket->fd(), address.sockaddr(), address.length())) {
            LOG(ERROR) << "Unable to bind server socket: " << strerror(errno);
            return false;
        }

        return true;
    }

    /**
     * @brief Listens for incoming connection requests.
     *
     * This method is a wrapper around the `listen` system call.
     *
     * @param backlog Maximum length to which the queue of pending connections for the socket may grow.
     * @return true on success and false otherwise.
     */
    bool listen(int backlog = 1)
    {
        if (0 != ::listen(m_socket->fd(), backlog)) {
            LOG(ERROR) << "Unable to listen for incoming connections: " << strerror(errno);
            return false;
        }

        return true;
    }

    /**
     * @brief Accepts an incoming connection request.
     *
     * This method is a wrapper around the `accept` system call.
     *
     * @param address Address of the peer socket.
     * @return Accepted incoming connection and nullptr on error.
     */
    std::unique_ptr<Socket::Connection> accept(Socket::Address &address) override
    {
        m_socket->m_name     = address.name() + " server";
        address.length()     = address.size();
        int connected_socket = ::accept(m_socket->fd(), address.sockaddr(), &address.length());
        if (FileDescriptor::invalid_descriptor == connected_socket) {
            LOG(ERROR) << "Unable to accept socket connection: " << strerror(errno);
            return nullptr;
        }

        return std::make_unique<SocketConnectionImpl>(
            std::make_shared<ConnectedSocket>(connected_socket));
    }

protected:
    /**
     * @brief Class constructor.
     *
     * @param socket Underlying socket used by this server socket.
     */
    explicit ServerSocketAbstractImpl(std::shared_ptr<Socket> socket) : m_socket(socket) {}

    /**
     * Underlying socket used by this server socket.
     */
    std::shared_ptr<Socket> m_socket;
};

class ClientSocketAbstractImpl : public ClientSocket {
public:
    /**
     * @brief Returns the underlying socket used by this client.
     *
     * @see ClientSocket::socket
     */
    std::shared_ptr<Socket> socket() override { return m_socket; }

    /**
     * @brief Binds address to the socket.
     *
     * This method is a wrapper around the `bind` system call.
     *
     * @param address Socket address to assign to the socket.
     * @return true on success and false otherwise.
     */
    bool bind(const Socket::Address &address)
    {
        if (0 != ::bind(m_socket->fd(), address.sockaddr(), address.length())) {
            LOG(ERROR) << "Unable to bind client socket: " << strerror(errno);
            return false;
        }

        return true;
    }

    /**
     * @brief Connects to a server socket.
     *
     * This method is a wrapper around the `connect` system call.
     *
     * @param address Address of the server socket.
     * @return Connection established and nullptr on error.
     */
    std::unique_ptr<Socket::Connection> connect(const Socket::Address &address) override
    {
        m_socket->m_name = address.name() + " client";
        if (0 != ::connect(m_socket->fd(), address.sockaddr(), address.length())) {
            LOG(ERROR) << "Unable to connect client socket: " << strerror(errno);
            return nullptr;
        }

        return std::make_unique<SocketConnectionImpl>(m_socket);
    }

protected:
    /**
     * @brief Class constructor.
     *
     * @param socket Underlying socket used by this client socket.
     */
    explicit ClientSocketAbstractImpl(std::shared_ptr<Socket> socket) : m_socket(socket) {}

    /**
     * Underlying socket used by this client socket.
     */
    std::shared_ptr<Socket> m_socket;
};

template <class SocketType> class ServerSocketImpl : public ServerSocketAbstractImpl {
public:
    explicit ServerSocketImpl(std::shared_ptr<SocketType> socket) : ServerSocketAbstractImpl(socket)
    {
    }
};

template <class SocketType> class ClientSocketImpl : public ClientSocketAbstractImpl {
public:
    explicit ClientSocketImpl(std::shared_ptr<SocketType> socket) : ClientSocketAbstractImpl(socket)
    {
    }
};

class UdsServerSocket : public ServerSocketImpl<UdsSocket> {
public:
    static std::unique_ptr<ServerSocket> create_instance(const UdsAddress &address)
    {
        // Create UDS socket
        auto socket = std::make_shared<UdsSocket>();

        LOG_IF(!socket, FATAL) << "Unable to create UDS socket!";

        socket->m_name = address.name() + " server";

        // Create UDS server socket to listen for and accept incoming connections from clients.
        auto server_socket = std::make_unique<ServerSocketImpl<UdsSocket>>(socket);
        if (!server_socket) {
            LOG(ERROR) << "Unable to create server socket";
            return nullptr;
        }

        // Bind server socket to given UDS address
        if (!server_socket->bind(address)) {
            LOG(ERROR) << "Unable to bind server socket to UDS address: '" << address.path() << "'";
            return nullptr;
        }

        // Listen for incoming connection requests
        if (!server_socket->listen()) {
            LOG(ERROR) << "Unable to listen for connection requests at UDS address: '"
                       << address.path() << "'";
            return nullptr;
        }

        return server_socket;
    }
};

class TcpServerSocket : public ServerSocketImpl<TcpSocket> {
public:
    static std::unique_ptr<ServerSocket> create_instance(const InternetAddress &address)
    {
        // Create TCP socket
        auto socket = std::make_shared<beerocks::net::TcpSocket>();

        // Create TCP server socket to listen for and accept incoming connections from clients.
        using TcpServerSocket = beerocks::net::ServerSocketImpl<beerocks::net::TcpSocket>;
        auto server_socket    = std::make_unique<TcpServerSocket>(socket);
        if (!server_socket) {
            LOG(ERROR) << "Unable to create server socket";
            return nullptr;
        }

        // Allow reuse of local addresses.
        // (to avoid the "address already in use" error when calling bind() on a TCP server socket
        // if server has been shut down and restarted right away).
        if (!socket->setsockopt_reuseaddr(true)) {
            LOG(ERROR) << "Unable to set socket option";
            return nullptr;
        }

        // Bind server socket to given TCP address
        if (!server_socket->bind(address)) {
            LOG(ERROR) << "Unable to bind server socket to TCP address at port: " << address.port();
            return nullptr;
        }

        // Listen for incoming connection requests
        if (!server_socket->listen()) {
            LOG(ERROR) << "Unable to listen for connection requests at TCP address at port: "
                       << address.port();
            return nullptr;
        }

        return server_socket;
    }

    static std::unique_ptr<ServerSocket> create_instance(uint16_t port)
    {
        return create_instance(InternetAddress(port));
    }
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_SOCKETS_IMPL_H_ */
