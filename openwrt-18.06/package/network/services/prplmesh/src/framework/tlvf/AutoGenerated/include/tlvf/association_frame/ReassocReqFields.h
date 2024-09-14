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

#ifndef _TLVF_ASSOCIATION_FRAME_REASSOCREQFIELDS_H_
#define _TLVF_ASSOCIATION_FRAME_REASSOCREQFIELDS_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include <tuple>
#include "tlvf/common/sMacAddr.h"
#include "tlvf/association_frame/eElementID.h"
#include "tlvf/AssociationRequestFrame/assoc_frame_bitfields.h"

namespace assoc_frame {

class cFastBssTrans;
class cFmsRequest;
class cDmsRequest;

class cCurrentApAddress : public BaseClass
{
    public:
        cCurrentApAddress(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cCurrentApAddress(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cCurrentApAddress();

        sMacAddr& ap_addr();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        sMacAddr* m_ap_addr = nullptr;
};

class cFastBssTrans : public BaseClass
{
    public:
        cFastBssTrans(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cFastBssTrans(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cFastBssTrans();

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

class cFmsRequest : public BaseClass
{
    public:
        cFmsRequest(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cFmsRequest(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cFmsRequest();

        eElementID& type();
        const uint8_t& length();
        uint8_t& fms_token();
        size_t fms_request_subelem_length() { return m_fms_request_subelem_idx__ * sizeof(uint8_t); }
        uint8_t* fms_request_subelem(size_t idx = 0);
        bool set_fms_request_subelem(const void* buffer, size_t size);
        bool alloc_fms_request_subelem(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_fms_token = nullptr;
        uint8_t* m_fms_request_subelem = nullptr;
        size_t m_fms_request_subelem_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

class cDmsRequest : public BaseClass
{
    public:
        cDmsRequest(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cDmsRequest(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cDmsRequest();

        eElementID& type();
        const uint8_t& length();
        size_t dms_descrip_list_length() { return m_dms_descrip_list_idx__ * sizeof(uint8_t); }
        uint8_t* dms_descrip_list(size_t idx = 0);
        bool set_dms_descrip_list(const void* buffer, size_t size);
        bool alloc_dms_descrip_list(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_dms_descrip_list = nullptr;
        size_t m_dms_descrip_list_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

}; // close namespace: assoc_frame

#endif //_TLVF/ASSOCIATION_FRAME_REASSOCREQFIELDS_H_
