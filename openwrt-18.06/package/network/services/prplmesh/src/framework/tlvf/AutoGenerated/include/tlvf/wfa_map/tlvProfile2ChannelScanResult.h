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

#ifndef _TLVF_WFA_MAP_TLVPROFILE2CHANNELSCANRESULT_H_
#define _TLVF_WFA_MAP_TLVPROFILE2CHANNELSCANRESULT_H_

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
#include <vector>
#include <ostream>

namespace wfa_map {

class cNeighbors;

class tlvProfile2ChannelScanResult : public BaseClass
{
    public:
        tlvProfile2ChannelScanResult(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvProfile2ChannelScanResult(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvProfile2ChannelScanResult();

        enum eScanStatus: uint8_t {
            SUCCESS = 0x0,
            SCAN_NOT_SUPPORTED_ON_THIS_OPERATING_CLASS_AND_CHANNEL_ON_THIS_RADIO = 0x1,
            REQUEST_TOO_SOON_AFTER_LAST_SCAN = 0x2,
            RADIO_TOO_BUSY_TO_PERFORM_SCAN = 0x3,
            SCAN_NOT_COMPLETED = 0x4,
            SCAN_ABORTED = 0x5,
            FRESH_SCAN_NOT_SUPPORTED_RADIO_ONLY_SUPPORTS_ON_BOOT_SCANS = 0x6,
        };
        // Enum AutoPrint generated code snippet begining- DON'T EDIT!
        // clang-format off
        static const char *eScanStatus_str(eScanStatus enum_value) {
            switch (enum_value) {
            case SUCCESS:                                                              return "SUCCESS";
            case SCAN_NOT_SUPPORTED_ON_THIS_OPERATING_CLASS_AND_CHANNEL_ON_THIS_RADIO: return "SCAN_NOT_SUPPORTED_ON_THIS_OPERATING_CLASS_AND_CHANNEL_ON_THIS_RADIO";
            case REQUEST_TOO_SOON_AFTER_LAST_SCAN:                                     return "REQUEST_TOO_SOON_AFTER_LAST_SCAN";
            case RADIO_TOO_BUSY_TO_PERFORM_SCAN:                                       return "RADIO_TOO_BUSY_TO_PERFORM_SCAN";
            case SCAN_NOT_COMPLETED:                                                   return "SCAN_NOT_COMPLETED";
            case SCAN_ABORTED:                                                         return "SCAN_ABORTED";
            case FRESH_SCAN_NOT_SUPPORTED_RADIO_ONLY_SUPPORTS_ON_BOOT_SCANS:           return "FRESH_SCAN_NOT_SUPPORTED_RADIO_ONLY_SUPPORTS_ON_BOOT_SCANS";
            }
            static std::string out_str = std::to_string(int(enum_value));
            return out_str.c_str();
        }
        friend inline std::ostream &operator<<(std::ostream &out, eScanStatus value) { return out << eScanStatus_str(value); }
        // clang-format on
        // Enum AutoPrint generated code snippet end
        
        enum eScanType: uint8_t {
            SCAN_WAS_ACTIVE_SCAN = 0x80,
            SCAN_WAS_PASSIVE_SCAN = 0x0,
        };
        // Enum AutoPrint generated code snippet begining- DON'T EDIT!
        // clang-format off
        static const char *eScanType_str(eScanType enum_value) {
            switch (enum_value) {
            case SCAN_WAS_ACTIVE_SCAN:  return "SCAN_WAS_ACTIVE_SCAN";
            case SCAN_WAS_PASSIVE_SCAN: return "SCAN_WAS_PASSIVE_SCAN";
            }
            static std::string out_str = std::to_string(int(enum_value));
            return out_str.c_str();
        }
        friend inline std::ostream &operator<<(std::ostream &out, eScanType value) { return out << eScanType_str(value); }
        // clang-format on
        // Enum AutoPrint generated code snippet end
        
        const eTlvTypeMap& type();
        const uint16_t& length();
        sMacAddr& radio_uid();
        uint8_t& operating_class();
        uint8_t& channel();
        eScanStatus& success();
        uint8_t& timestamp_length();
        //The timestamp shall be formatted as a string using the typedef dateandtime string
        //format as defined in section 3 of [1] and shall include timesecfrac and time-offset
        //as defined in section 5.6 of [1]
        std::string timestamp_str();
        char* timestamp(size_t length = 0);
        bool set_timestamp(const std::string& str);
        bool set_timestamp(const char buffer[], size_t size);
        bool alloc_timestamp(size_t count = 1);
        //The current channel utilization measured by the radio on the scanned 20 MHz channel
        uint8_t& utilization();
        //An indicator of the average radio noise plus interference power measured
        //on the 20 MHz channel during a channel scan.
        uint8_t& noise();
        uint16_t& neighbors_list_length();
        std::tuple<bool, cNeighbors&> neighbors_list(size_t idx);
        std::shared_ptr<cNeighbors> create_neighbors_list();
        bool add_neighbors_list(std::shared_ptr<cNeighbors> ptr);
        //Total time spent performing the scan of this channel in milliseconds
        uint32_t& aggregate_scan_duration();
        eScanType& scan_type();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        sMacAddr* m_radio_uid = nullptr;
        uint8_t* m_operating_class = nullptr;
        uint8_t* m_channel = nullptr;
        eScanStatus* m_success = nullptr;
        uint8_t* m_timestamp_length = nullptr;
        char* m_timestamp = nullptr;
        size_t m_timestamp_idx__ = 0;
        int m_lock_order_counter__ = 0;
        uint8_t* m_utilization = nullptr;
        uint8_t* m_noise = nullptr;
        uint16_t* m_neighbors_list_length = nullptr;
        cNeighbors* m_neighbors_list = nullptr;
        size_t m_neighbors_list_idx__ = 0;
        std::vector<std::shared_ptr<cNeighbors>> m_neighbors_list_vector;
        bool m_lock_allocation__ = false;
        uint32_t* m_aggregate_scan_duration = nullptr;
        eScanType* m_scan_type = nullptr;
};

class cNeighbors : public BaseClass
{
    public:
        cNeighbors(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cNeighbors(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cNeighbors();

        enum eBssLoadElementPresent: uint8_t {
            FIELD_PRESENT = 0x80,
            FIELD_NOT_PRESENT = 0x0,
        };
        // Enum AutoPrint generated code snippet begining- DON'T EDIT!
        // clang-format off
        static const char *eBssLoadElementPresent_str(eBssLoadElementPresent enum_value) {
            switch (enum_value) {
            case FIELD_PRESENT:     return "FIELD_PRESENT";
            case FIELD_NOT_PRESENT: return "FIELD_NOT_PRESENT";
            }
            static std::string out_str = std::to_string(int(enum_value));
            return out_str.c_str();
        }
        friend inline std::ostream &operator<<(std::ostream &out, eBssLoadElementPresent value) { return out << eBssLoadElementPresent_str(value); }
        // clang-format on
        // Enum AutoPrint generated code snippet end
        
        sMacAddr& bssid();
        uint8_t& ssid_length();
        //The SSID indicated by the neighboring BSS
        std::string ssid_str();
        char* ssid(size_t length = 0);
        bool set_ssid(const std::string& str);
        bool set_ssid(const char buffer[], size_t size);
        bool alloc_ssid(size_t count = 1);
        //An indicator of radio signal strength (RSSI) of the Beacon or Probe
        //Response frames of the neighboring BSS as received by the radio
        //measured in dBm
        uint8_t& signal_strength();
        uint8_t& channel_bw_length();
        //String indicating the maximum bandwidth at which the neighbor BSS is
        //operating, e.g., "20" or "40" or "80" or "80+80" or "160" MHz.
        std::string channels_bw_list_str();
        char* channels_bw_list(size_t length = 0);
        bool set_channels_bw_list(const std::string& str);
        bool set_channels_bw_list(const char buffer[], size_t size);
        bool alloc_channels_bw_list(size_t count = 1);
        eBssLoadElementPresent& bss_load_element_present();
        bool alloc_channel_utilization();
        uint8_t* channel_utilization();
        bool set_channel_utilization(const uint8_t channel_utilization);
        bool alloc_station_count();
        uint16_t* station_count();
        bool set_station_count(const uint16_t station_count);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        sMacAddr* m_bssid = nullptr;
        uint8_t* m_ssid_length = nullptr;
        char* m_ssid = nullptr;
        size_t m_ssid_idx__ = 0;
        int m_lock_order_counter__ = 0;
        uint8_t* m_signal_strength = nullptr;
        uint8_t* m_channel_bw_length = nullptr;
        char* m_channels_bw_list = nullptr;
        size_t m_channels_bw_list_idx__ = 0;
        eBssLoadElementPresent* m_bss_load_element_present = nullptr;
        uint8_t* m_channel_utilization = nullptr;
        bool m_channel_utilization_allocated = false;
        uint16_t* m_station_count = nullptr;
        bool m_station_count_allocated = false;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVPROFILE2CHANNELSCANRESULT_H_
