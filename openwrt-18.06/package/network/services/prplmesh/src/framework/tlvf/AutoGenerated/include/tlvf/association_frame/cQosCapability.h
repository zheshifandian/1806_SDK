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

#ifndef _TLVF_ASSOCIATION_FRAME_CQOSCAPABILITY_H_
#define _TLVF_ASSOCIATION_FRAME_CQOSCAPABILITY_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include <asm/byteorder.h>
#include "tlvf/association_frame/eElementID.h"

namespace assoc_frame {


class cQosCapability : public BaseClass
{
    public:
        cQosCapability(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cQosCapability(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cQosCapability();

        typedef struct sQosInfo {
            #if defined(__LITTLE_ENDIAN_BITFIELD)
            uint8_t edca_param_set_update_count : 4;
            uint8_t q_ask : 1;
            uint8_t queue_request : 1;
            uint8_t txop_request : 1;
            uint8_t reserved : 1;
            #elif defined(__BIG_ENDIAN_BITFIELD)
            uint8_t reserved : 1;
            uint8_t txop_request : 1;
            uint8_t queue_request : 1;
            uint8_t q_ask : 1;
            uint8_t edca_param_set_update_count : 4;
            #else
            #error "Bitfield macros are not defined"
            #endif
            void struct_swap(){
            }
            void struct_init(){
            }
        } __attribute__((packed)) sQosInfo;
        
        eElementID& type();
        const uint8_t& length();
        sQosInfo& qos_info();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        sQosInfo* m_qos_info = nullptr;
};

}; // close namespace: assoc_frame

#endif //_TLVF/ASSOCIATION_FRAME_CQOSCAPABILITY_H_
