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

#ifndef _TLVF_WFA_MAP_TLVASSOCIATEDSTAEXTENDEDLINKMETRICS_H_
#define _TLVF_WFA_MAP_TLVASSOCIATEDSTAEXTENDEDLINKMETRICS_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include "tlvf/wfa_map/eTlvTypeMap.h"
#include "tlvf/common/sMacAddr.h"
#include <tuple>

namespace wfa_map {


class tlvAssociatedStaExtendedLinkMetrics : public BaseClass
{
    public:
        tlvAssociatedStaExtendedLinkMetrics(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvAssociatedStaExtendedLinkMetrics(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvAssociatedStaExtendedLinkMetrics();

        typedef struct sMetrics {
            sMacAddr bssid;
            uint32_t last_data_down_link_rate;
            uint32_t last_data_up_link_rate;
            uint32_t utilization_receive;
            uint32_t utilization_transmit;
            void struct_swap(){
                bssid.struct_swap();
                tlvf_swap(32, reinterpret_cast<uint8_t*>(&last_data_down_link_rate));
                tlvf_swap(32, reinterpret_cast<uint8_t*>(&last_data_up_link_rate));
                tlvf_swap(32, reinterpret_cast<uint8_t*>(&utilization_receive));
                tlvf_swap(32, reinterpret_cast<uint8_t*>(&utilization_transmit));
            }
            void struct_init(){
                bssid.struct_init();
            }
        } __attribute__((packed)) sMetrics;
        
        const eTlvTypeMap& type();
        const uint16_t& length();
        sMacAddr& associated_sta();
        uint8_t& metrics_list_length();
        std::tuple<bool, sMetrics&> metrics_list(size_t idx);
        bool alloc_metrics_list(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        sMacAddr* m_associated_sta = nullptr;
        uint8_t* m_metrics_list_length = nullptr;
        sMetrics* m_metrics_list = nullptr;
        size_t m_metrics_list_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVASSOCIATEDSTAEXTENDEDLINKMETRICS_H_
