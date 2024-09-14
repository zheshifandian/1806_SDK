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

#ifndef _TLVF_WFA_MAP_TLVPROFILE2RADIOMETRICS_H_
#define _TLVF_WFA_MAP_TLVPROFILE2RADIOMETRICS_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include "tlvf/wfa_map/eTlvTypeMap.h"
#include "tlvf/common/sMacAddr.h"

namespace wfa_map {


class tlvProfile2RadioMetrics : public BaseClass
{
    public:
        tlvProfile2RadioMetrics(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvProfile2RadioMetrics(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvProfile2RadioMetrics();

        const eTlvTypeMap& type();
        const uint16_t& length();
        sMacAddr& radio_uid();
        uint8_t& noise();
        uint8_t& transmit();
        uint8_t& receive_self();
        uint8_t& receive_other();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        sMacAddr* m_radio_uid = nullptr;
        uint8_t* m_noise = nullptr;
        uint8_t* m_transmit = nullptr;
        uint8_t* m_receive_self = nullptr;
        uint8_t* m_receive_other = nullptr;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVPROFILE2RADIOMETRICS_H_
