/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_TIMER_MOCK_H_
#define BCL_NETWORK_TIMER_MOCK_H_

#include <bcl/network/timer.h>

#include <gmock/gmock.h>

namespace beerocks {
namespace net {

template <class TimeUnits = std::chrono::milliseconds> class TimerMock : public Timer<TimeUnits> {
public:
    MOCK_METHOD(int, fd, (), (override));
    MOCK_METHOD(bool, schedule, (TimeUnits delay, TimeUnits period), (override));
    MOCK_METHOD(bool, cancel, (), (override));
    MOCK_METHOD(bool, read, (uint64_t & number_of_expirations), (override));
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_TIMER_MOCK_H_ */
