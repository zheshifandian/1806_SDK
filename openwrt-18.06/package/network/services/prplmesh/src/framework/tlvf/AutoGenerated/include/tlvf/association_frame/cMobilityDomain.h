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

#ifndef _TLVF_ASSOCIATION_FRAME_CMOBILITYDOMAIN_H_
#define _TLVF_ASSOCIATION_FRAME_CMOBILITYDOMAIN_H_

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


class cMobilityDomain : public BaseClass
{
    public:
        cMobilityDomain(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cMobilityDomain(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cMobilityDomain();

        typedef struct sFtCapabilityPolicy {
            #if defined(__LITTLE_ENDIAN_BITFIELD)
            uint8_t fast_bss_trans_over_ds : 1;
            uint8_t resource_req_protocol_cap : 1;
            uint8_t reserved : 1;
            #elif defined(__BIG_ENDIAN_BITFIELD)
            uint8_t reserved : 1;
            uint8_t resource_req_protocol_cap : 1;
            uint8_t fast_bss_trans_over_ds : 1;
            #else
            #error "Bitfield macros are not defined"
            #endif
            void struct_swap(){
            }
            void struct_init(){
            }
        } __attribute__((packed)) sFtCapabilityPolicy;
        
        eElementID& type();
        const uint8_t& length();
        uint16_t& mdid();
        sFtCapabilityPolicy& ft_cap_policy();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint16_t* m_mdid = nullptr;
        sFtCapabilityPolicy* m_ft_cap_policy = nullptr;
};

}; // close namespace: assoc_frame

#endif //_TLVF/ASSOCIATION_FRAME_CMOBILITYDOMAIN_H_
