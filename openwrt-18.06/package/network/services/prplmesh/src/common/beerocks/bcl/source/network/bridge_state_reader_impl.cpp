/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/network/bridge_state_reader_impl.h>

#include <dirent.h>

namespace beerocks {
namespace net {

bool BridgeStateReaderImpl::read_state(const std::string &bridge_name,
                                       std::set<std::string> &iface_names)
{
    const std::string path = "/sys/class/net/" + bridge_name + "/brif";

    DIR *dir = opendir(path.c_str());
    if (!dir) {
        return false;
    }

    errno = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string iface_name = entry->d_name;
        if (iface_name == "." || iface_name == "..") {
            continue;
        }
        iface_names.emplace(iface_name);
    }
    closedir(dir);

    return (errno == 0);
}

} // namespace net
} // namespace beerocks
