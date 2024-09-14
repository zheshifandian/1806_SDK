/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_BUFFER_MOCK_H_
#define BCL_NETWORK_BUFFER_MOCK_H_

#include "buffer.h"

#include <gmock/gmock.h>

namespace beerocks {
namespace net {

class BufferMock : public Buffer {
public:
    MOCK_METHOD(const uint8_t *, data, (), (const, override));
    MOCK_METHOD(const size_t &, length, (), (const, override));
    MOCK_METHOD(size_t, size, (), (const, override));
    MOCK_METHOD(void, clear, (), (override));
    MOCK_METHOD(bool, append, (const uint8_t *data, size_t length), (override));
    MOCK_METHOD(bool, shift, (size_t count), (override));
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_BUFFER_MOCK_H_ */
