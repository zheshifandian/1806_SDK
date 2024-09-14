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

#ifndef _TLVF_ASSOCIATION_FRAME_CRMENABLEDCAPS_H_
#define _TLVF_ASSOCIATION_FRAME_CRMENABLEDCAPS_H_

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


class cRmEnabledCaps : public BaseClass
{
    public:
        cRmEnabledCaps(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cRmEnabledCaps(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cRmEnabledCaps();

        eElementID& type();
        const uint8_t& length();
        assoc_frame::sRmEnabledCaps1& data1();
        assoc_frame::sRmEnabledCaps2& data2();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        assoc_frame::sRmEnabledCaps1* m_data1 = nullptr;
        assoc_frame::sRmEnabledCaps2* m_data2 = nullptr;
};

}; // close namespace: assoc_frame

#endif //_TLVF/ASSOCIATION_FRAME_CRMENABLEDCAPS_H_
