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

#ifndef _TLVF_ASSOCIATION_FRAME_CSTAVHTCAPABILITY_H_
#define _TLVF_ASSOCIATION_FRAME_CSTAVHTCAPABILITY_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include "tlvf/association_frame/eElementID.h"
#include "tlvf/AssociationRequestFrame/assoc_frame_bitfields.h"

namespace assoc_frame {


class cStaVhtCapability : public BaseClass
{
    public:
        cStaVhtCapability(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cStaVhtCapability(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cStaVhtCapability();

        eElementID& type();
        const uint8_t& length();
        assoc_frame::sStaVhtCapInfo& vht_cap_info();
        assoc_frame::sSupportedVhtMcsSet& supported_vht_mcs();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        assoc_frame::sStaVhtCapInfo* m_vht_cap_info = nullptr;
        assoc_frame::sSupportedVhtMcsSet* m_supported_vht_mcs = nullptr;
};

}; // close namespace: assoc_frame

#endif //_TLVF/ASSOCIATION_FRAME_CSTAVHTCAPABILITY_H_
