/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_CMDU_CLIENT_FACTORY_FACTORY_H_
#define _BEEROCKS_CMDU_CLIENT_FACTORY_FACTORY_H_

#include <bcl/beerocks_cmdu_client_factory.h>

#include <bcl/beerocks_event_loop.h>

namespace beerocks {

/**
 * @brief Creates an instance of a CMDU client factory.
 *
 * A CMDU client factory creates CMDU client instances connected to a CMDU server running in
 * another process.
 *
 * @param uds_address Unix Domain Socket address where the CMDU server is listening for connection
 * requests and hence the CMDU client has to connect to.
 * @param event_loop Application event loop used by the process to wait for I/O events.
 */
std::unique_ptr<CmduClientFactory>
create_cmdu_client_factory(const std::string &uds_path,
                           std::shared_ptr<beerocks::EventLoop> event_loop);

} // namespace beerocks

#endif // _BEEROCKS_CMDU_CLIENT_FACTORY_FACTORY_H_
