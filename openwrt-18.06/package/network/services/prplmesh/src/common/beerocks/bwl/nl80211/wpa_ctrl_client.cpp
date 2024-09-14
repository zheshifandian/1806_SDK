/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2022 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "wpa_ctrl_client.h"

#include <easylogging++.h>

extern "C" {
#include <wpa_ctrl.h>
}

namespace bwl {
namespace nl80211 {

wpa_ctrl_socket::wpa_ctrl_socket(const std::string &path) : m_path(path) {}

wpa_ctrl_socket::~wpa_ctrl_socket() { close(); }

const std::string &wpa_ctrl_socket::path() const { return m_path; }

bool wpa_ctrl_socket::open()
{
    if (m_ctx) {
        return true;
    }
    if (!(m_ctx = wpa_ctrl_open(m_path.c_str()))) {
        LOG(ERROR) << "wpa_ctrl_open() failed, ctrl_iface_path: " << m_path;
        return false;
    }
    LOG(DEBUG) << "WPA ctrl sock is opened with path " << m_path;
    return true;
}

void wpa_ctrl_socket::close()
{
    if (!m_ctx) {
        return;
    }
    wpa_ctrl_close(m_ctx);
    m_ctx = nullptr;
}

int wpa_ctrl_socket::fd() const
{
    if (!m_ctx) {
        LOG(DEBUG) << "WPA ctrl sock " << m_path << " not opened.";
        return -1;
    }
    return wpa_ctrl_get_fd(m_ctx);
}

wpa_ctrl_socket_cmd::wpa_ctrl_socket_cmd(const std::string &path) : wpa_ctrl_socket(path) {}

bool wpa_ctrl_socket_cmd::connect() { return open(); }

bool wpa_ctrl_socket_cmd::disconnect(bool force)
{
    close();
    return true;
}

bool wpa_ctrl_socket_cmd::request(const std::string &cmd, char *buffer, size_t buff_size)
{
    if (!m_ctx) {
        LOG(ERROR) << "Control socket not available!";
        return false;
    }

    int result;
    int try_cnt         = 0;
    auto buff_size_copy = buff_size;
    do {
        result = wpa_ctrl_request(m_ctx, cmd.c_str(), cmd.size(), buffer, &buff_size_copy, NULL);
    } while (result == -2 && ++try_cnt < WPA_CTRL_READ_RETRY_MAX);

    if (result < 0) {
        LOG(ERROR) << "can't send wpa_ctrl_request with cmd: " << cmd;
        return false;
    }

    if (buff_size_copy >= buff_size) {
        LOG(ERROR) << "wpa_ctrl_request returned reply of size " << buff_size_copy;
        return false;
    }

    // the wpa_ctrl does not put null terminator at the and of the string
    buffer[buff_size_copy] = 0;
    return true;
}

wpa_ctrl_socket_event::wpa_ctrl_socket_event(const std::string &path, std::vector<int> &event_fds)
    : wpa_ctrl_socket(path), m_attached(false), m_event_fds(event_fds)
{
}

wpa_ctrl_socket_event::~wpa_ctrl_socket_event() { close(); }

bool wpa_ctrl_socket_event::open()
{
    if (wpa_ctrl_socket::open()) {
        add_event_fd(fd());
        return true;
    }
    return false;
}

bool wpa_ctrl_socket_event::connect() { return (open() && attach()); }

bool wpa_ctrl_socket_event::disconnect(bool force)
{
    if (detach() || force) {
        m_attached = false;
        del_event_fd(fd());
        wpa_ctrl_socket::close();
        return true;
    }
    return false;
}

void wpa_ctrl_socket_event::close() { disconnect(true); }

bool wpa_ctrl_socket_event::attach()
{
    if (!m_ctx) {
        LOG(ERROR) << "Control socket not available!";
        return false;
    }
    if (m_attached) {
        return true;
    }

    // Attach to the control interface for events receiving
    int result;
    int try_cnt = 0;
    do {
        result = wpa_ctrl_attach(m_ctx);

        // return values: 0 on success, -1 on failure, -2 on timeout
    } while (result == -2 && ++try_cnt < WPA_CTRL_READ_RETRY_MAX);
    if (result != 0) {
        LOG(DEBUG) << "wpa_ctrl_attach() failed, ctrl_iface_path: " << path() << " res: " << result;
        return false;
    }
    m_attached = true;
    LOG(DEBUG) << "attached for event over " << path();
    return true;
}

bool wpa_ctrl_socket_event::detach()
{
    if (!m_ctx) {
        return true;
    }
    if (!m_attached) {
        return true;
    }

    // Detach to the control interface for events receiving
    int result;
    int try_cnt = 0;
    do {
        result = wpa_ctrl_detach(m_ctx);

        // return values: 0 on success, -1 on failure, -2 on timeout
    } while (result == -2 && ++try_cnt < WPA_CTRL_READ_RETRY_MAX);
    if (result != 0) {
        LOG(DEBUG) << "wpa_ctrl_detach() failed, ctrl_iface_path: " << path() << " res: " << result;
        return false;
    }
    m_attached = false;
    LOG(DEBUG) << "detached for event over " << path();
    return true;
}

bool wpa_ctrl_socket_event::pending() { return (m_ctx && m_attached && wpa_ctrl_pending(m_ctx)); }

bool wpa_ctrl_socket_event::receive(char *buffer, size_t buff_size)
{
    if (!m_ctx) {
        LOG(ERROR) << "Control socket not available!";
        return false;
    }
    auto buff_size_copy = buff_size;
    int result          = wpa_ctrl_recv(m_ctx, buffer, &buff_size_copy);
    if (result < 0) {
        LOG(ERROR) << "wpa_ctrl_recv() failed!";
        return false;
    }
    if (buff_size_copy >= buff_size) {
        LOG(ERROR) << "wpa_ctrl_recv returned reply of size " << buff_size_copy;
        return false;
    }

    // the wpa_ctrl does not put null terminator at the and of the string
    buffer[buff_size_copy] = 0;
    return true;
}

void wpa_ctrl_socket_event::add_event_fd(int fd)
{
    if (fd <= 0) {
        return;
    }
    auto itReplace = m_event_fds.end();
    for (auto it = std::begin(m_event_fds); it != std::end(m_event_fds); ++it) {
        if (*it == fd) {
            return;
        }
        if (*it == -1) {
            itReplace = it;
        }
    }

    // replace first invalid fd (-1) or append the new fd value
    if (itReplace != m_event_fds.end()) {
        *itReplace = fd;
    } else {
        m_event_fds.push_back(fd);
    }
}

void wpa_ctrl_socket_event::del_event_fd(int fd)
{
    if (fd <= 0) {
        return;
    }
    auto it = std::find(m_event_fds.begin(), m_event_fds.end(), fd);
    if (it != m_event_fds.end()) {
        m_event_fds.erase(it);
    }
}

bool wpa_ctrl_client::add_interface(const std::string &interface, const std::string &path,
                                    std::vector<int> &event_fds)
{
    auto wpa_ctrl_iface = sWpaCtrlIface(path, event_fds);
    auto ret            = m_wpa_ctrl.insert(std::make_pair(interface, wpa_ctrl_iface));
    if (ret.second == false) {
        LOG(DEBUG) << "wpa_ctrl info already exists for interface " << interface << " with path "
                   << path;

        // even if interface exists, return success if wpa_ctrl path is the same
        auto &saved_path = ret.first->second.cmd->path();
        if (saved_path.compare(path) != 0) {
            LOG(ERROR) << "WPA ctrl " << interface << " socket new path " << path
                       << " mismatches saved path " << saved_path;
            return false;
        }
    }
    LOG(DEBUG) << "WPA ctrl socket is added with path " << path << " for " << interface;
    return true;
}

bool wpa_ctrl_client::del_interface(const std::string &interface)
{
    auto ret = m_wpa_ctrl.erase(interface);
    if (ret == 0) {
        LOG(WARNING) << "no found wpa_ctrl info for interface " << interface;
    }
    return (ret > 0);
}

bool wpa_ctrl_client::has_interface(const std::string &interface) const
{
    return (m_wpa_ctrl.find(interface) != m_wpa_ctrl.end());
}

void wpa_ctrl_client::clear_interfaces() { m_wpa_ctrl.clear(); }

const std::shared_ptr<wpa_ctrl_socket_cmd>
wpa_ctrl_client::get_socket_cmd(const std::string &interface) const
{
    auto it = m_wpa_ctrl.find(interface);
    if (it != m_wpa_ctrl.end()) {
        return it->second.cmd;
    }
    return {};
}

const std::shared_ptr<wpa_ctrl_socket_event>
wpa_ctrl_client::get_socket_event(const std::string &interface) const
{
    auto it = m_wpa_ctrl.find(interface);
    if (it != m_wpa_ctrl.end()) {
        return it->second.event;
    }
    return {};
}

const std::shared_ptr<wpa_ctrl_socket_event> wpa_ctrl_client::get_socket_event(int fd) const
{
    if (fd < 0) {
        return {};
    }
    for (auto &it : m_wpa_ctrl) {
        /*
         * if file descriptor is equal to 0,
         * then check that event socket has pending data to be processed
         * otherwise fetch event socket matching the same file descriptor.
         */
        if (((fd == 0) && it.second.event->pending()) || (fd == it.second.event->fd())) {
            return it.second.event;
        }
    }
    return {};
}

const std::string wpa_ctrl_client::get_interface(int fd) const
{
    if (fd < 0) {
        return {};
    }
    for (auto &it : m_wpa_ctrl) {
        if ((fd == it.second.event->fd()) || (fd == it.second.cmd->fd())) {
            return it.first;
        }
    }
    return {};
}

} // namespace nl80211
} // namespace bwl
