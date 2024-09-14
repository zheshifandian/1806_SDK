/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_EVENTLOOP_THREAD_H_
#define _BEEROCKS_EVENTLOOP_THREAD_H_

#include "beerocks_thread_base.h"
#include <bcl/beerocks_event_loop.h>

namespace beerocks {

class EventLoopThread : public thread_base {
public:
    EventLoopThread();
    virtual ~EventLoopThread();

    bool work() final;

protected:
    /**
     * Application event loop used by the process to wait for I/O events.
     */
    std::shared_ptr<beerocks::EventLoop> m_event_loop;

private:
};

} // namespace beerocks

#endif //_BEEROCKS_EVENTLOOP_THREAD_H_
