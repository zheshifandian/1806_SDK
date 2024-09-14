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

#ifndef _TLVF_WFA_MAP_TLVAPEXTENDEDMETRICS_H_
#define _TLVF_WFA_MAP_TLVAPEXTENDEDMETRICS_H_

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


class tlvApExtendedMetrics : public BaseClass
{
    public:
        tlvApExtendedMetrics(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvApExtendedMetrics(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvApExtendedMetrics();

        const eTlvTypeMap& type();
        const uint16_t& length();
        sMacAddr& bssid();
        uint32_t& unicast_bytes_sent();
        uint32_t& unicast_bytes_received();
        uint32_t& multicast_bytes_sent();
        uint32_t& multicast_bytes_received();
        uint32_t& broadcast_bytes_sent();
        uint32_t& broadcast_bytes_received();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        sMacAddr* m_bssid = nullptr;
        uint32_t* m_unicast_bytes_sent = nullptr;
        uint32_t* m_unicast_bytes_received = nullptr;
        uint32_t* m_multicast_bytes_sent = nullptr;
        uint32_t* m_multicast_bytes_received = nullptr;
        uint32_t* m_broadcast_bytes_sent = nullptr;
        uint32_t* m_broadcast_bytes_received = nullptr;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVAPEXTENDEDMETRICS_H_
