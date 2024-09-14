/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include <bcl/beerocks_mac_map.h>
#include <string>
#include <tlvf/common/sMacAddr.h>

// Forward declaration of son::node
namespace son {
class node;
}

namespace prplmesh {
namespace controller {
namespace db {

/**
 * Node that represents a Device's Interface in database.
 */
class Interface {
public:
    Interface(const sMacAddr &mac, son::node &node) : m_mac(mac), m_node(node) {}

    // Neighbor that connects to the interface
    struct sNeighbor {

        const sMacAddr mac;  //!< Neighbor AL-MAC
        bool ieee1905_flag;  //!< IEEE 1905 flag
        std::string dm_path; //!< Data model path

        sNeighbor(const sMacAddr &mac_, bool ieee1905_flag_)
            : mac(mac_), ieee1905_flag(ieee1905_flag_)
        {
        }
    };

    const sMacAddr m_mac;  //!< Interface MAC address
    son::node &m_node;     //!< Node on which the Interface exists.
    std::string m_dm_path; //!< Data model path
    beerocks::mac_map<sNeighbor> m_neighbors;
};

} // namespace db
} // namespace controller
} // namespace prplmesh

#endif // INTERFACE_H
