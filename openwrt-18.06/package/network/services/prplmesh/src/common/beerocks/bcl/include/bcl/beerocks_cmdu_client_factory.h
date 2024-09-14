/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_CMDU_CLIENT_FACTORY_H_
#define _BEEROCKS_CMDU_CLIENT_FACTORY_H_

#include <bcl/beerocks_cmdu_client.h>

#include <memory>

namespace beerocks {

/**
 * @brief CMDU client factory interface.
 *
 * Classes that need to dynamically create @see CmduClientFactory objects must use a factory to do so,
 * instead of creating such objects directly (to avoid dependencies with real implementations).
 * The factory will be provided as a dependency so it can be mocked while unit testing. A factory
 * mock will return mock objects whenever an expectation of a call to `create_instance()` is
 * satisfied.
 */
class CmduClientFactory {
public:
    /**
     * Default destructor.
     */
    virtual ~CmduClientFactory() = default;

    /**
     * @brief Creates an instance of a CMDU client.
     *
     * The CMDU client created is already connected to the CMDU server.
     *
     * @return CMDU client instance.
     */
    virtual std::unique_ptr<CmduClient> create_instance() = 0;
};

} // namespace beerocks

#endif // _BEEROCKS_CMDU_CLIENT_FACTORY_H_
