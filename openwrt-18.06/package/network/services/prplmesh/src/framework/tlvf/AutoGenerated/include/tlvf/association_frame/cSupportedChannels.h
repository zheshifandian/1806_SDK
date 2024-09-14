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

#ifndef _TLVF_ASSOCIATION_FRAME_CSUPPORTEDCHANNELS_H_
#define _TLVF_ASSOCIATION_FRAME_CSUPPORTEDCHANNELS_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include <tuple>
#include "tlvf/association_frame/eElementID.h"

namespace assoc_frame {


class cSupportedChannels : public BaseClass
{
    public:
        cSupportedChannels(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cSupportedChannels(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cSupportedChannels();

        typedef struct sSupportedChannelsSet {
            //First channel number
            uint8_t first_ch_num;
            //Number of channels in the set
            uint8_t channels_number;
            void struct_swap(){
            }
            void struct_init(){
            }
        } __attribute__((packed)) sSupportedChannelsSet;
        
        eElementID& type();
        const uint8_t& length();
        size_t supported_channel_sets_length() { return m_supported_channel_sets_idx__ * sizeof(cSupportedChannels::sSupportedChannelsSet); }
        std::tuple<bool, sSupportedChannelsSet&> supported_channel_sets(size_t idx);
        bool alloc_supported_channel_sets(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        sSupportedChannelsSet* m_supported_channel_sets = nullptr;
        size_t m_supported_channel_sets_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

}; // close namespace: assoc_frame

#endif //_TLVF/ASSOCIATION_FRAME_CSUPPORTEDCHANNELS_H_
