/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_CMDU_SERIALIZER_MOCK_H_
#define BCL_NETWORK_CMDU_SERIALIZER_MOCK_H_

#include <bcl/network/cmdu_serializer.h>

#include <gmock/gmock.h>

namespace beerocks {
namespace net {

class CmduSerializerMock : public CmduSerializer {
public:
    MOCK_METHOD(bool, serialize_cmdu,
                (uint32_t iface_index, const sMacAddr &dst_mac, const sMacAddr &src_mac,
                 ieee1905_1::CmduMessage &cmdu, Buffer &buffer),
                (override));
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_CMDU_SERIALIZER_MOCK_H_ */
