/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_BUFFER_H_
#define BCL_NETWORK_BUFFER_H_

#include <stddef.h>
#include <stdint.h>

namespace beerocks {
namespace net {

/**
 * Array of bytes used to hold data received through a socket.
 * Code is programmed to interfaces so it does not care about which implementation is used.
 * Unit tests can use a mock and set different expectations per test (pretend that different data
 * has been received through the socket).
 */
class Buffer {
public:
    /**
     * @brief Class destructor
     */
    virtual ~Buffer() = default;

    /**
     * @brief Returns pointer to the raw data.
     *
     * @return address of raw data.
     */
    virtual const uint8_t *data() const = 0;

    /**
     * @brief Returns the length of the buffer (number of bytes).
     *
     * @return length of buffer
     */
    virtual const size_t &length() const = 0;

    /**
     * @brief Returns the size of the buffer.
     *
     * @return size of buffer
     */
    virtual size_t size() const = 0;

    /**
     * @brief Clears buffer contents.
     */
    virtual void clear() = 0;

    /**
     * @brief Appends given data to buffer contents.
     *
     * @param data Array of bytes to append.
     * @param length Number of bytes to append.
     * @return true on success and false otherwise (i.e.: data to append does not fit into buffer).
     */
    virtual bool append(const uint8_t *data, size_t length) = 0;

    /**
     * @brief Shifts buffer contents to the left.
     *
     * The operation of shifting a certain amount of bytes to the left is like "consuming" those
     * bytes.
     *
     * @param count The number of bytes to shift (consume).
     * @return true on success and false otherwise.
     */
    virtual bool shift(size_t count) = 0;

    /**
     * @brief  Returns pointer to the raw data.
     *
     * This is the non-const version of the method with the same name.
     *
     * @return address of raw data.
     */
    uint8_t *data() { return const_cast<uint8_t *>(const_cast<const Buffer *>(this)->data()); }

    /**
     * @brief Returns the length of the buffer (number of bytes).
     *
     * This is the non-const version of the method with the same name.
     *
     * @return length of buffer
     */
    size_t &length() { return const_cast<size_t &>(const_cast<const Buffer *>(this)->length()); }
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_BUFFER_IMPL_H_ */
