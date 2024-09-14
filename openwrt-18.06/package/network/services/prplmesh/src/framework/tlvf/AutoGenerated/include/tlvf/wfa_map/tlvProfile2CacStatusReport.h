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

#ifndef _TLVF_WFA_MAP_TLVPROFILE2CACSTATUSREPORT_H_
#define _TLVF_WFA_MAP_TLVPROFILE2CACSTATUSREPORT_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include "tlvf/wfa_map/eTlvTypeMap.h"
#include <tuple>

namespace wfa_map {


class tlvProfile2CacStatusReport : public BaseClass
{
    public:
        tlvProfile2CacStatusReport(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvProfile2CacStatusReport(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvProfile2CacStatusReport();

        typedef struct sAvailableChannels {
            uint8_t operating_class;
            uint8_t channel;
            uint16_t minutes_since_cac_completion;
            void struct_swap(){
                tlvf_swap(16, reinterpret_cast<uint8_t*>(&minutes_since_cac_completion));
            }
            void struct_init(){
            }
        } __attribute__((packed)) sAvailableChannels;
        
        typedef struct sDetectedPairs {
            uint8_t operating_class_detected;
            uint8_t channel_detected;
            uint16_t duration;
            void struct_swap(){
                tlvf_swap(16, reinterpret_cast<uint8_t*>(&duration));
            }
            void struct_init(){
            }
        } __attribute__((packed)) sDetectedPairs;
        
        typedef struct sActiveCacPairs {
            uint8_t operating_class_active_cac;
            uint8_t channel_active_cac;
            uint8_t countdown[3];
            void struct_swap(){
            }
            void struct_init(){
            }
        } __attribute__((packed)) sActiveCacPairs;
        
        const eTlvTypeMap& type();
        const uint16_t& length();
        uint8_t& number_of_available_channels();
        std::tuple<bool, sAvailableChannels&> available_channels(size_t idx);
        bool alloc_available_channels(size_t count = 1);
        uint8_t& number_of_detected_pairs();
        std::tuple<bool, sDetectedPairs&> detected_pairs(size_t idx);
        bool alloc_detected_pairs(size_t count = 1);
        uint8_t& number_of_active_cac_pairs();
        std::tuple<bool, sActiveCacPairs&> active_cac_pairs(size_t idx);
        bool alloc_active_cac_pairs(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        uint8_t* m_number_of_available_channels = nullptr;
        sAvailableChannels* m_available_channels = nullptr;
        size_t m_available_channels_idx__ = 0;
        int m_lock_order_counter__ = 0;
        uint8_t* m_number_of_detected_pairs = nullptr;
        sDetectedPairs* m_detected_pairs = nullptr;
        size_t m_detected_pairs_idx__ = 0;
        uint8_t* m_number_of_active_cac_pairs = nullptr;
        sActiveCacPairs* m_active_cac_pairs = nullptr;
        size_t m_active_cac_pairs_idx__ = 0;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVPROFILE2CACSTATUSREPORT_H_
