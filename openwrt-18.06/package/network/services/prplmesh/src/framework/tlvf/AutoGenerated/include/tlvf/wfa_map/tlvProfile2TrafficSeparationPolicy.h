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

#ifndef _TLVF_WFA_MAP_TLVPROFILE2TRAFFICSEPARATIONPOLICY_H_
#define _TLVF_WFA_MAP_TLVPROFILE2TRAFFICSEPARATIONPOLICY_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include "tlvf/wfa_map/eTlvTypeMap.h"
#include <tuple>
#include <vector>

namespace wfa_map {

class cSsidVlanId;

class tlvProfile2TrafficSeparationPolicy : public BaseClass
{
    public:
        tlvProfile2TrafficSeparationPolicy(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvProfile2TrafficSeparationPolicy(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvProfile2TrafficSeparationPolicy();

        const eTlvTypeMap& type();
        const uint16_t& length();
        uint8_t& ssids_vlan_id_list_length();
        std::tuple<bool, cSsidVlanId&> ssids_vlan_id_list(size_t idx);
        std::shared_ptr<cSsidVlanId> create_ssids_vlan_id_list();
        bool add_ssids_vlan_id_list(std::shared_ptr<cSsidVlanId> ptr);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        uint8_t* m_ssids_vlan_id_list_length = nullptr;
        cSsidVlanId* m_ssids_vlan_id_list = nullptr;
        size_t m_ssids_vlan_id_list_idx__ = 0;
        std::vector<std::shared_ptr<cSsidVlanId>> m_ssids_vlan_id_list_vector;
        bool m_lock_allocation__ = false;
        int m_lock_order_counter__ = 0;
};

class cSsidVlanId : public BaseClass
{
    public:
        cSsidVlanId(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cSsidVlanId(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cSsidVlanId();

        uint8_t& ssid_name_length();
        std::string ssid_name_str();
        char* ssid_name(size_t length = 0);
        bool set_ssid_name(const std::string& str);
        bool set_ssid_name(const char buffer[], size_t size);
        bool alloc_ssid_name(size_t count = 1);
        //0x000x - 0x0002: Reserved
        //0x0003 - 0x0FFE: VLAN ID
        //0x0FFF - 0xFFFF: Reserved
        uint16_t& vlan_id();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        uint8_t* m_ssid_name_length = nullptr;
        char* m_ssid_name = nullptr;
        size_t m_ssid_name_idx__ = 0;
        int m_lock_order_counter__ = 0;
        uint16_t* m_vlan_id = nullptr;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVPROFILE2TRAFFICSEPARATIONPOLICY_H_
