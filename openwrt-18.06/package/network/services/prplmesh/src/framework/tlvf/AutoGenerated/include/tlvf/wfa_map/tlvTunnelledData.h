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

#ifndef _TLVF_WFA_MAP_TLVTUNNELLEDDATA_H_
#define _TLVF_WFA_MAP_TLVTUNNELLEDDATA_H_

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


class tlvTunnelledData : public BaseClass
{
    public:
        tlvTunnelledData(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvTunnelledData(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvTunnelledData();

        const eTlvTypeMap& type();
        const uint16_t& length();
        //802.11 request frame body
        size_t data_length() { return m_data_idx__ * sizeof(uint8_t); }
        uint8_t* data(size_t idx = 0);
        bool set_data(const void* buffer, size_t size);
        bool alloc_data(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        uint8_t* m_data = nullptr;
        size_t m_data_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVTUNNELLEDDATA_H_
