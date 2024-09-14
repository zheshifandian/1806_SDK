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

#ifndef _TLVF_WSC_EWSCATTRIBUTES_H_
#define _TLVF_WSC_EWSCATTRIBUTES_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace WSC {

enum eWscAttributes: uint16_t {
    ATTR_ASSOC_STATE = 0x1002,
    ATTR_AUTH_TYPE = 0x1003,
    ATTR_AUTH_TYPE_FLAGS = 0x1004,
    ATTR_AUTHENTICATOR = 0x1005,
    ATTR_CONFIG_METHODS = 0x1008,
    ATTR_CONFIG_ERROR = 0x1009,
    ATTR_CONN_TYPE_FLAGS = 0x100d,
    ATTR_ENCR_TYPE = 0x100f,
    ATTR_ENCR_TYPE_FLAGS = 0x1010,
    ATTR_DEV_NAME = 0x1011,
    ATTR_DEV_PASSWORD_ID = 0x1012,
    ATTR_ENCR_SETTINGS = 0x1018,
    ATTR_ENROLLEE_NONCE = 0x101a,
    ATTR_KEY_WRAP_AUTH = 0x101e,
    ATTR_MAC_ADDR = 0x1020,
    ATTR_MANUFACTURER = 0x1021,
    ATTR_MSG_TYPE = 0x1022,
    ATTR_MODEL_NAME = 0x1023,
    ATTR_MODEL_NUMBER = 0x1024,
    ATTR_NETWORK_KEY = 0x1027,
    ATTR_OS_VERSION = 0x102d,
    ATTR_PUBLIC_KEY = 0x1032,
    ATTR_REGISTRAR_NONCE = 0x1039,
    ATTR_RF_BANDS = 0x103c,
    ATTR_SERIAL_NUMBER = 0x1042,
    ATTR_WSC_STATE = 0x1044,
    ATTR_SSID = 0x1045,
    ATTR_UUID_E = 0x1047,
    ATTR_UUID_R = 0x1048,
    ATTR_VENDOR_EXTENSION = 0x1049,
    ATTR_VERSION = 0x104a,
    ATTR_PRIMARY_DEV_TYPE = 0x1054,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscAttributes_str(eWscAttributes enum_value) {
    switch (enum_value) {
    case ATTR_ASSOC_STATE:      return "ATTR_ASSOC_STATE";
    case ATTR_AUTH_TYPE:        return "ATTR_AUTH_TYPE";
    case ATTR_AUTH_TYPE_FLAGS:  return "ATTR_AUTH_TYPE_FLAGS";
    case ATTR_AUTHENTICATOR:    return "ATTR_AUTHENTICATOR";
    case ATTR_CONFIG_METHODS:   return "ATTR_CONFIG_METHODS";
    case ATTR_CONFIG_ERROR:     return "ATTR_CONFIG_ERROR";
    case ATTR_CONN_TYPE_FLAGS:  return "ATTR_CONN_TYPE_FLAGS";
    case ATTR_ENCR_TYPE:        return "ATTR_ENCR_TYPE";
    case ATTR_ENCR_TYPE_FLAGS:  return "ATTR_ENCR_TYPE_FLAGS";
    case ATTR_DEV_NAME:         return "ATTR_DEV_NAME";
    case ATTR_DEV_PASSWORD_ID:  return "ATTR_DEV_PASSWORD_ID";
    case ATTR_ENCR_SETTINGS:    return "ATTR_ENCR_SETTINGS";
    case ATTR_ENROLLEE_NONCE:   return "ATTR_ENROLLEE_NONCE";
    case ATTR_KEY_WRAP_AUTH:    return "ATTR_KEY_WRAP_AUTH";
    case ATTR_MAC_ADDR:         return "ATTR_MAC_ADDR";
    case ATTR_MANUFACTURER:     return "ATTR_MANUFACTURER";
    case ATTR_MSG_TYPE:         return "ATTR_MSG_TYPE";
    case ATTR_MODEL_NAME:       return "ATTR_MODEL_NAME";
    case ATTR_MODEL_NUMBER:     return "ATTR_MODEL_NUMBER";
    case ATTR_NETWORK_KEY:      return "ATTR_NETWORK_KEY";
    case ATTR_OS_VERSION:       return "ATTR_OS_VERSION";
    case ATTR_PUBLIC_KEY:       return "ATTR_PUBLIC_KEY";
    case ATTR_REGISTRAR_NONCE:  return "ATTR_REGISTRAR_NONCE";
    case ATTR_RF_BANDS:         return "ATTR_RF_BANDS";
    case ATTR_SERIAL_NUMBER:    return "ATTR_SERIAL_NUMBER";
    case ATTR_WSC_STATE:        return "ATTR_WSC_STATE";
    case ATTR_SSID:             return "ATTR_SSID";
    case ATTR_UUID_E:           return "ATTR_UUID_E";
    case ATTR_UUID_R:           return "ATTR_UUID_R";
    case ATTR_VENDOR_EXTENSION: return "ATTR_VENDOR_EXTENSION";
    case ATTR_VERSION:          return "ATTR_VERSION";
    case ATTR_PRIMARY_DEV_TYPE: return "ATTR_PRIMARY_DEV_TYPE";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscAttributes value) { return out << eWscAttributes_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: WSC

#endif //_TLVF/WSC_EWSCATTRIBUTES_H_
