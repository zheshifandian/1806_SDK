/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _TLVF_WSC_ATTR_LIST_H_
#define _TLVF_WSC_ATTR_LIST_H_

#include <memory>
#include <tlvf/ClassList.h>
#include <tlvf/WSC/WSC_Attributes.h>
#include <tlvf/ieee_1905_1/tlvWsc.h>
#include <tlvf/tlvflogging.h>

namespace WSC {

template <typename T, typename D> struct sAttrHeader {
    T type;
    T length;
    D *data() { return ((D *)this + sizeof(*this)); }
} __attribute__((packed));

template <typename TT, typename TD> class AttrList : public ClassList {
protected:
    AttrList(uint8_t *buff, size_t buff_len, bool parse) : ClassList(buff, buff_len, parse) {}

public:
    virtual ~AttrList() = default;

    template <class T> std::list<std::shared_ptr<T>> getAttrList() const
    {
        return this->getClassList<T>();
    };
    template <class T> std::shared_ptr<T> getAttr() const { return this->getClass<T>(); };
    template <class T> std::shared_ptr<T> addAttr() { return this->addClass<T>(); };
    bool finalize() { return ClassList::finalize(); };
    size_t len() const { return this->getMessageLength(); };
    uint8_t *buffer() { return this->getMessageBuff(); };
    virtual bool valid() const = 0;

protected:
    sAttrHeader<TT, TD> *getNextAttrHdr()
    {
        return reinterpret_cast<sAttrHeader<TT, TD> *>(
            m_class_vector.empty() ? getMessageBuff() : m_class_vector.back()->getBuffPtr());
    }
    uint16_t getNextAttrType()
    {
        auto type = getNextAttrHdr()->type;
        if (m_parse) {
            tlvf_swap((sizeof(type) * 8), reinterpret_cast<uint8_t *>(&type));
        }
        return static_cast<uint16_t>(type);
    };
    size_t getRemainingBytes()
    {
        return m_class_vector.empty() ? getMessageBuffLength()
                                      : m_class_vector.back()->getBuffRemainingBytes();
    };
};

} // namespace WSC

#endif // _TLVF_WSC_ATTR_LIST_H_
