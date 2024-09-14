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

#ifndef _TLVF_WFA_MAP_TLVDEVICEINVENTORY_H_
#define _TLVF_WFA_MAP_TLVDEVICEINVENTORY_H_

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
#include "tlvf/common/sMacAddr.h"

namespace wfa_map {

class cRadioVendorInfo;

class tlvDeviceInventory : public BaseClass
{
    public:
        tlvDeviceInventory(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvDeviceInventory(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvDeviceInventory();

        const eTlvTypeMap& type();
        const uint16_t& length();
        //Shall be less than or equal to 64 octets
        uint8_t& serial_number_length();
        std::string serial_number_str();
        char* serial_number(size_t length = 0);
        bool set_serial_number(const std::string& str);
        bool set_serial_number(const char buffer[], size_t size);
        bool alloc_serial_number(size_t count = 1);
        //Shall be less than or equal to 64 octets
        uint8_t& software_version_length();
        std::string software_version_str();
        char* software_version(size_t length = 0);
        bool set_software_version(const std::string& str);
        bool set_software_version(const char buffer[], size_t size);
        bool alloc_software_version(size_t count = 1);
        //Shall be less than or equal to 64 octets
        uint8_t& execution_environment_length();
        std::string execution_environment_str();
        char* execution_environment(size_t length = 0);
        bool set_execution_environment(const std::string& str);
        bool set_execution_environment(const char buffer[], size_t size);
        bool alloc_execution_environment(size_t count = 1);
        //Shall be at least 1 element
        uint8_t& number_of_radios();
        std::tuple<bool, cRadioVendorInfo&> radios_vendor_info(size_t idx);
        std::shared_ptr<cRadioVendorInfo> create_radios_vendor_info();
        bool add_radios_vendor_info(std::shared_ptr<cRadioVendorInfo> ptr);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        uint8_t* m_serial_number_length = nullptr;
        char* m_serial_number = nullptr;
        size_t m_serial_number_idx__ = 0;
        int m_lock_order_counter__ = 0;
        uint8_t* m_software_version_length = nullptr;
        char* m_software_version = nullptr;
        size_t m_software_version_idx__ = 0;
        uint8_t* m_execution_environment_length = nullptr;
        char* m_execution_environment = nullptr;
        size_t m_execution_environment_idx__ = 0;
        uint8_t* m_number_of_radios = nullptr;
        cRadioVendorInfo* m_radios_vendor_info = nullptr;
        size_t m_radios_vendor_info_idx__ = 0;
        std::vector<std::shared_ptr<cRadioVendorInfo>> m_radios_vendor_info_vector;
        bool m_lock_allocation__ = false;
};

class cRadioVendorInfo : public BaseClass
{
    public:
        cRadioVendorInfo(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cRadioVendorInfo(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cRadioVendorInfo();

        sMacAddr& ruid();
        //Shall be less than 65 octets
        uint8_t& chipset_vendor_length();
        //Shall be less than or equal to 64 octets
        std::string chipset_vendor_str();
        char* chipset_vendor(size_t length = 0);
        bool set_chipset_vendor(const std::string& str);
        bool set_chipset_vendor(const char buffer[], size_t size);
        bool alloc_chipset_vendor(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        sMacAddr* m_ruid = nullptr;
        uint8_t* m_chipset_vendor_length = nullptr;
        char* m_chipset_vendor = nullptr;
        size_t m_chipset_vendor_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVDEVICEINVENTORY_H_
