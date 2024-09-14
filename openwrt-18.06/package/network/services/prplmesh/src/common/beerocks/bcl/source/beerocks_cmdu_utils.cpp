/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_cmdu_utils.h>

#include <bcl/beerocks_message_structs.h>

#include <tlvf/CmduMessage.h>
#include <tlvf/ieee_1905_1/tlvVendorSpecific.h>

#include <easylogging++.h>

namespace beerocks {

bool CmduUtils::verify_cmdu(uint8_t *data, size_t length)
{
    if (length < ieee1905_1::cCmduHeader::get_initial_size() + sizeof(ieee1905_1::sTlvHeader)) {
        LOG(ERROR) << "Invalid CMDU length: " << length;
        return false;
    }

    auto tlv = reinterpret_cast<ieee1905_1::sTlvHeader *>(
        data + ieee1905_1::cCmduHeader::get_initial_size());

    do {

        auto tlv_type = static_cast<ieee1905_1::eTlvType>(tlv->type);

        uint16_t tlv_length = tlv->length;
        swap_16(tlv_length);

        if (tlv_type == ieee1905_1::eTlvType::TLV_VENDOR_SPECIFIC) {
            auto tlv_vendor_specific =
                ieee1905_1::tlvVendorSpecific(reinterpret_cast<uint8_t *>(tlv),
                                              sizeof(ieee1905_1::sTlvHeader) + tlv_length, true);
            if (!tlv_vendor_specific.isInitialized()) {
                LOG(ERROR) << "tlvVendorSpecific init() failure";
                break;
            }

            if (tlv_vendor_specific.vendor_oui() ==
                ieee1905_1::tlvVendorSpecific::eVendorOUI::OUI_INTEL) {
                // assuming that the magic is the first data on the beerocks header
                auto beerocks_magic =
                    *(reinterpret_cast<uint32_t *>(tlv_vendor_specific.payload()));
                swap_32(beerocks_magic);
                if (beerocks_magic != message::MESSAGE_MAGIC) {
                    LOG(WARNING) << "mismatch magic " << std::hex << int(beerocks_magic)
                                 << " != " << int(message::MESSAGE_MAGIC) << std::dec;
                    break;
                }

            } else {
                LOG(INFO) << "Not an Intel vendor specific message!";
            }

            // cancel the swap we did
            tlv_vendor_specific.class_swap();
        } else if ((tlv_type == ieee1905_1::eTlvType::TLV_END_OF_MESSAGE) && (0 == tlv_length)) {
            return true;
        }

        // set the next tlv
        tlv = reinterpret_cast<ieee1905_1::sTlvHeader *>(
            reinterpret_cast<uint8_t *>(tlv) + sizeof(ieee1905_1::sTlvHeader) + tlv_length);

    } while (reinterpret_cast<uint8_t *>(tlv) < (data + length));

    LOG(ERROR) << "TLV end of message not found! tlv_type=" << tlv->type
               << ", tlv_length=" << tlv->length;

    return false;
}

} // namespace beerocks
