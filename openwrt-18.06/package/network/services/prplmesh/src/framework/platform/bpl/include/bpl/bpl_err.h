/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BPL_ERR_H_
#define _BPL_ERR_H_

#include <ostream>

namespace beerocks {
namespace bpl {

/****************************************************************************/
/******************************* Definitions ********************************/
/****************************************************************************/

/* Beerocks Error Definitions */
#define BPL_ERROR_STRING_LEN 256 /* Maximal length of BPL error string */

// Helper MACROs for Enum/String generation
#define GENERATE_ERROR_ENUM(ENUM) ENUM,
#define GENERATE_ERROR_STRING(STRING) "BPL_ERR_" #STRING,

/* Beerocks Error Codes */
// Error codes ENUM
enum class eErrorCode {
    /* 00 */ NONE,
    /* 01 */ UNKNOWN,
    /* 02 */ OPERATION_NOT_SUPPORTED,
    /* 03 */ BH_READING_DATA_FROM_THE_BRIDGE,
    /* 04 */ BH_TIMEOUT_ATTACHING_TO_WPA_SUPPLICANT,
    /* 05 */ BH_SCAN_FAILED_TO_INITIATE_SCAN,
    /* 06 */ BH_SCAN_TIMEOUT,
    /* 07 */ BH_SCAN_EXCEEDED_MAXIMUM_FAILED_SCAN_ATTEMPTS,
    /* 08 */ BH_CONNECTING_TO_MASTER,
    /* 09 */ BH_ASSOCIATE_4ADDR_TIMEOUT,
    /* 10 */ BH_ASSOCIATE_4ADDR_FAILURE,
    /* 11 */ BH_ROAMING,
    /* 12 */ BH_DISCONNECTED,
    /* 13 */ BH_WPA_SUPPLICANT_TERMINATED,
    /* 14 */ BH_SLAVE_SOCKET_DISCONNECTED,
    /* 15 */ BH_STOPPED,
    /* 16 */ SLAVE_CONNECTING_TO_BACKHAUL_MANAGER,
    /* 17 */ SLAVE_INVALID_MASTER_SOCKET,
    /* 18 */ SLAVE_FAILED_CONNECT_TO_PLATFORM_MANAGER,
    /* 19 */ SLAVE_PLATFORM_MANAGER_REGISTER_TIMEOUT,
    /* 20 */ SLAVE_SLAVE_BACKHAUL_MANAGER_DISCONNECTED,
    /* 21 */ SLAVE_STOPPED,
    /* 22 */ AP_MANAGER_START,
    /* 23 */ AP_MANAGER_DISCONNECTED,
    /* 24 */ AP_MANAGER_HOSTAP_DISABLED,
    /* 25 */ AP_MANAGER_ATTACH_FAIL,
    /* 26 */ AP_MANAGER_SUDDEN_DETACH,
    /* 27 */ AP_MANAGER_HAL_DISCONNECTED,
    /* 28 */ AP_MANAGER_CAC_TIMEOUT,
    /* 29 */ MONITOR_DISCONNECTED,
    /* 30 */ MONITOR_HOSTAP_DISABLED,
    /* 31 */ MONITOR_ATTACH_FAIL,
    /* 32 */ MONITOR_SUDDEN_DETACH,
    /* 33 */ MONITOR_HAL_DISCONNECTED,
    /* 34 */ MONITOR_REPORT_PROCESS_FAIL,
    /* 35 */ CONFIG_PLATFORM_REPORTED_INVALID_CONFIGURATION,
    /* 36 */ CONFIG_NO_VALID_BACKHAUL_INTERFACE,
    /* 37 */ WATCHDOG_PROCESS_STUCK,
    /* 38 */ WATCHDOG_PROCESS_ZOMBIE,
    /* 39 */ LAST,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eErrorCode_str(eErrorCode enum_value) {
    switch (enum_value) {
    case eErrorCode::NONE:                                           return "eErrorCode::NONE";
    case eErrorCode::UNKNOWN:                                        return "eErrorCode::UNKNOWN";
    case eErrorCode::OPERATION_NOT_SUPPORTED:                        return "eErrorCode::OPERATION_NOT_SUPPORTED";
    case eErrorCode::BH_READING_DATA_FROM_THE_BRIDGE:                return "eErrorCode::BH_READING_DATA_FROM_THE_BRIDGE";
    case eErrorCode::BH_TIMEOUT_ATTACHING_TO_WPA_SUPPLICANT:         return "eErrorCode::BH_TIMEOUT_ATTACHING_TO_WPA_SUPPLICANT";
    case eErrorCode::BH_SCAN_FAILED_TO_INITIATE_SCAN:                return "eErrorCode::BH_SCAN_FAILED_TO_INITIATE_SCAN";
    case eErrorCode::BH_SCAN_TIMEOUT:                                return "eErrorCode::BH_SCAN_TIMEOUT";
    case eErrorCode::BH_SCAN_EXCEEDED_MAXIMUM_FAILED_SCAN_ATTEMPTS:  return "eErrorCode::BH_SCAN_EXCEEDED_MAXIMUM_FAILED_SCAN_ATTEMPTS";
    case eErrorCode::BH_CONNECTING_TO_MASTER:                        return "eErrorCode::BH_CONNECTING_TO_MASTER";
    case eErrorCode::BH_ASSOCIATE_4ADDR_TIMEOUT:                     return "eErrorCode::BH_ASSOCIATE_4ADDR_TIMEOUT";
    case eErrorCode::BH_ASSOCIATE_4ADDR_FAILURE:                     return "eErrorCode::BH_ASSOCIATE_4ADDR_FAILURE";
    case eErrorCode::BH_ROAMING:                                     return "eErrorCode::BH_ROAMING";
    case eErrorCode::BH_DISCONNECTED:                                return "eErrorCode::BH_DISCONNECTED";
    case eErrorCode::BH_WPA_SUPPLICANT_TERMINATED:                   return "eErrorCode::BH_WPA_SUPPLICANT_TERMINATED";
    case eErrorCode::BH_SLAVE_SOCKET_DISCONNECTED:                   return "eErrorCode::BH_SLAVE_SOCKET_DISCONNECTED";
    case eErrorCode::BH_STOPPED:                                     return "eErrorCode::BH_STOPPED";
    case eErrorCode::SLAVE_CONNECTING_TO_BACKHAUL_MANAGER:           return "eErrorCode::SLAVE_CONNECTING_TO_BACKHAUL_MANAGER";
    case eErrorCode::SLAVE_INVALID_MASTER_SOCKET:                    return "eErrorCode::SLAVE_INVALID_MASTER_SOCKET";
    case eErrorCode::SLAVE_FAILED_CONNECT_TO_PLATFORM_MANAGER:       return "eErrorCode::SLAVE_FAILED_CONNECT_TO_PLATFORM_MANAGER";
    case eErrorCode::SLAVE_PLATFORM_MANAGER_REGISTER_TIMEOUT:        return "eErrorCode::SLAVE_PLATFORM_MANAGER_REGISTER_TIMEOUT";
    case eErrorCode::SLAVE_SLAVE_BACKHAUL_MANAGER_DISCONNECTED:      return "eErrorCode::SLAVE_SLAVE_BACKHAUL_MANAGER_DISCONNECTED";
    case eErrorCode::SLAVE_STOPPED:                                  return "eErrorCode::SLAVE_STOPPED";
    case eErrorCode::AP_MANAGER_START:                               return "eErrorCode::AP_MANAGER_START";
    case eErrorCode::AP_MANAGER_DISCONNECTED:                        return "eErrorCode::AP_MANAGER_DISCONNECTED";
    case eErrorCode::AP_MANAGER_HOSTAP_DISABLED:                     return "eErrorCode::AP_MANAGER_HOSTAP_DISABLED";
    case eErrorCode::AP_MANAGER_ATTACH_FAIL:                         return "eErrorCode::AP_MANAGER_ATTACH_FAIL";
    case eErrorCode::AP_MANAGER_SUDDEN_DETACH:                       return "eErrorCode::AP_MANAGER_SUDDEN_DETACH";
    case eErrorCode::AP_MANAGER_HAL_DISCONNECTED:                    return "eErrorCode::AP_MANAGER_HAL_DISCONNECTED";
    case eErrorCode::AP_MANAGER_CAC_TIMEOUT:                         return "eErrorCode::AP_MANAGER_CAC_TIMEOUT";
    case eErrorCode::MONITOR_DISCONNECTED:                           return "eErrorCode::MONITOR_DISCONNECTED";
    case eErrorCode::MONITOR_HOSTAP_DISABLED:                        return "eErrorCode::MONITOR_HOSTAP_DISABLED";
    case eErrorCode::MONITOR_ATTACH_FAIL:                            return "eErrorCode::MONITOR_ATTACH_FAIL";
    case eErrorCode::MONITOR_SUDDEN_DETACH:                          return "eErrorCode::MONITOR_SUDDEN_DETACH";
    case eErrorCode::MONITOR_HAL_DISCONNECTED:                       return "eErrorCode::MONITOR_HAL_DISCONNECTED";
    case eErrorCode::MONITOR_REPORT_PROCESS_FAIL:                    return "eErrorCode::MONITOR_REPORT_PROCESS_FAIL";
    case eErrorCode::CONFIG_PLATFORM_REPORTED_INVALID_CONFIGURATION: return "eErrorCode::CONFIG_PLATFORM_REPORTED_INVALID_CONFIGURATION";
    case eErrorCode::CONFIG_NO_VALID_BACKHAUL_INTERFACE:             return "eErrorCode::CONFIG_NO_VALID_BACKHAUL_INTERFACE";
    case eErrorCode::WATCHDOG_PROCESS_STUCK:                         return "eErrorCode::WATCHDOG_PROCESS_STUCK";
    case eErrorCode::WATCHDOG_PROCESS_ZOMBIE:                        return "eErrorCode::WATCHDOG_PROCESS_ZOMBIE";
    case eErrorCode::LAST:                                           return "eErrorCode::LAST";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eErrorCode value) { return out << eErrorCode_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end

} // namespace bpl
} // namespace beerocks

#endif /* _BPL_ERR_H_ */
