///////////////////////////////////////
// AUTO GENERATED FILE - DO NOT EDIT //
///////////////////////////////////////

/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_TLVF_CLASSES_CHANNELLIST_H_
#define _BEEROCKS_TLVF_CLASSES_CHANNELLIST_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include <tuple>
#include <vector>
#include "../enums/eDfsState.h"
#include "../structs/sSupportedBandwidth.h"

namespace beerocks_message {

class cChannel;

class cChannelList : public BaseClass
{
    public:
        cChannelList(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cChannelList(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cChannelList();

        uint8_t& channels_list_length();
        std::tuple<bool, cChannel&> channels_list(size_t idx);
        std::shared_ptr<cChannel> create_channels_list();
        bool add_channels_list(std::shared_ptr<cChannel> ptr);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        uint8_t* m_channels_list_length = nullptr;
        cChannel* m_channels_list = nullptr;
        size_t m_channels_list_idx__ = 0;
        std::vector<std::shared_ptr<cChannel>> m_channels_list_vector;
        bool m_lock_allocation__ = false;
        int m_lock_order_counter__ = 0;
};

class cChannel : public BaseClass
{
    public:
        cChannel(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cChannel(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cChannel();

        //Beaconing channel (20MHz)
        uint8_t& beacon_channel();
        int8_t& tx_power_dbm();
        //Represent the state only on the 20 MHz beaconing channel
        eDfsState& dfs_state();
        uint8_t& supported_bandwidths_length();
        std::tuple<bool, sSupportedBandwidth&> supported_bandwidths(size_t idx);
        bool alloc_supported_bandwidths(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        uint8_t* m_beacon_channel = nullptr;
        int8_t* m_tx_power_dbm = nullptr;
        eDfsState* m_dfs_state = nullptr;
        uint8_t* m_supported_bandwidths_length = nullptr;
        sSupportedBandwidth* m_supported_bandwidths = nullptr;
        size_t m_supported_bandwidths_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

}; // close namespace: beerocks_message

#endif //_BEEROCKS/TLVF/CLASSES_CHANNELLIST_H_
