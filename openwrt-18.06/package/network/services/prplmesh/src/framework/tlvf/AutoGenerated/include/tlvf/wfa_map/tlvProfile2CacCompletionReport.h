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

#ifndef _TLVF_WFA_MAP_TLVPROFILE2CACCOMPLETIONREPORT_H_
#define _TLVF_WFA_MAP_TLVPROFILE2CACCOMPLETIONREPORT_H_

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
#include <ostream>

namespace wfa_map {

class cCacCompletionReportRadio;

class tlvProfile2CacCompletionReport : public BaseClass
{
    public:
        tlvProfile2CacCompletionReport(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvProfile2CacCompletionReport(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvProfile2CacCompletionReport();

        const eTlvTypeMap& type();
        const uint16_t& length();
        uint8_t& number_of_cac_radios();
        std::tuple<bool, cCacCompletionReportRadio&> cac_radios(size_t idx);
        std::shared_ptr<cCacCompletionReportRadio> create_cac_radios();
        bool add_cac_radios(std::shared_ptr<cCacCompletionReportRadio> ptr);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        uint8_t* m_number_of_cac_radios = nullptr;
        cCacCompletionReportRadio* m_cac_radios = nullptr;
        size_t m_cac_radios_idx__ = 0;
        std::vector<std::shared_ptr<cCacCompletionReportRadio>> m_cac_radios_vector;
        bool m_lock_allocation__ = false;
        int m_lock_order_counter__ = 0;
};

class cCacCompletionReportRadio : public BaseClass
{
    public:
        cCacCompletionReportRadio(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cCacCompletionReportRadio(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cCacCompletionReportRadio();

        typedef struct sCacDetectedPair {
            uint8_t operating_class_detected;
            uint8_t channel_detected;
            void struct_swap(){
            }
            void struct_init(){
            }
        } __attribute__((packed)) sCacDetectedPair;
        
        enum eCompletionStatus: uint8_t {
            SUCCESSFUL = 0x0,
            RADAR_DETECTED = 0x1,
            CAC_NOT_SUPPORTED = 0x2,
            RADIO_TOO_BUSY = 0x3,
            NOT_UNDER_REGULATION = 0x4,
            OTHER_ERROR = 0x5,
            NOT_PERFORMED = 0xff,
        };
        // Enum AutoPrint generated code snippet begining- DON'T EDIT!
        // clang-format off
        static const char *eCompletionStatus_str(eCompletionStatus enum_value) {
            switch (enum_value) {
            case SUCCESSFUL:           return "SUCCESSFUL";
            case RADAR_DETECTED:       return "RADAR_DETECTED";
            case CAC_NOT_SUPPORTED:    return "CAC_NOT_SUPPORTED";
            case RADIO_TOO_BUSY:       return "RADIO_TOO_BUSY";
            case NOT_UNDER_REGULATION: return "NOT_UNDER_REGULATION";
            case OTHER_ERROR:          return "OTHER_ERROR";
            case NOT_PERFORMED:        return "NOT_PERFORMED";
            }
            static std::string out_str = std::to_string(int(enum_value));
            return out_str.c_str();
        }
        friend inline std::ostream &operator<<(std::ostream &out, eCompletionStatus value) { return out << eCompletionStatus_str(value); }
        // clang-format on
        // Enum AutoPrint generated code snippet end
        
        sMacAddr& radio_uid();
        uint8_t& operating_class();
        uint8_t& channel();
        eCompletionStatus& cac_completion_status();
        uint8_t& number_of_detected_pairs();
        std::tuple<bool, sCacDetectedPair&> detected_pairs(size_t idx);
        bool alloc_detected_pairs(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        sMacAddr* m_radio_uid = nullptr;
        uint8_t* m_operating_class = nullptr;
        uint8_t* m_channel = nullptr;
        eCompletionStatus* m_cac_completion_status = nullptr;
        uint8_t* m_number_of_detected_pairs = nullptr;
        sCacDetectedPair* m_detected_pairs = nullptr;
        size_t m_detected_pairs_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVPROFILE2CACCOMPLETIONREPORT_H_
