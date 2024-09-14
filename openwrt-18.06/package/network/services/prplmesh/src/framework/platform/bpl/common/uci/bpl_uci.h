/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BPL_UCI_H_
#define _BPL_UCI_H_

#include <string>
#include <unordered_map>
#include <vector>

namespace beerocks {
namespace bpl {

using OptionsUnorderedMap = std::unordered_map<std::string, std::string>;
/****************************************************************************/
/******************************* Definitions ********************************/
/****************************************************************************/

/****************************************************************************/
/******************************* Structures *********************************/
/****************************************************************************/

/****************************************************************************/
/******************************** Functions *********************************/
/****************************************************************************/

/**
 * @brief Find if section exists.
 * 
 * @param[in] package_name name of the requested configuration file.
 * @param[in] section_type type of the requested section.
 * @param[in] section_name name of the requested section.
 * @return true if section exists in db, false otherwise
 */
bool uci_section_exists(const std::string &package_name, const std::string &section_type,
                        const std::string &section_name);

/**
 * @brief Finds a section with given type and containing option with name=value.
 *
 * The returned @a secion_name can be used as the @a secion_name in e.g. uci_set_section and
 * uci_get_section.
 *
 * @param[in] package_name name of the requested configuration file.
 * @param[in] section_type type of the requested section.
 * @param[in] option_name name of the requested option.
 * @param[in] option_value value of the requested option.
 * @param[out] section_name name of the section found or empty if it does not exist.
 * @return true on success, false otherwise.
 */
bool uci_find_section_by_option(const std::string &package_name, const std::string &section_type,
                                const std::string &option_name, const std::string &option_value,
                                std::string &section_name);

/**
 * @brief Add new named section.
 * 
 * @param[in] package_name name of the requested configuration file.
 * @param[in] section_type type of the requested section.
 * @param[in] section_name name of the requested section.
 * @param[in] commit_changes to show immediate intention to commit.
 * @return true if section exists in db, false otherwise
 */
bool uci_add_section(const std::string &package_name, const std::string &section_type,
                     const std::string &section_name, bool commit_changes);

/**
 * @brief Set values in section, updating as needed.
 * An option set to an empty string will be removed from the section.
 * An option that does not exist will be added to the section.
 * An option that already exists will be overridden with the new value
 * Options that exist and are not mentioned in @a options will remain unchanged.
 * 
 * @param[in] package_name name of the requested configuration file.
 * @param[in] section_type type of the requested section.
 * @param[in] section_name name of the requested section.
 * @param[in] options unordered map containing a key/value pair of parameters to be set.
 * @param[in] commit_changes to show immediate intention to commit.
 * @return true on success, false otherwise.
 */
bool uci_set_section(const std::string &package_name, const std::string &section_type,
                     const std::string &section_name, const OptionsUnorderedMap &options,
                     bool commit_changes);

/**
 * @brief Get values in section.
 * 
 * @param[in] package_name name of the requested configuration file.
 * @param[in] section_type type of the requested section.
 * @param[in] section_name name of the requested section.
 * @param[out] options empty unordered map, will be filled with configured parameters.
 * @return true on success, false otherwise.
 */
bool uci_get_section(const std::string &package_name, const std::string &section_type,
                     const std::string &section_name, OptionsUnorderedMap &options);

/**
 * @brief Get type of section by name.
 * 
 * @param[in] package_name name of the requested configuration file.
 * @param[in] section_name name of the requested section.
 * @param[out] section_type will contain type of found section.
 * @return true on success, false otherwise.
 */
bool uci_get_section_type(const std::string &package_name, const std::string &section_name,
                          std::string &section_type);

/**
 * @brief Get specific value from section.
 * 
 * @param[in] package_name name of the requested configuration file.
 * @param[in] section_type type of the requested section.
 * @param[in] section_name name of the requested section.
 * @param[in] option_name name of the requested option.
 * @param[out] option_value will contain value of found option. 
 * @return true on success, false otherwise.
 */
bool uci_get_option(const std::string &package_name, const std::string &section_type,
                    const std::string &section_name, const std::string &option_name,
                    std::string &option_value);

/**
 * @brief Delete existing section.
 * 
 * @param[in] package_name name of the requested configuration file.
 * @param[in] section_type type of the requested section.
 * @param[in] section_name name of the requested section.
 * @param[in] commit_changes to show immediate intention to commit.
 * @return true if section exists in db, false otherwise
 */
bool uci_delete_section(const std::string &package_name, const std::string &section_type,
                        const std::string &section_name, bool commit_changes);

/**
 * @brief Get all entries with the same type.
 * 
 * @param[in] package_name name of the requested configuration file.
 * @param[in] section_type type of the requested section. (If empty get sections of all types)
 * @param[out] sections empty vector, will be filled with all matching sections.
 * @return true if section exists in db, false otherwise
 */
bool uci_get_all_sections(const std::string &package_name, const std::string &section_type,
                          std::vector<std::string> &sections);

/**
 * @brief Commit changes by package name
 * 
 * @param[in] package_name name of the requested package.
 * @return true on success, false otherwise.
 */
bool uci_commit_changes(const std::string &package_name);

} // namespace bpl
} // namespace beerocks

#endif /* _BPL_UCI_H_ */
