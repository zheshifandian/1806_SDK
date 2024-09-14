/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef AMBIORIX_MOCK_H
#define AMBIORIX_MOCK_H

#include "ambiorix.h"
#include <gmock/gmock.h>

namespace beerocks {
namespace nbapi {

/**
 * @class Ambiorix
 * @brief Mock version of Ambiorix class
 */
class AmbiorixMock : public Ambiorix {
public:
    MOCK_METHOD(bool, set,
                (const std::string &relative_path, const std::string &parameter,
                 const std::string &value),
                (override));
    MOCK_METHOD(bool, set,
                (const std::string &relative_path, const std::string &parameter,
                 const int32_t &value),
                (override));
    MOCK_METHOD(bool, set,
                (const std::string &relative_path, const std::string &parameter,
                 const int64_t &value),
                (override));
    MOCK_METHOD(bool, set,
                (const std::string &relative_path, const std::string &parameter,
                 const uint32_t &value),
                (override));
    MOCK_METHOD(bool, set,
                (const std::string &relative_path, const std::string &parameter,
                 const uint64_t &value),
                (override));
    MOCK_METHOD(bool, set,
                (const std::string &relative_path, const std::string &parameter, const bool &value),
                (override));
    MOCK_METHOD(bool, set,
                (const std::string &relative_path, const std::string &parameter,
                 const double &value),
                (override));
    MOCK_METHOD(bool, set,
                (const std::string &relative_path, const std::string &parameter,
                 const sMacAddr &value),
                (override));
    MOCK_METHOD(std::string, add_instance, (const std::string &relative_path), (override));
    MOCK_METHOD(bool, remove_instance, (const std::string &relative_path, uint32_t index),
                (override));
    MOCK_METHOD(uint32_t, get_instance_index,
                (const std::string &specific_path, const std::string &key), (override));
    MOCK_METHOD(std::string, get_datamodel_time_format, (), (override));
    MOCK_METHOD(bool, remove_all_instances, (const std::string &relative_path), (override));
    MOCK_METHOD(bool, add_optional_subobject,
                (const std::string &path_to_obj, const std::string &subobject_name), (override));
    MOCK_METHOD(bool, remove_optional_subobject,
                (const std::string &path_to_obj, const std::string &subobject_name), (override));
    MOCK_METHOD(bool, set_current_time,
                (const std::string &path_to_object, const std::string &object), (override));
    MOCK_METHOD(bool, read_param,
                (const std::string &path_to_object, const std::string &object, uint64_t *param_val),
                (override));
};

} // namespace nbapi
} // namespace beerocks

#endif // AMBIORIX_MOCK_H
