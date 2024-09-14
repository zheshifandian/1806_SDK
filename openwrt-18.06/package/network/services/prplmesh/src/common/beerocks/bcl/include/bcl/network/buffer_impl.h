/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_BUFFER_IMPL_H_
#define BCL_NETWORK_BUFFER_IMPL_H_

#include "buffer.h"

#include <algorithm>

namespace beerocks {
namespace net {

/**
 * One possible Buffer implementation where size is provided as a template parameter and raw data
 * is stored internally.
 */
template <size_t Size> class BufferImpl : public Buffer {
public:
    const uint8_t *data() const override { return m_data; }
    const size_t &length() const override { return m_length; }
    size_t size() const override { return sizeof(m_data); }

    void clear() override
    {
        std::fill_n(m_data, size(), 0);
        m_length = 0;
    }

    bool append(const uint8_t *data, size_t length) override
    {
        if (m_length + length > size()) {
            return false;
        }

        std::copy_n(data, length, &m_data[m_length]);
        m_length += length;

        return true;
    }

    bool shift(size_t count) override
    {
        if (count > m_length) {
            return false;
        }

        if (count > 0) {
            m_length -= count;
            std::copy_n(m_data + count, m_length, m_data);
            std::fill_n(m_data + m_length, size() - m_length, 0);
        }

        return true;
    }

private:
    uint8_t m_data[Size]{};
    size_t m_length = 0;
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_BUFFER_IMPL_H_ */
