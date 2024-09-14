/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_BRIDGE_STATE_MANAGER_H_
#define BCL_NETWORK_BRIDGE_STATE_MANAGER_H_

#include "bridge_state_monitor.h"
#include "bridge_state_reader.h"

namespace beerocks {
namespace net {

/**
 * The BridgeStateManager is a facade interface for both the BridgeStateMonitor and
 * BridgeStateReader interfaces together.
 */
class BridgeStateManager : public BridgeStateMonitor, public BridgeStateReader {
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_BRIDGE_STATE_MANAGER_H_ */
