/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_UCC_SERVER_FACTORY_H_
#define _BEEROCKS_UCC_SERVER_FACTORY_H_

#include <bcl/beerocks_event_loop.h>
#include <bcl/beerocks_ucc_server.h>

#include <memory>

namespace beerocks {

class UccServerFactory {
public:
    /**
     * @brief Creates an instance of a UCC server.
     *
     * @param port TCP port where the socket server will listen for incoming connection requests
     * from clients.
     * @param event_loop Application event loop used by the process to wait for I/O events.
     */
    static std::unique_ptr<UccServer> create_instance(uint16_t port,
                                                      std::shared_ptr<EventLoop> event_loop);
};

} // namespace beerocks

#endif // _BEEROCKS_UCC_SERVER_FACTORY_H_
