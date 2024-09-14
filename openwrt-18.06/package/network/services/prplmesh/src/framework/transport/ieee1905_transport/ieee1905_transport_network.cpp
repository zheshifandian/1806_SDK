/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "ieee1905_transport.h"

#include <arpa/inet.h>
#include <bpl/bpl_cfg.h>
#include <iomanip>
#include <linux/filter.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

namespace beerocks {
namespace transport {

// Use transport messaging classes
using namespace beerocks::transport::messages;

void Ieee1905Transport::update_network_interfaces(
    const std::map<std::string, NetworkInterface> &added_updated_network_interfaces,
    const std::map<std::string, NetworkInterface> &removed_network_interfaces)
{

    for (const auto &remove_net_if_element : removed_network_interfaces) {
        auto &ifname = remove_net_if_element.second.ifname;
        MAPF_INFO("interface " << ifname << " is no longer used.");
        auto iface_it = network_interfaces_.find(ifname);
        if (iface_it == network_interfaces_.end()) {
            // Request to remove interface which was not added, ignore.
            continue;
        }
        auto &network_interface = iface_it->second;

        if (network_interface.fd) {
            m_event_loop->remove_handlers(network_interface.fd->getSocketFd());
            network_interface.fd = nullptr;
        }

        update_network_interface(network_interface.bridge_name, network_interface.ifname, false);
    }

    // Add new interfaces or update existing ones.
    for (const auto &entry : added_updated_network_interfaces) {
        auto &updated_network_interface = entry.second;

        auto &bridge_name = updated_network_interface.bridge_name;
        auto &ifname      = updated_network_interface.ifname;
        auto is_bridge    = updated_network_interface.is_bridge;
        LOG(DEBUG) << "Adding iface " << ifname << ", is_bridge=" << is_bridge
                   << ", bridge_name=" << bridge_name;
        update_network_interface(bridge_name, ifname, true, is_bridge);
    }
}

bool Ieee1905Transport::update_network_interface(const std::string &bridge_name,
                                                 const std::string &ifname, bool iface_added,
                                                 bool is_bridge)
{
    if (iface_added) {
        // Add the new interface to network_interfaces_
        unsigned int if_index = if_nametoindex(ifname.c_str());
        if (if_index == 0) {
            MAPF_ERR("Failed to get index for interface " << ifname);
            remove_network_interface(ifname);
            return false;
        }

        auto &interface =
            network_interfaces_[ifname]; // Creates the interface object if it doesn't exist yet..
        interface.ifname      = ifname;
        interface.bridge_name = bridge_name;
        interface.is_bridge   = is_bridge;

        // must be called before open_interface_socket (address is used for packet filtering)
        if (!get_interface_mac_addr(if_index, interface.addr)) {
            MAPF_ERR("Failed to get address of interface " << ifname << " with index " << if_index
                                                           << ".");
            remove_network_interface(ifname);
            return false;
        }

        LOG(INFO) << "Interface " << ifname << " with index " << if_index << " and address "
                  << tlvf::mac_from_array(interface.addr) << " is used.";

        // If the interface is not up and running, then do not activate it yet, otherwise operations
        // on the socket created for the interface would fail.
        // The interface will be activated later, when its state changes to up.
        bool iface_state;
        if (!m_interface_state_manager->read_state(ifname, iface_state)) {
            LOG(ERROR) << "Failed to read state of interface " << ifname << ".";
            remove_network_interface(ifname);
            return false;
        }
        if (!iface_state) {
            LOG(INFO) << "Interface " << ifname << " is not up and running.";
            return true;
        }

        activate_interface(interface);
    } else {
        MAPF_INFO("Removing interface " << ifname << " from the transport");
        remove_network_interface(ifname);
    }
    return true;
}

bool Ieee1905Transport::remove_network_interface(const std::string &ifname)
{
    LOG(DEBUG) << "Removing iface " << ifname << " monitoring";

    auto interface = network_interfaces_.find(ifname);
    if (interface == network_interfaces_.end()) {
        MAPF_WARN("Can't remove interface " << ifname
                                            << " from transport since it's not on the list.");
        return false;
    }
    LOG(DEBUG) << "iface " << ifname << " iterator found";

    auto &network_interface = interface->second;

    LOG(DEBUG) << "Deactivating iface " << ifname;
    deactivate_interface(network_interface);

    LOG(DEBUG) << "Removing iface " << ifname << " from network_interfaces_ container";
    network_interfaces_.erase(interface);
    LOG(DEBUG) << "Remove iface " << ifname << " monitoring finished";

    return true;
}

bool Ieee1905Transport::open_interface_socket(NetworkInterface &interface)
{
    if (interface.fd) {
        interface.fd = nullptr;
    }

    // Note to developer: The current implementation uses AF_PACKET socket with SOCK_RAW protocol which means we receive
    // and send packets with the Ethernet header included in the buffer. Please consider changing
    // implementation to use SOCK_DGRAM protocol (without L2 header handling)

    // open packet raw socket - see man packet(7) https://linux.die.net/man/7/packet
    int sockfd;
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        MAPF_ERR("cannot open raw socket, error: \"" << strerror(errno) << "\" (" << errno << ").");
        return false;
    }

    // the interface can be used by other processes
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        MAPF_ERR("cannot set socket option SO_REUSEADDR for FD ("
                 << sockfd << "), error: \"" << strerror(errno) << "\" (" << errno << ").");
        close(sockfd);
        return false;
    }

    // bind to specified interface - note that we cannot use SO_BINDTODEVICE sockopt as it does not support AF_PACKET sockets
    struct sockaddr_ll sockaddr;
    memset(&sockaddr, 0, sizeof(struct sockaddr_ll));
    sockaddr.sll_family   = AF_PACKET;
    sockaddr.sll_protocol = htons(ETH_P_ALL);
    sockaddr.sll_ifindex  = if_nametoindex(interface.ifname.c_str());
    if (bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
        MAPF_ERR("cannot bind socket to interface for FD ("
                 << sockfd << "), error: \"" << strerror(errno) << "\" (" << errno << ").");
        close(sockfd);
        return false;
    }

    interface.fd = std::make_shared<Socket>(sockfd);
    LOG_IF(!sockfd, FATAL) << "Failed creating new Socket for FD (" << sockfd << ")";

    MAPF_DBG("Raw socket on interface " << interface.ifname << " opened with FD (" << sockfd
                                        << ").");

    attach_interface_socket_filter(interface);

    return true;
}

bool Ieee1905Transport::attach_interface_socket_filter(NetworkInterface &interface)
{
    int fd = interface.fd->getSocketFd();

    auto management_mode = bpl::cfg_get_management_mode();
    LOG_IF((management_mode < 0), ERROR) << "Failed to get management mode";

    // For non-EasyMesh mode the promiscuous should not be set since it may propogate control messages
    // to outside of the device and may affect some applications behavior (e.g. DNAT).
    if (management_mode != BPL_MGMT_MODE_NOT_MULTIAP) {

        // 1st step is to put the interface in promiscuous mode.
        // promiscuous mode is required since we expect to receive packets destined to
        // the AL MAC address (which is different than the interfaces HW address)
        struct packet_mreq mr = {0};
        mr.mr_ifindex         = if_nametoindex(interface.ifname.c_str());
        mr.mr_type            = PACKET_MR_PROMISC;
        if (setsockopt(fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
            MAPF_ERR("Interface cannot be put in promiscuous mode for FD ("
                     << fd << "), error: \"" << strerror(errno) << "\" (" << errno << ").");
            return false;
        }
    }

    // This BPF is designed to accepts the following packets:
    // - IEEE1905 multicast packets (with IEEE1905 Multicast Address [01:80:c2:00:00:13] set as destination address)
    // - LLDP multicast packets (with LLDP Multicast Address as destination address)
    // - IEEE1905 unicast packets (with either this devices' AL MAC address or the interface's HW address set as destination address)
    //
    // BPF template is generated using the following command:
    // tcpdump -dd '(ether proto 0x893a and (ether dst 01:80:c2:00:00:13 or ether dst 11:22:33:44:55:66 or ether dst 77:88:99:aa:bb:cc)) or (ether proto 0x88cc and ether dst 01:80:c2:00:00:0e)'
    //
    // The two dummy addresses in this filter 11:22... and 77:88... will be replaced in runtime with the AL MAC address and the interface's HW address
    struct sock_filter code[17] = {
        {0x28, 0, 0, 0x0000000c}, {0x15, 0, 8, 0x0000893a}, {0x20, 0, 0, 0x00000002},
        {0x15, 9, 0, 0xc2000013}, {0x15, 0, 2, 0x33445566}, // 4: replace with AL MAC Addr [2..5]
        {0x28, 0, 0, 0x00000000}, {0x15, 8, 9, 0x00001122}, // 6: replace with AL MAC Addr [0..1]
        {0x15, 0, 8, 0x99aabbcc},                           // 7: replace with IF MAC Addr [2..5]
        {0x28, 0, 0, 0x00000000}, {0x15, 5, 6, 0x00007788}, // 9: replace with IF MAC Addr [0..1]
        {0x15, 0, 5, 0x000088cc}, {0x20, 0, 0, 0x00000002}, {0x15, 0, 3, 0xc200000e},
        {0x28, 0, 0, 0x00000000}, {0x15, 0, 1, 0x00000180}, {0x6, 0, 0, 0x0000ffff},
        {0x6, 0, 0, 0x00000000},
    };

    // Replace dummy values with AL MAC
    code[4].k = (uint32_t(al_mac_addr_[2]) << 24) | (uint32_t(al_mac_addr_[3]) << 16) |
                (uint32_t(al_mac_addr_[4]) << 8) | (uint32_t(al_mac_addr_[5]));
    code[6].k = (uint32_t(al_mac_addr_[0]) << 8) | (uint32_t(al_mac_addr_[1]));

    // Replace dummy values with the Interface MAC
    code[7].k = (uint32_t(interface.addr[2]) << 24) | (uint32_t(interface.addr[3]) << 16) |
                (uint32_t(interface.addr[4]) << 8) | (uint32_t(interface.addr[5]));
    code[9].k = (uint32_t(interface.addr[0]) << 8) | (uint32_t(interface.addr[1]));

    // BPF filter structure
    struct sock_fprog bpf = {.len = (sizeof(code) / sizeof((code)[0])), .filter = code};

    // Attach the filter
    MAPF_DBG("Attaching filter on iface = '" << interface.ifname << "' (" << fd << ")");
    if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf)) == -1) {
        MAPF_ERR("Failed attaching filter for '" << interface.ifname << "': " << strerror(errno));
        return false;
    }

    return true;
}

void Ieee1905Transport::handle_interface_state_change(const std::string &ifname, bool is_active)
{
    auto it = network_interfaces_.find(ifname);
    if (it == network_interfaces_.end()) {
        MAPF_INFO("Ignoring event from untracked interface " << ifname << ".");
        return;
    }
    auto &interface = it->second;

    MAPF_INFO("Interface state change - interface " << ifname << " " << (is_active ? "up" : "down")
                                                    << ".");

    if (is_active) {
        activate_interface(interface);
    } else {
        deactivate_interface(interface);
    }
}

void Ieee1905Transport::handle_bridge_state_change(const std::string &bridge_name,
                                                   const std::string &iface_name, bool iface_added)
{
    MAPF_INFO("Bridge state change - interface "
              << iface_name << " " << (iface_added ? "added to" : "removed from") << " bridge "
              << bridge_name << ".");

    update_network_interface(bridge_name, iface_name, iface_added, false);
}

void Ieee1905Transport::deactivate_interface(NetworkInterface &interface, bool remove_handlers)
{
    if (!interface.fd) {
        MAPF_INFO("interface " << interface.ifname << " socket is nullptr");
        return;
    }

    MAPF_INFO("Deactivating interface " << interface.ifname << " start");

    // The bridge interface is not used for receiving but for sending packets only. Since no
    // event handlers were registered when the socket was open, neither they have to be removed
    // when the socket is closed.
    if (!interface.is_bridge) {
        // If requested, remove event handlers for the connected socket
        if (remove_handlers) {
            m_event_loop->remove_handlers(interface.fd->getSocketFd());
        }
    }
    MAPF_INFO("Closing interface " << interface.ifname
                                   << " socket fd=" << interface.fd->getSocketFd());
    close(interface.fd->getSocketFd());
    MAPF_INFO("FD " << interface.fd->getSocketFd() << " closed");
    interface.fd = nullptr;
    MAPF_INFO("Deactivating interface " << interface.ifname << " finished");
}

void Ieee1905Transport::activate_interface(NetworkInterface &interface)
{
    if (interface.fd) {
        return;
    }

    MAPF_INFO("Activating interface " << interface.ifname << ".");

    // Open a socket on the interface to send/receive packets through it.
    if (!open_interface_socket(interface)) {
        MAPF_ERR("cannot open network interface " << interface.ifname << ".");
        return;
    }

    std::string socket_name;
    if (!interface.bridge_name.empty()) {
        socket_name.assign(interface.bridge_name).append(":");
    }
    socket_name.append(interface.ifname).append(" Socket");

    // Handle network events, but not for the bridge which is used for sending only.
    if (!interface.is_bridge) {
        EventLoop::EventHandlers handlers = {
            // Handlers name
            .name = socket_name,

            // Accept incoming connections
            .on_read =
                [&](int fd, EventLoop &loop) {
                    LOG(DEBUG) << "Incoming message on interface " << interface.ifname << " FD ("
                               << fd << ")";
                    handle_interface_pollin_event(fd);
                    return true;
                },

            // Not implemented
            .on_write      = nullptr,
            .on_disconnect = nullptr,

            // Handle interface errors
            .on_error =
                [&](int fd, EventLoop &loop) {
                    std::string error_message;

                    int error        = 0;
                    socklen_t errlen = sizeof(error);
                    if (0 == getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen)) {
                        error_message = ": \"" + std::string(strerror(error)) + "\" (" +
                                        std::to_string(error) + ")";
                    }

                    LOG(ERROR) << "Error on FD (" << fd << ")" << error_message
                               << ". Disabling interface " << interface.ifname;

                    deactivate_interface(interface, false);
                    return true;
                },
        };
        m_event_loop->register_handlers(interface.fd->getSocketFd(), handlers);
    }
}

void Ieee1905Transport::handle_interface_pollin_event(int fd)
{
    if (fd < 0) {
        MAPF_ERR("Illegal file descriptor FD (" << fd << ")");
        return;
    }

    // Note to developer: add support for VLAN ethernet header (if required)?

    uint8_t buf[ETH_FRAME_LEN];
    struct sockaddr_ll addr;
    socklen_t addr_len = sizeof(addr);
    ssize_t len = recvfrom(fd, buf, sizeof(buf), MSG_DONTWAIT | MSG_TRUNC, (struct sockaddr *)&addr,
                           &addr_len);
    if (len == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
        return;
    }
    if (len == -1) {
        MAPF_ERR("cannot read from socket with FD (" << fd << "), error: \"" << strerror(errno)
                                                     << "\" (" << errno << ").");
        return;
    }
    if (len < (ssize_t)sizeof(struct ether_header)) {
        MAPF_WARN("received packet smaller than ethernet header size (dropped).");
        return;
    }
    if (len > int(sizeof(buf))) {
        MAPF_WARN("received oversized packet (truncated).");
        len = sizeof(buf);
    }

    // convert packet to internal data structure for further handling
    struct ether_header *eh = (struct ether_header *)buf;
    struct Packet packet;
    packet.dst_if_type  = CmduRxMessage::IF_TYPE_NONE;
    packet.dst_if_index = 0;
    packet.src_if_type  = CmduRxMessage::IF_TYPE_NET;
    packet.src_if_index = (unsigned int)addr.sll_ifindex;
    packet.dst          = tlvf::mac_from_array(eh->ether_dhost);
    packet.src          = tlvf::mac_from_array(eh->ether_shost);
    packet.ether_type   = ntohs(eh->ether_type);
    packet.header       = {.iov_base = buf, .iov_len = sizeof(struct ether_header)};
    packet.payload      = {.iov_base = buf + sizeof(struct ether_header),
                      .iov_len  = len - sizeof(struct ether_header)};

    counters_[CounterId::INCOMMING_NETWORK_PACKETS]++;
    handle_packet(packet);
}

bool Ieee1905Transport::get_interface_mac_addr(unsigned int if_index, uint8_t *addr)
{
    int sockfd;
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
        MAPF_ERR("cannot open raw socket, error: \"" << strerror(errno) << "\" (" << errno << ").");
        return false;
    }

    struct ifreq ifr = {};

    if (!if_indextoname(if_index, ifr.ifr_name)) {
        MAPF_ERR("cannot find name of interface  " << if_index << ".");
        close(sockfd);
        return false;
    }

    if ((ioctl(sockfd, SIOCGIFHWADDR, &ifr)) < 0) {
        MAPF_ERR("raw socket SIOCGIFHWADDR ioctl failed, error: \"" << strerror(errno) << "\" ("
                                                                    << errno << ").");
        close(sockfd);
        return false;
    }
    std::copy_n(ifr.ifr_hwaddr.sa_data, ETH_ALEN, addr);

    MAPF_DBG("address of interface "
             << if_index << " is " << std::hex << std::setfill('0') << std::setw(2)
             << (unsigned)addr[0] << ":" << std::hex << std::setfill('0') << std::setw(2)
             << (unsigned)addr[1] << ":" << std::hex << std::setfill('0') << std::setw(2)
             << (unsigned)addr[2] << ":" << std::hex << std::setfill('0') << std::setw(2)
             << (unsigned)addr[3] << ":" << std::hex << std::setfill('0') << std::setw(2)
             << (unsigned)addr[4] << ":" << std::hex << std::setfill('0') << std::setw(2)
             << (unsigned)addr[5] << "." << std::dec);

    close(sockfd);
    return true;
}

bool Ieee1905Transport::send_packet_to_network_interface(unsigned int if_index, Packet &packet)
{
    std::string ifname = if_index2name(if_index);
    if (ifname.empty()) {
        MAPF_ERR("Failed to get interface name for index " << if_index);
        return false;
    }
    MAPF_DBG("sending packet on interface " << ifname << ".");

    if (!network_interfaces_.count(ifname)) {
        MAPF_ERR("un-tracked interface " << ifname << ".");
        return false;
    }

    if (!network_interfaces_[ifname].fd) {
        LOG(ERROR) << "Invalid fd for iface: " << ifname;
        return false;
    }

    counters_[CounterId::OUTGOING_NETWORK_PACKETS]++;

    struct ether_header eh;
    tlvf::mac_to_array(packet.dst, eh.ether_dhost);
    tlvf::mac_to_array(packet.src, eh.ether_shost);
    eh.ether_type = htons(packet.ether_type);

    packet.header = {.iov_base = &eh, .iov_len = sizeof(eh)};

    int fd             = network_interfaces_[ifname].fd->getSocketFd();
    struct iovec iov[] = {packet.header, packet.payload};
    int n              = writev(fd, iov, sizeof(iov) / sizeof(struct iovec));

    // Clear the packet header to prevent leaking locally allocated stack pointer
    packet.header = {.iov_base = nullptr, .iov_len = sizeof(eh)};
    if (size_t(n) != sizeof(eh) + packet.payload.iov_len) {
        MAPF_ERR("cannot write to socket, error: \"" << strerror(errno) << "\" (" << errno << ").");
        return false;
    }

    return true;
}

void Ieee1905Transport::set_al_mac_addr(const uint8_t *addr)
{
    if (!addr)
        return;

    std::copy_n(addr, ETH_ALEN, al_mac_addr_);

    // refresh packet filtering on all active interfaces to use the new AL MAC address
    for (auto it = network_interfaces_.begin(); it != network_interfaces_.end(); ++it) {
        auto &network_interface = it->second;

        if (network_interface.fd) {
            attach_interface_socket_filter(network_interface);
        }
    }
}

} // namespace transport
} // namespace beerocks

#if 0
    static const uint8_t ieee1905_multicast_address[ETH_ALEN] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x13 }; // 01:80:c2:00:00:13
    static const uint8_t lldp_multicast_address[ETH_ALEN]     = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e }; // 01:80:c2:00:00:0e

    // put interface in all-multicast/promiscuous mode
    struct ifreq ifr;
    strncpy(ifr.ifr_name, ifName, IFNAMSIZ-1);
    if ((ioctl(network_interfaces_[if_index].fd, SIOCGIFFLAGS, &ifr) < 0) {
        MAPF_ERR("raw socket SIOCGIFFLAGS ioctl failed (get flags)\"" << strerror(errno) << "\" (" << errno << ").");
        return false;
    }
    ifr.ifr_flags |= IFF_PROMISC;
    ifr.ifr_flags |= IFF_ALLMULTI;
    if ((ioctl(network_interfaces_[if_index].fd, SIOCSIFFLAGS, &ifr) < 0) {
        MAPF_ERR("raw socket SIOCSIFFLAGS ioctl failed (set flags)\"" << strerror(errno) << "\" (" << errno << ").");
        return false;
    }

    // promisc/multicast ioctl alternative code (uses specific multicast addresses - but not tested)
    struct packet_mreq mr;
    memset(&mr, 0, sizeof(mr));
    mr.mr_ifindex = addr.sll_ifindex;
    mr.mr_type = PACKET_MR_MULTICAST;
    mr.mr_alen = 6;
    memcpy(mr.mr_address, ieee1905_multicast_address, 6);
    if (setsockopt(sockfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        perror("PACKET_ADD_MEMBERSHIP");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    memcpy(mr.mr_address, lldp_multicast_address, 6);
    if (setsockopt(sockfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        perror("PACKET_ADD_MEMBERSHIP");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // hand-made filter
    struct sock_filter ieee1905_filter[] = {
        // should we add support for 802.1Q (VLAN/QoS tagging)
        BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 12),                /* load the EtherType field */
        BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, ETH_1905_1, 0, 1), /* if EtherType != IEEE1905 skip next instr. */
        BPF_STMT(BPF_RET+BPF_K, (u_int)-1),                /* return max int (the whole packet) */
        BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, ETH_LLDP, 0, 1),   /* if EtherType != LLDP skip next instr. */
        BPF_STMT(BPF_RET+BPF_K, (u_int)-1),                /* return max int (the whole packet) */
        BPF_STMT(BPF_RET+BPF_K, 0),                        /* return zero */
    };
#endif
