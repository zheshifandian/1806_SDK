/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_UCC_SERIALIZER_MOCK_H_
#define _BEEROCKS_UCC_SERIALIZER_MOCK_H_

#include <bcl/beerocks_ucc_serializer.h>

#include <gmock/gmock.h>

namespace beerocks {

class UccSerializerMock : public UccSerializer {
public:
    MOCK_METHOD(bool, serialize_reply, (const std::string &reply, beerocks::net::Buffer &buffer),
                (override));
};

} // namespace beerocks

#endif /* _BEEROCKS_UCC_SERIALIZER_MOCK_H_ */
