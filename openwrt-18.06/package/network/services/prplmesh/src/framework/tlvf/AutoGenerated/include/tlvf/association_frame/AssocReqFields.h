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

#ifndef _TLVF_ASSOCIATION_FRAME_ASSOCREQFIELDS_H_
#define _TLVF_ASSOCIATION_FRAME_ASSOCREQFIELDS_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include <tuple>
#include <ostream>
#include "tlvf/common/sMacAddr.h"
#include "tlvf/association_frame/eElementID.h"
#include "tlvf/AssociationRequestFrame/assoc_frame_bitfields.h"

namespace assoc_frame {

class cRSN;
class cSupportedOpClasses;
class cSupportRates;
class cExtendedSupportRates;
class cPowerCapability;
class cBssCoexistence20_40;
class cQosTrafficCap;
class cTimBroadcastRequest;
class cInterworking;
class cMultiBand;
class cDmgCapabilities;
class cMultipleMacSublayers;
class cOperatingModeNotify;
class cVendorSpecific;
class cUnknownField;

class cSSID : public BaseClass
{
    public:
        cSSID(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cSSID(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cSSID();

        eElementID& type();
        const uint8_t& length();
        size_t ssid_length() { return m_ssid_idx__ * sizeof(char); }
        std::string ssid_str();
        char* ssid(size_t length = 0);
        bool set_ssid(const std::string& str);
        bool set_ssid(const char buffer[], size_t size);
        bool alloc_ssid(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        char* m_ssid = nullptr;
        size_t m_ssid_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

class cRSN : public BaseClass
{
    public:
        cRSN(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cRSN(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cRSN();

        eElementID& type();
        const uint8_t& length();
        uint16_t& version();
        size_t optional_length() { return m_optional_idx__ * sizeof(uint8_t); }
        uint8_t* optional(size_t idx = 0);
        bool set_optional(const void* buffer, size_t size);
        bool alloc_optional(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint16_t* m_version = nullptr;
        uint8_t* m_optional = nullptr;
        size_t m_optional_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

class cSupportedOpClasses : public BaseClass
{
    public:
        cSupportedOpClasses(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cSupportedOpClasses(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cSupportedOpClasses();

        eElementID& type();
        const uint8_t& length();
        uint8_t& current_op_class();
        size_t op_classes_length() { return m_op_classes_idx__ * sizeof(uint8_t); }
        uint8_t* op_classes(size_t idx = 0);
        bool set_op_classes(const void* buffer, size_t size);
        bool alloc_op_classes(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_current_op_class = nullptr;
        uint8_t* m_op_classes = nullptr;
        size_t m_op_classes_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

class cSupportRates : public BaseClass
{
    public:
        cSupportRates(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cSupportRates(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cSupportRates();

        eElementID& type();
        const uint8_t& length();
        size_t supported_rated_length() { return m_supported_rated_idx__ * sizeof(uint8_t); }
        uint8_t* supported_rated(size_t idx = 0);
        bool set_supported_rated(const void* buffer, size_t size);
        bool alloc_supported_rated(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_supported_rated = nullptr;
        size_t m_supported_rated_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

class cExtendedSupportRates : public BaseClass
{
    public:
        cExtendedSupportRates(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cExtendedSupportRates(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cExtendedSupportRates();

        eElementID& type();
        const uint8_t& length();
        size_t extended_suport_rated_length() { return m_extended_suport_rated_idx__ * sizeof(uint8_t); }
        uint8_t* extended_suport_rated(size_t idx = 0);
        bool set_extended_suport_rated(const void* buffer, size_t size);
        bool alloc_extended_suport_rated(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_extended_suport_rated = nullptr;
        size_t m_extended_suport_rated_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

class cPowerCapability : public BaseClass
{
    public:
        cPowerCapability(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cPowerCapability(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cPowerCapability();

        eElementID& type();
        const uint8_t& length();
        uint8_t& min_tx_power();
        uint8_t& max_tx_power();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_min_tx_power = nullptr;
        uint8_t* m_max_tx_power = nullptr;
};

class cBssCoexistence20_40 : public BaseClass
{
    public:
        cBssCoexistence20_40(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cBssCoexistence20_40(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cBssCoexistence20_40();

        eElementID& type();
        const uint8_t& length();
        uint8_t& info_field();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_info_field = nullptr;
};

class cQosTrafficCap : public BaseClass
{
    public:
        cQosTrafficCap(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cQosTrafficCap(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cQosTrafficCap();

        eElementID& type();
        const uint8_t& length();
        //QoS traffic capability bitmask/flags
        uint8_t& flags();
        //Total number of nonzero bits in Bits 0-1 of 'flags'
        uint8_t& ac_sta_count_list();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_flags = nullptr;
        uint8_t* m_ac_sta_count_list = nullptr;
};

class cTimBroadcastRequest : public BaseClass
{
    public:
        cTimBroadcastRequest(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cTimBroadcastRequest(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cTimBroadcastRequest();

        eElementID& type();
        const uint8_t& length();
        uint8_t& tim_brdcast_interval();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_tim_brdcast_interval = nullptr;
};

class cInterworking : public BaseClass
{
    public:
        cInterworking(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cInterworking(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cInterworking();

        eElementID& type();
        const uint8_t& length();
        uint8_t& ac_network_options();
        size_t optional_params_length() { return m_optional_params_idx__ * sizeof(uint8_t); }
        uint8_t* optional_params(size_t idx = 0);
        bool set_optional_params(const void* buffer, size_t size);
        bool alloc_optional_params(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_ac_network_options = nullptr;
        uint8_t* m_optional_params = nullptr;
        size_t m_optional_params_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

class cMultiBand : public BaseClass
{
    public:
        cMultiBand(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cMultiBand(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cMultiBand();

        enum eMultiBandId: uint8_t {
            BAND_TV_WHITE_SPACES = 0x0,
            BAND_SUB_1_GHZ = 0x1,
            BAND_2_4_GHZ = 0x2,
            BAND_3_6_GHZ = 0x3,
            BAND_4_9_AND_5_GHZ = 0x4,
            BAND_60_GHZ = 0x5,
            BAND_45_GHZ = 0x6,
        };
        // Enum AutoPrint generated code snippet begining- DON'T EDIT!
        // clang-format off
        static const char *eMultiBandId_str(eMultiBandId enum_value) {
            switch (enum_value) {
            case BAND_TV_WHITE_SPACES: return "BAND_TV_WHITE_SPACES";
            case BAND_SUB_1_GHZ:       return "BAND_SUB_1_GHZ";
            case BAND_2_4_GHZ:         return "BAND_2_4_GHZ";
            case BAND_3_6_GHZ:         return "BAND_3_6_GHZ";
            case BAND_4_9_AND_5_GHZ:   return "BAND_4_9_AND_5_GHZ";
            case BAND_60_GHZ:          return "BAND_60_GHZ";
            case BAND_45_GHZ:          return "BAND_45_GHZ";
            }
            static std::string out_str = std::to_string(int(enum_value));
            return out_str.c_str();
        }
        friend inline std::ostream &operator<<(std::ostream &out, eMultiBandId value) { return out << eMultiBandId_str(value); }
        // clang-format on
        // Enum AutoPrint generated code snippet end
        
        eElementID& type();
        const uint8_t& length();
        uint8_t& multi_band_control();
        uint8_t& band_id();
        uint8_t& op_class();
        uint8_t& channel_num();
        sMacAddr& bssid();
        uint8_t& beacon_interval();
        uint8_t* tsf_offset(size_t idx = 0);
        bool set_tsf_offset(const void* buffer, size_t size);
        uint8_t& multi_band_con_cap();
        uint8_t& fst_session_timeout();
        uint8_t& optional();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_multi_band_control = nullptr;
        uint8_t* m_band_id = nullptr;
        uint8_t* m_op_class = nullptr;
        uint8_t* m_channel_num = nullptr;
        sMacAddr* m_bssid = nullptr;
        uint8_t* m_beacon_interval = nullptr;
        uint8_t* m_tsf_offset = nullptr;
        size_t m_tsf_offset_idx__ = 0;
        int m_lock_order_counter__ = 0;
        uint8_t* m_multi_band_con_cap = nullptr;
        uint8_t* m_fst_session_timeout = nullptr;
        uint8_t* m_optional = nullptr;
};

class cDmgCapabilities : public BaseClass
{
    public:
        cDmgCapabilities(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cDmgCapabilities(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cDmgCapabilities();

        eElementID& type();
        const uint8_t& length();
        sMacAddr& bssid();
        uint8_t& aid();
        uint8_t* dmg_sta_cap_info(size_t idx = 0);
        bool set_dmg_sta_cap_info(const void* buffer, size_t size);
        //DMG AP or PCP capability info
        uint16_t& dmg_ap();
        uint16_t& dmg_sta_beam_track_time_lim();
        uint8_t& extended_sc_mcs_cap();
        uint8_t& basic_amsdu_subframe_max_num();
        uint8_t& short_amsdu_subframe_max_num();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        sMacAddr* m_bssid = nullptr;
        uint8_t* m_aid = nullptr;
        uint8_t* m_dmg_sta_cap_info = nullptr;
        size_t m_dmg_sta_cap_info_idx__ = 0;
        int m_lock_order_counter__ = 0;
        uint16_t* m_dmg_ap = nullptr;
        uint16_t* m_dmg_sta_beam_track_time_lim = nullptr;
        uint8_t* m_extended_sc_mcs_cap = nullptr;
        uint8_t* m_basic_amsdu_subframe_max_num = nullptr;
        uint8_t* m_short_amsdu_subframe_max_num = nullptr;
};

class cMultipleMacSublayers : public BaseClass
{
    public:
        cMultipleMacSublayers(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cMultipleMacSublayers(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cMultipleMacSublayers();

        eElementID& type();
        const uint8_t& length();
        uint8_t& mms_control();
        sMacAddr& sta_mac();
        size_t interface_addr_length() { return m_interface_addr_idx__ * sizeof(uint8_t); }
        uint8_t* interface_addr(size_t idx = 0);
        bool set_interface_addr(const void* buffer, size_t size);
        bool alloc_interface_addr(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_mms_control = nullptr;
        sMacAddr* m_sta_mac = nullptr;
        uint8_t* m_interface_addr = nullptr;
        size_t m_interface_addr_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

class cOperatingModeNotify : public BaseClass
{
    public:
        cOperatingModeNotify(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cOperatingModeNotify(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cOperatingModeNotify();

        eElementID& type();
        const uint8_t& length();
        uint8_t& op_mode();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_op_mode = nullptr;
};

class cVendorSpecific : public BaseClass
{
    public:
        cVendorSpecific(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cVendorSpecific(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cVendorSpecific();

        eElementID& type();
        const uint8_t& length();
        uint8_t* oui(size_t idx = 0);
        bool set_oui(const void* buffer, size_t size);
        uint8_t& oui_type();
        size_t data_length() { return m_data_idx__ * sizeof(uint8_t); }
        uint8_t* data(size_t idx = 0);
        bool set_data(const void* buffer, size_t size);
        bool alloc_data(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_oui = nullptr;
        size_t m_oui_idx__ = 0;
        int m_lock_order_counter__ = 0;
        uint8_t* m_oui_type = nullptr;
        uint8_t* m_data = nullptr;
        size_t m_data_idx__ = 0;
};

class cUnknownField : public BaseClass
{
    public:
        cUnknownField(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cUnknownField(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cUnknownField();

        eElementID& type();
        const uint8_t& length();
        size_t data_length() { return m_data_idx__ * sizeof(uint8_t); }
        uint8_t* data(size_t idx = 0);
        bool set_data(const void* buffer, size_t size);
        bool alloc_data(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_data = nullptr;
        size_t m_data_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

}; // close namespace: assoc_frame

#endif //_TLVF/ASSOCIATION_FRAME_ASSOCREQFIELDS_H_
