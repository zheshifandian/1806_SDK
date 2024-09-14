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

#ifndef _TLVF_WFA_MAP_TLVASSOCIATEDWIFI6STASTATUSREPORT_H_
#define _TLVF_WFA_MAP_TLVASSOCIATEDWIFI6STASTATUSREPORT_H_

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


class tlvAssociatedWiFi6StaStatusReport : public BaseClass
{
    public:
        tlvAssociatedWiFi6StaStatusReport(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvAssociatedWiFi6StaStatusReport(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvAssociatedWiFi6StaStatusReport();

        typedef struct sTidQueueSize {
            //The Tid of the corresponding queue size field. 
            uint8_t tid;
            //Queue size of associated TID field.
            uint8_t queue_size;
            void struct_swap(){
            }
            void struct_init(){
            }
        } __attribute__((packed)) sTidQueueSize;
        
        const eTlvTypeMap& type();
        const uint16_t& length();
        sMacAddr& sta_mac();
        uint8_t& tid_queue_size_list_length();
        std::tuple<bool, sTidQueueSize&> tid_queue_size_list(size_t idx);
        bool alloc_tid_queue_size_list(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        sMacAddr* m_sta_mac = nullptr;
        uint8_t* m_tid_queue_size_list_length = nullptr;
        sTidQueueSize* m_tid_queue_size_list = nullptr;
        size_t m_tid_queue_size_list_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVASSOCIATEDWIFI6STASTATUSREPORT_H_
