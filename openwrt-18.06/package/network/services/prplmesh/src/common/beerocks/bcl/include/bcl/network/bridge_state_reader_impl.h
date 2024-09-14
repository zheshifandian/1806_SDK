/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_BRIDGE_STATE_READER_IMPL_H_
#define BCL_NETWORK_BRIDGE_STATE_READER_IMPL_H_

#include "bridge_state_reader.h"

namespace beerocks {
namespace net {

class BridgeStateReaderImpl : public BridgeStateReader {
public:
    /**
     * @brief Reads current state of a network bridge (i.e.: the list of network interfaces in the
     * bridge).
     *
     * @see BridgeStateReader::read_state
     *
     * This implementation reads the contents of "/sys/class/net/" + bridge + "/brif" to obtain the
     * list of interfaces in the bridge.
     */
    bool read_state(const std::string &bridge_name, std::set<std::string> &iface_names) override;
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_BRIDGE_STATE_READER_IMPL_H_ */
