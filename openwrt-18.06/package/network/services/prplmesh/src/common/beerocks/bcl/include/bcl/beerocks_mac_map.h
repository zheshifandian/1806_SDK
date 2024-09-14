/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_MAC_MAP_H_
#define _BEEROCKS_MAC_MAP_H_

#include <algorithm>
#include <memory>
#include <tlvf/common/sMacAddr.h>
#include <tlvf/tlvftypes.h>
#include <unordered_map>
#include <vector>

namespace beerocks {

/**
 * @brief Map of objects keyed by MAC address.
 *
 * In many places, we need maps of objects keyed by MAC address. This class makes abstraction of
 * such maps and provides some useful shorthands.
 *
 * The objects are considered to be part of the map and kept alive by it. Therefore, they are
 * handled through @a shared_ptr. Since they are @a shared_ptr, the same @a mac_map class can also
 * be used to store objects created elsewhere.
 */
template <class T> class mac_map : public std::unordered_map<sMacAddr, std::shared_ptr<T>> {
public:
    mac_map()                = default;
    mac_map(const mac_map &) = default;
    mac_map(mac_map &&)      = default;

    // Implementation note
    // -------------------
    // Due to the difference in template instantiation between mac_map and std::unordered_map, the
    // compiler can't resolve the members of std::unordered_map. Any member, e.g. emplace must be
    // called explicitly as `std::unordered_map<sMacAddr, std::shared_ptr<T>>::emplace(...)`. Or,
    // a simpler solution, with an explicit this: `this->emplace(...)`.

    /**
     * @brief Create a @a T entry if it doesn't exist yet.
     *
     * If a @a T with the same MAC exists already, the old one is returned. In this case, the
     * additional arguments, if any, are ignored. (To be completely correct: a temporary object
     * is constructed, then destroyed right away.)
     *
     * This function can only be used if T has a constructor that takes the mac address as its first
     * argument. Additional arguments can be specified through the @a Args template argument.
     *
     * @param mac The MAC address of the new @a T.
     * @param args Additional constructor arguments of @a T.
     * @return The new or existing @a T. Never null.
     */
    template <class... Args> std::shared_ptr<T> add(const sMacAddr &mac, Args &&... args)
    {
        keep_new(mac);
        return this->emplace(mac, std::make_shared<T>(mac, std::forward<Args>(args)...))
            .first->second;
    }

    /**
     * @brief Add a @a T entry if it doesn't exist yet.
     *
     * If a @a T with the same MAC exists already, the old one is returned. In this case, @a new_t
     * is unaffected.
     *
     * This function can only be used if T has an ::sMacAddr member called @a mac.
     *
     * @param new_t The @a T entry to add. Must not be null.
     * @return The existing @a T if any, otherwise @a new_t. Never null.
     */
    std::shared_ptr<T> add(std::shared_ptr<T> new_t)
    {
        if (!new_t) { // It's not supposed to be null, but ignore it if it is null.
            return new_t;
        }
        keep_new(new_t->mac);
        return this->emplace(new_t->mac, new_t).first->second;
    }

    /**
     * @brief Get the @a T with the given MAC address.
     * @param mac MAC address of the @a T to look up.
     * @return The @a T with the given MAC address, or null if not found.
     */
    std::shared_ptr<T> get(const sMacAddr &mac) const
    {
        auto it = this->find(mac);
        if (it == this->end()) {
            return {};
        } else {
            return it->second;
        }
    }

    /**
     * @brief Prepare the mac_map for a keep_new process.
     *
     * In many situations, a mac_map is updated with a list of new or already-existing entries, and
     * the old entries that are not on that list should be removed. The keep_new functionality
     * facilitates this process.
     *
     * It looks like this in pseudo code:
     *
     * @code{.cpp}
     * keep_new_prepare();
     * for (...) {
     *     // prepare to add/update new entries
     *     ...
     *     keep_new(mac);
     * }
     * auto old = keep_new_remove_old();
     * // Handle removal of old entries
     * ...
     * @endcode
     *
     * keep_new_prepare() prepares the mac_map for updating like this. For every entry that needs
     * to be kept, keep_new() should be called. keep_new() is implied by the add() functions.
     * Finally, keep_new_remove_old() removes all the entries that were not marked with keep_new().
     *
     * When the vector returned by keep_new_remove_old() goes out of scope, the entries are actually
     * removed.
     */
    void keep_new_prepare()
    {
        old_macs.clear();
        std::transform(
            this->begin(), this->end(), std::back_inserter(old_macs),
            [](const std::pair<sMacAddr, std::shared_ptr<T>> &elt) { return elt.first; });
    }

    /**
     * @brief Mark a MAC address as to-be-kept by keep_new_remove_old()
     * @see keep_new_prepare()
     * @param mac The MAC address to keep.
     */
    void keep_new(const sMacAddr &mac)
    {
        old_macs.erase(std::remove(old_macs.begin(), old_macs.end(), mac), old_macs.end());
    }

    /**
     * @brief Remove all entries that have not been marked by keep_new().
     * @see keep_new_prepare()
     * @param C A collection of @c shared_ptr<T>
     */
    template <class C> void keep_new_remove_old(C &old)
    {
        for (auto mac : old_macs) {
            old.emplace_back(this->at(mac));
            this->erase(mac);
        }
        old_macs.clear();
    }

    /**
     * @brief Remove all entries that have not been marked by keep_new().
     * @see keep_new_prepare()
     * @return The list of removed entries.
     */
    std::vector<std::shared_ptr<T>> keep_new_remove_old()
    {
        std::vector<std::shared_ptr<T>> ret;
        keep_new_remove_old(ret);
        return ret;
    }

private:
    std::vector<sMacAddr> old_macs;
};

} // namespace beerocks

#endif // _BEEROCKS_MAC_MAP_H_
