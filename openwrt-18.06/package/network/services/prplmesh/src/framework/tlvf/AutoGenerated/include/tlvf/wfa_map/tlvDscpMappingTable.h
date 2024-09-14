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

#ifndef _TLVF_WFA_MAP_TLVDSCPMAPPINGTABLE_H_
#define _TLVF_WFA_MAP_TLVDSCPMAPPINGTABLE_H_

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


class tlvDscpMappingTable : public BaseClass
{
    public:
        tlvDscpMappingTable(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvDscpMappingTable(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvDscpMappingTable();

        const eTlvTypeMap& type();
        const uint16_t& length();
        //List of 64 PCP values (one octet per value) corresponding to the DSCP markings 0x00 to 0x3F,
        //ordered by increasing DSCP Value.
        //This table is used to select a PCP value if a Service Prioritization Rule specifies Rule
        //Output: 0x08.
        uint8_t* dscp_pcp_mapping(size_t idx = 0);
        bool set_dscp_pcp_mapping(const void* buffer, size_t size);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        uint8_t* m_dscp_pcp_mapping = nullptr;
        size_t m_dscp_pcp_mapping_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVDSCPMAPPINGTABLE_H_
