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

#ifndef _TLVF_ASSOCIATION_FRAME_CSTAHTCAPABILITY_H_
#define _TLVF_ASSOCIATION_FRAME_CSTAHTCAPABILITY_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include <tuple>
#include <asm/byteorder.h>
#include "tlvf/association_frame/eElementID.h"
#include "tlvf/AssociationRequestFrame/assoc_frame_bitfields.h"

namespace assoc_frame {


class cStaHtCapability : public BaseClass
{
    public:
        cStaHtCapability(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cStaHtCapability(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cStaHtCapability();

        typedef struct sA_MpduParam {
            #if defined(__LITTLE_ENDIAN_BITFIELD)
            uint8_t max_ampdu_len_expo : 2;
            uint8_t min_mpdu_start_spacing : 3;
            uint8_t reserved : 3;
            #elif defined(__BIG_ENDIAN_BITFIELD)
            uint8_t reserved : 3;
            uint8_t min_mpdu_start_spacing : 3;
            uint8_t max_ampdu_len_expo : 2;
            #else
            #error "Bitfield macros are not defined"
            #endif
            void struct_swap(){
            }
            void struct_init(){
            }
        } __attribute__((packed)) sA_MpduParam;
        
        eElementID& type();
        const uint8_t& length();
        assoc_frame::sStaHtCapabilityInfo& ht_cap_info();
        sA_MpduParam& a_mpdu_param();
        uint8_t* ht_mcs_set(size_t idx = 0);
        bool set_ht_mcs_set(const void* buffer, size_t size);
        uint16_t& ht_extended_caps();
        uint32_t& tx_beamforming_caps();
        uint8_t& asel_caps();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        assoc_frame::sStaHtCapabilityInfo* m_ht_cap_info = nullptr;
        sA_MpduParam* m_a_mpdu_param = nullptr;
        uint8_t* m_ht_mcs_set = nullptr;
        size_t m_ht_mcs_set_idx__ = 0;
        int m_lock_order_counter__ = 0;
        uint16_t* m_ht_extended_caps = nullptr;
        uint32_t* m_tx_beamforming_caps = nullptr;
        uint8_t* m_asel_caps = nullptr;
};

}; // close namespace: assoc_frame

#endif //_TLVF/ASSOCIATION_FRAME_CSTAHTCAPABILITY_H_
