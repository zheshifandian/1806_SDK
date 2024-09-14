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

#ifndef _TLVF_WFA_MAP_TLVPROFILE2METRICCOLLECTIONINTERVAL_H_
#define _TLVF_WFA_MAP_TLVPROFILE2METRICCOLLECTIONINTERVAL_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include "tlvf/wfa_map/eTlvTypeMap.h"

namespace wfa_map {


class tlvProfile2MetricCollectionInterval : public BaseClass
{
    public:
        tlvProfile2MetricCollectionInterval(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvProfile2MetricCollectionInterval(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvProfile2MetricCollectionInterval();

        const eTlvTypeMap& type();
        const uint16_t& length();
        uint32_t& collection_interval();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        uint32_t* m_collection_interval = nullptr;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVPROFILE2METRICCOLLECTIONINTERVAL_H_
