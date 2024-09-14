/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef AMBIORIX_H
#define AMBIORIX_H

#include "tlvf/common/sMacAddr.h"
#include <easylogging++.h>

namespace beerocks {
namespace nbapi {

/**
 * @class Ambiorix
 * @brief Interface for AmbiorixImpl and AmbiorixDummy classes
 */
class Ambiorix {
public:
    Ambiorix(){};
    Ambiorix(const Ambiorix &) = delete;
    Ambiorix &operator=(const Ambiorix &) = delete;
    virtual ~Ambiorix()                   = 0;

    /**
     * @brief Set the value to the object variable.
     *
     * @param relative_path Path to the object in datamodel (example: "Device.WiFi.DataElements.Network").
     * @param parameter The parameter to be set (example: "ID").
     * @param value Value which need to set.
     * @return True on success and false otherwise.
     */
    virtual bool set(const std::string &relative_path, const std::string &parameter,
                     const std::string &value) = 0;
    virtual bool set(const std::string &relative_path, const std::string &parameter,
                     const int32_t &value)     = 0;
    virtual bool set(const std::string &relative_path, const std::string &parameter,
                     const int64_t &value)     = 0;
    virtual bool set(const std::string &relative_path, const std::string &parameter,
                     const uint32_t &value)    = 0;
    virtual bool set(const std::string &relative_path, const std::string &parameter,
                     const uint64_t &value)    = 0;
    virtual bool set(const std::string &relative_path, const std::string &parameter,
                     const bool &value)        = 0;
    virtual bool set(const std::string &relative_path, const std::string &parameter,
                     const double &value)      = 0;
    virtual bool set(const std::string &relative_path, const std::string &parameter,
                     const sMacAddr &value)    = 0;

    /* @brief Add instance to the data model object with type list
     *
     * @param relative_path Path to the object with type list in datamodel (example: "Device.WiFi.DataElements.Network.Device").
     * @return Path to recently added object on success, empty string otherwise
     */
    virtual std::string add_instance(const std::string &relative_path) = 0;

    /**
     * @brief Remove instance from the data model object with type list
     *
     * @param relative_path Path to the object with type list in datamodel (example: "Device.WiFi.DataElements.Network.Device").
     * @param index Number of instance which should be remove.
     * @return True on success and false otherwise.
     */
    virtual bool remove_instance(const std::string &relative_path, uint32_t index) = 0;

    /**
     * @brief Get instance index by ID.
     *
     * @param specific_path Path to the object specific key (example: "Device.[ID == '%s'].").
     * @param key String which contains ID, for example it can be a MAC address.
     * @return Instance index on success and 0 otherwise.
     */
    virtual uint32_t get_instance_index(const std::string &specific_path,
                                        const std::string &key) = 0;

    /**
     * @brief Get Date and Time in the Ambiorix data model format (RFC3339): "2020-08-31T11:22:39Z".
     *
     * @return String with date and time in the Ambiorix data model format.
     */
    virtual std::string get_datamodel_time_format() = 0;

    /**
     * @brief Remove all instances from the data model object which name starts with given relative path
     *
     * @param relative_path Path to the object with type list in datamodel (example: "Device.WiFi.DataElements.Network.Device").
     * @return True on success and false otherwise.
     */
    virtual bool remove_all_instances(const std::string &relative_path) = 0;

    /**
     * @brief Instantiate optional sub-object.
     *
     * The subobject must be defined as a mib in the odl file. The name of the mib must be the same as
     * the name of the subobject, and it must contain only a single object definition.
     *
     * @param path_to_obj path to the object in datamodel (example: "Device.WiFi.DataElements.Network").
     * @param subobject_name name of optional subobject to instantiate (example: "HTCapabilities").
     * @return true if subobject successfully added, false otherwise
     */
    virtual bool add_optional_subobject(const std::string &path_to_obj,
                                        const std::string &subobject_name) = 0;

    /**
     * @brief Remove optional sub-object.
     *
     * The subobject must be defined as a mib in the odl file. The name of the mib must be the same as
     * the name of the subobject, and it must contain only a single object definition.
     *
     * @param path_to_obj path to the object in datamodel (example: "Device.WiFi.DataElements.Network").
     * @param subobject_name name of optional subobject to be removed (example: "HTCapabilities").
     * @return true if subobject successfully removed, false otherwise
     */
    virtual bool remove_optional_subobject(const std::string &path_to_obj,
                                           const std::string &subobject_name) = 0;
    /**
     * @brief Set current data and time in RFC 3339 format.
     *
     * @param path_to_object Path to NBAPI object which has parameter object.
     * @param param parameter name which is TimeStamp as default.
     * @return True if date and time successfully set, false otherwise.
     */
    virtual bool set_current_time(const std::string &path_to_object,
                                  const std::string &param = "TimeStamp") = 0;

    virtual bool read_param(const std::string &obj_path, const std::string &param_name,
                            uint64_t *param_val) = 0;
};

inline Ambiorix::~Ambiorix() {}

} // namespace nbapi
} // namespace beerocks

#endif
