/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_eventloop_thread.h>
#include <easylogging++.h>

#include <bcl/beerocks_event_loop_impl.h>

using namespace beerocks;

EventLoopThread::EventLoopThread() : thread_base()
{
    // Create application event loop to wait for blocking I/O operations.
    m_event_loop = std::make_shared<beerocks::EventLoopImpl>();
    LOG_IF(!m_event_loop, FATAL) << "Unable to create event loop!";
}

EventLoopThread::~EventLoopThread() {}

bool EventLoopThread::work()
{
    if (m_event_loop->run() < 0) {
        LOG(ERROR) << "Event loop failure!";
        should_stop = true;
        return false;
    }
    return true;
}
