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

#ifndef _TLVF_WFA_MAP_TLVPROFILE2CACCAPABILITIES_H_
#define _TLVF_WFA_MAP_TLVPROFILE2CACCAPABILITIES_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include "tlvf/wfa_map/eTlvTypeMap.h"
#include <tuple>
#include <vector>
#include "tlvf/common/sMacAddr.h"

namespace wfa_map {

class cCacCapabilitiesRadio;
class cCacTypes;
class cCacCapabilitiesOperatingClasses;
enum eCacMethod: uint8_t {
    CONTINUOUS_CAC = 0x0,
    CONTINUOUS_CAC_WITH_DEDICATED_RADIO = 0x1,
    MIMO_DIMENSION_REDUCED = 0x2,
    TIME_SLICED = 0x3,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eCacMethod_str(eCacMethod enum_value) {
    switch (enum_value) {
    case CONTINUOUS_CAC:                      return "CONTINUOUS_CAC";
    case CONTINUOUS_CAC_WITH_DEDICATED_RADIO: return "CONTINUOUS_CAC_WITH_DEDICATED_RADIO";
    case MIMO_DIMENSION_REDUCED:              return "MIMO_DIMENSION_REDUCED";
    case TIME_SLICED:                         return "TIME_SLICED";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eCacMethod value) { return out << eCacMethod_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


class tlvProfile2CacCapabilities : public BaseClass
{
    public:
        tlvProfile2CacCapabilities(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvProfile2CacCapabilities(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvProfile2CacCapabilities();

        const eTlvTypeMap& type();
        const uint16_t& length();
        uint8_t* country_code(size_t idx = 0);
        bool set_country_code(const void* buffer, size_t size);
        uint8_t& number_of_cac_radios();
        std::tuple<bool, cCacCapabilitiesRadio&> cac_radios(size_t idx);
        std::shared_ptr<cCacCapabilitiesRadio> create_cac_radios();
        bool add_cac_radios(std::shared_ptr<cCacCapabilitiesRadio> ptr);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        uint8_t* m_country_code = nullptr;
        size_t m_country_code_idx__ = 0;
        int m_lock_order_counter__ = 0;
        uint8_t* m_number_of_cac_radios = nullptr;
        cCacCapabilitiesRadio* m_cac_radios = nullptr;
        size_t m_cac_radios_idx__ = 0;
        std::vector<std::shared_ptr<cCacCapabilitiesRadio>> m_cac_radios_vector;
        bool m_lock_allocation__ = false;
};

class cCacCapabilitiesRadio : public BaseClass
{
    public:
        cCacCapabilitiesRadio(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cCacCapabilitiesRadio(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cCacCapabilitiesRadio();

        sMacAddr& radio_uid();
        uint8_t& number_of_cac_type_supported();
        std::tuple<bool, cCacTypes&> cac_types(size_t idx);
        std::shared_ptr<cCacTypes> create_cac_types();
        bool add_cac_types(std::shared_ptr<cCacTypes> ptr);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        sMacAddr* m_radio_uid = nullptr;
        uint8_t* m_number_of_cac_type_supported = nullptr;
        cCacTypes* m_cac_types = nullptr;
        size_t m_cac_types_idx__ = 0;
        std::vector<std::shared_ptr<cCacTypes>> m_cac_types_vector;
        bool m_lock_allocation__ = false;
        int m_lock_order_counter__ = 0;
};

class cCacTypes : public BaseClass
{
    public:
        cCacTypes(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cCacTypes(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cCacTypes();

        eCacMethod& cac_method();
        uint8_t* duration(size_t idx = 0);
        bool set_duration(const void* buffer, size_t size);
        uint8_t& number_of_operating_classes();
        std::tuple<bool, cCacCapabilitiesOperatingClasses&> operating_classes(size_t idx);
        std::shared_ptr<cCacCapabilitiesOperatingClasses> create_operating_classes();
        bool add_operating_classes(std::shared_ptr<cCacCapabilitiesOperatingClasses> ptr);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eCacMethod* m_cac_method = nullptr;
        uint8_t* m_duration = nullptr;
        size_t m_duration_idx__ = 0;
        int m_lock_order_counter__ = 0;
        uint8_t* m_number_of_operating_classes = nullptr;
        cCacCapabilitiesOperatingClasses* m_operating_classes = nullptr;
        size_t m_operating_classes_idx__ = 0;
        std::vector<std::shared_ptr<cCacCapabilitiesOperatingClasses>> m_operating_classes_vector;
        bool m_lock_allocation__ = false;
};

class cCacCapabilitiesOperatingClasses : public BaseClass
{
    public:
        cCacCapabilitiesOperatingClasses(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cCacCapabilitiesOperatingClasses(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cCacCapabilitiesOperatingClasses();

        uint8_t& operating_class();
        uint8_t& number_of_channels();
        uint8_t* channels(size_t idx = 0);
        bool set_channels(const void* buffer, size_t size);
        bool alloc_channels(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        uint8_t* m_operating_class = nullptr;
        uint8_t* m_number_of_channels = nullptr;
        uint8_t* m_channels = nullptr;
        size_t m_channels_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVPROFILE2CACCAPABILITIES_H_
