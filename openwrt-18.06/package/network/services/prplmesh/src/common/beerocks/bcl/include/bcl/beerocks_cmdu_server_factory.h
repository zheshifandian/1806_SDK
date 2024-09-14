/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_CMDU_SERVER_FACTORY_H_
#define _BEEROCKS_CMDU_SERVER_FACTORY_H_

#include <bcl/beerocks_cmdu_server.h>
#include <bcl/beerocks_event_loop.h>
#include <bcl/network/sockets_impl.h>

namespace beerocks {

class CmduServerFactory {
public:
    /**
     * @brief Creates an instance of a CMDU server.
     * 
     * @param uds_address Unix Domain Socket address where the socket server will listen for
     * incoming connection requests from clients.
     * @param event_loop Application event loop used by the process to wait for I/O events.
     */
    static std::unique_ptr<CmduServer>
    create_instance(std::shared_ptr<beerocks::net::UdsAddress> uds_address,
                    std::shared_ptr<EventLoop> event_loop);
};

} // namespace beerocks

#endif // _BEEROCKS_CMDU_SERVER_FACTORY_H_
