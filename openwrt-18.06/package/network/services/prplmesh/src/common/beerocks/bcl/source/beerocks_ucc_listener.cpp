/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

/*
 * In order to to send WFA-CA command, send message with netcat a specific on port which is set on
 * beerocks_agent.conf or beerocks_controller.conf under the field "ucc_listener_port".
 * Example:
 *     echo -n <command> | nc <bridge_ip_addr> <port>
 */

#include <bcl/beerocks_ucc_listener.h>

#include <bcl/beerocks_string_utils.h>
#include <bcl/network/network_utils.h>

#include <algorithm>
#include <easylogging++.h>
#include <iomanip>

#define SELECT_TIMEOUT_MSC 5000
#define FORBIDDEN_CHARS "$#&*()[];/\\<>?|`~=+"

using namespace beerocks;

static uint16_t g_mid = UINT16_MAX / 2 + 1;

/* Common Error Messages */
static const std::string err_internal = "Internal error";

//////////////////////////////////////////////////////////////////////////////
////////////////////////////// Helper functions //////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief Checks for forbidden characters inside WFA-CA command.
 * 
 * @param[in] str WFA-CA command.
 * @return String containing all the forbidden characters the have been found.
 */
std::string beerocks_ucc_listener::check_forbidden_chars(const std::string &str)
{
    std::string forbidden_chars_found;
    auto found = str.find_first_of(FORBIDDEN_CHARS);
    while (found != std::string::npos) {
        forbidden_chars_found += str[found];
        found = str.find_first_of(FORBIDDEN_CHARS, found + 1);
    }
    return forbidden_chars_found;
}

/**
 * @brief Checks that the given string is in a hex notation.
 * 
 * @param[in] str String to check hex notation.
 * @param[in] expected_octets Number of expected octets (Optional).
 * @return true if string is valid hex notation, else false.
 */
bool beerocks_ucc_listener::validate_hex_notation(const std::string &str, uint8_t expected_octets)
{
    // Each value must start with 0x
    if (str.substr(0, 2) != "0x") {
        return false;
    }

    // The number of digits must be even
    // In practice we already encountered HEX values with odd number of digits.

    // So instead of failing, adjust the number of octets calculation
    // "str.size() - 2" ignore "0x" prefix.
    // division by 2 because 2 chars are equal to one octet
    uint8_t number_of_octets = (str.size() - 2) / 2;
    // The calculation will round down the number of octets on odd size
    // so add the missing octet if needed.
    if (str.size() % 2 != 0) {
        ++number_of_octets;
    }

    // Expected octets validation
    if (expected_octets != 0 && (number_of_octets != expected_octets)) {
        return false;
    }

    // Check if string has non-hex character
    if (str.find_first_not_of("0123456789abcdefABCDEF", 2) != std::string::npos) {
        return false;
    }

    return true;
}

/**
 * @brief Checks that the given string is in binary notation.
 * 
 * @param[in] str String to check binary notation.
 * @return true if string is in valid binary notation, else false.
 */
bool beerocks_ucc_listener::validate_binary_notation(const std::string &str)
{
    // Each value must start with 0b
    if (str.substr(0, 2) != "0b") {
        return false;
    }

    if ((str.size() - 2) % 8 != 0) {
        return false;
    }

    // Check if string contains not only 0 and 1
    if (str.find_first_not_of("01", 2) != std::string::npos) {
        return false;
    }
    return true;
}

/**
 * @brief Checks that the given string is in decimal notation.
 * 
 * @param[in] str String to check decimal notation.
 * @return true if string is in valid decimal notation, else false.
 */
bool beerocks_ucc_listener::validate_decimal_notation(const std::string &str)
{
    // Each value must start with 0b
    if (str.substr(0, 2) != "0d") {
        return false;
    }

    // Check if string has non-decimal character
    if (str.find_first_not_of("0123456789", 2) != std::string::npos) {
        return false;
    }
    return true;
}

/**
 * @brief Convert enum of WFA-CA status to string.
 * 
 * @param[in] status Enum of WFA-CA status. 
 * @return Status as string.
 */
const std::string beerocks_ucc_listener::wfa_ca_status_to_string(eWfaCaStatus status)
{
    switch (status) {
    case eWfaCaStatus::RUNNING:
        return "RUNNING";
    case eWfaCaStatus::INVALID:
        return "INVALID";
    case eWfaCaStatus::ERROR:
        return "ERROR";
    case eWfaCaStatus::COMPLETE:
        return "COMPLETE";
    default:
        LOG(ERROR) << "Status argument doesn't have an enum";
        break;
    }
    return std::string();
}

/**
 * @brief Convert string of WFA-CA command into enum
 * 
 * @param[in] command String of WFA-CA command.
 * @return Command enum. If command is not recognized returns eWfaCaCommand::WFA_CA_COMMAND_MAX.
 */
beerocks_ucc_listener::eWfaCaCommand
beerocks_ucc_listener::wfa_ca_command_from_string(std::string command)
{
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);

    if (command == "CA_GET_VERSION") {
        return eWfaCaCommand::CA_GET_VERSION;
    } else if (command == "DEVICE_GET_INFO") {
        return eWfaCaCommand::DEVICE_GET_INFO;
    } else if (command == "DEV_RESET_DEFAULT") {
        return eWfaCaCommand::DEV_RESET_DEFAULT;
    } else if (command == "DEV_SEND_1905") {
        return eWfaCaCommand::DEV_SEND_1905;
    } else if (command == "DEV_SET_CONFIG") {
        return eWfaCaCommand::DEV_SET_CONFIG;
    } else if (command == "DEV_GET_PARAMETER") {
        return eWfaCaCommand::DEV_GET_PARAMETER;
    } else if (command == "START_WPS_REGISTRATION") {
        return eWfaCaCommand::START_WPS_REGISTRATION;
    } else if (command == "DEV_SET_RFEATURE") {
        return eWfaCaCommand::DEV_SET_RFEATURE;
    } else if (command == "CUSTOM_CMD") {
        return eWfaCaCommand::CUSTOM_CMD;
    }

    return eWfaCaCommand::WFA_CA_COMMAND_MAX;
}

/**
 * @brief Parse the strings vector which containing parameter names, and values into unordered map
 * which will contain the parameter name as a map key, and paramaeter value as map value.
 * The function receives as an input unordered map of parameters that are mandatory, which will
 * include non-mandatory parameters as output.
 * 
 * @param[in] command_tokens A vector containing commands parameters name and values.
 * @param[in,out] params An unordered map containing mandatory parameters on input, and parsed 
 * command as output.
 * @param[out] err_string Contains an error description if the function fails.
 * @return true if successful, false if not.
 */
bool beerocks_ucc_listener::parse_params(const std::vector<std::string> &command_tokens,
                                         std::unordered_map<std::string, std::string> &params,
                                         std::string &err_string)
{
    // create a copy of all mandatory param names
    std::list<std::string> mandatory_params;
    for (const auto &param_name : params) {
        mandatory_params.push_back(param_name.first);
    }

    size_t mandatory_params_cnt = 0;
    for (auto it = std::next(command_tokens.begin()); it != command_tokens.end(); it++) {
        std::string param_name = *it;
        std::transform(param_name.begin(), param_name.end(), param_name.begin(), ::tolower);
        if (params.find(param_name) != params.end()) {
            mandatory_params_cnt++;
        }

        if (std::next(it) == command_tokens.end()) {
            err_string = "missing param value for param '" + param_name + "'";
            return false;
        }

        params[param_name] = *std::next(it);
        it++;
    }

    if (mandatory_params_cnt == mandatory_params.size()) {
        return true;
    }

    // Generate an err string if missing mandatory param
    err_string = "missing mandatory params: ";
    for (const std::string &param_name : mandatory_params) {
        if ((params.find(param_name))->second.empty()) {
            err_string += param_name + ", ";
        }
    }

    // Remove last comma and space
    err_string.pop_back();
    err_string.pop_back();

    return false;
}

/**
 * @brief Parse command "DEV_SEND_1905" TLVs parameters into an ordered list of TLV structure
 * "tlv_hex_t", and does data validation.
 * 
 * @param[in,out] tlv_hex_list An ordered list of TLVs of type "tlv_hex_t".
 * @param[in] params An unordered map containing parsed parameters of a CAPI command. 
 * @param[out] err_string Contains an error description if the function fails.
 * @return true if successful, false if not.
 */
bool beerocks_ucc_listener::get_send_1905_1_tlv_hex_list(
    std::list<tlv_hex_t> &tlv_hex_list, const std::unordered_map<std::string, std::string> &params,
    std::string &err_string)
{
    static const int max_tlv                          = 256; // Magic number
    size_t tlv_idx                                    = 0;
    bool skip                                         = false;
    bool finish                                       = false;
    static const std::vector<std::string> tlv_members = {"tlv_type", "tlv_length", "tlv_value"};

    do {
        tlv_hex_t tlv_hex;
        for (size_t tlv_member_idx = 0; tlv_member_idx < tlv_members.size(); tlv_member_idx++) {
            std::string lookup_str =
                tlv_members[tlv_member_idx] + (tlv_idx > 0 ? std::to_string(tlv_idx) : "");
            auto it = params.find(lookup_str);
            if (it == params.end()) {
                if (tlv_idx == 0 && tlv_member_idx == 0) {
                    // Skipping TLV with no index (index 0), checking TLV index 1
                    skip = true;
                    break;
                } else if (tlv_idx > 0 && tlv_member_idx == 0) {
                    // No indexed TLVs found, finish
                    finish = true;
                    break;
                } else {
                    err_string = "missing param name '" + lookup_str + "'";
                    return false;
                }
            }

            // Type
            if (tlv_member_idx == 0) {
                tlv_hex.type = it->second;
                if (!validate_hex_notation(tlv_hex.type, 1)) {
                    err_string = "param name '" + lookup_str + "' has invalid hex notation value";
                    return false;
                }

                // Length
            } else if (tlv_member_idx == 1) {
                tlv_hex.length = it->second;
                if (!validate_hex_notation(tlv_hex.length, 2)) {
                    err_string = "param name '" + lookup_str + "' has invalid hex notation value";
                    return false;
                }

                // Value
            } else if (tlv_member_idx == 2) {
                // The hex values contains '{' and '}' delimiters, but we don't care about them.
                std::string param_value = it->second;
                param_value.erase(std::remove(param_value.begin(), param_value.end(), '{'),
                                  param_value.end());
                param_value.erase(std::remove(param_value.begin(), param_value.end(), '}'),
                                  param_value.end());

                std::string tmp_value;
                auto values = string_utils::str_split(param_value, ' ');
                for (const auto &value : values) {
                    // Validate hex notation on list of values separated by space
                    if ((validate_hex_notation(value) || net::network_utils::is_valid_mac(value) ||
                         validate_binary_notation(value) || validate_decimal_notation(value))) {
                        // Hex notation bytes are use as is
                        tmp_value += value + " ";
                    } else {
                        // If we got here, assuming we have received a string. Convert the string into ASCII hex values.
                        std::stringstream ss;
                        for (char c : value) {
                            ss << "0x" << std::hex << int(c) << " ";
                        }
                        tmp_value += ss.str();
                    }
                }
                tlv_hex.value = tmp_value;
            } else {
                LOG(ERROR) << "Illegal tlv_member_idx value: " << int(tlv_member_idx);
                err_string = err_internal;
                return false;
            }
        }

        if (finish) {
            return true;
        }

        tlv_idx++;

        if (skip) {
            skip = false;
            continue;
        }

        tlv_hex_list.push_back(tlv_hex);

    } while (tlv_idx < max_tlv);

    LOG(ERROR) << "Reached stop condition";
    err_string = err_internal;
    return false;
}

//////////////////////////////////////////////////////////////////////////////
////////////////////////////// Class functions ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

beerocks_ucc_listener::beerocks_ucc_listener(ieee1905_1::CmduMessageTx &cmdu,
                                             std::unique_ptr<beerocks::UccServer> ucc_server)
    : m_cmdu_tx(cmdu), m_ucc_server(std::move(ucc_server))
{
    // UCC server is provided in certification mode only
    if (m_ucc_server) {
        m_ucc_server->set_command_received_handler(
            [&](int fd, const std::string &command) { handle_wfa_ca_command(fd, command); });
    }
}

beerocks_ucc_listener::~beerocks_ucc_listener()
{
    if (m_ucc_server) {
        m_ucc_server->clear_command_received_handler();
    }
}

bool beerocks_ucc_listener::send_reply(int fd, const std::string &err_string)
{
    return reply_ucc(fd, err_string.empty() ? eWfaCaStatus::COMPLETE : eWfaCaStatus::ERROR,
                     err_string);
}

/**
 * @brief Generate WFA-CA reply message and send it.
 * 
 * @param[in] fd File descriptor of the client socket connection to send the reply through.
 * @param[in] status Status of the reply.
 * @param[in] description Optional, Description to add to the reply.
 * @return true if successful, false if not.
 */
bool beerocks_ucc_listener::reply_ucc(int fd, eWfaCaStatus status, const std::string &description)
{
    std::string reply_string = "status," + wfa_ca_status_to_string(status);

    if (!description.empty()) {
        reply_string += ",";
        if (status == eWfaCaStatus::INVALID || status == eWfaCaStatus::ERROR) {
            reply_string += "errorCode,";
        }
        reply_string += description;
    }

    LOG(DEBUG) << "Replying message: '" << reply_string << "'";

    // Send reply to UCC
    if (!m_ucc_server->send_reply(fd, reply_string)) {
        LOG(ERROR) << "Failed to send reply string!";
        return false;
    }

    return true;
}

/**
 * @brief Receives WFA-CA command, executes it and returns a reply to the sender.
 * 
 * @param[in] fd File descriptor of the client socket connection the command was received through.
 * @param[in] command Command string.
 */
void beerocks_ucc_listener::handle_wfa_ca_command(int fd, const std::string &command)
{
    LOG(DEBUG) << "Received WFA-CA, command=\"" << command << "\"";

    // send back first reply
    if (!reply_ucc(fd, eWfaCaStatus::RUNNING)) {
        LOG(ERROR) << "failed to send reply";
        return;
    }

    std::string err_string;

    auto forbidden_chars = check_forbidden_chars(command);
    if (!forbidden_chars.empty()) {
        err_string = "command contain illegal chars";
        LOG(ERROR) << err_string << ": " << forbidden_chars;
        reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
        return;
    }

    auto cmd_tokens_vec = string_utils::str_split(command, ',');

    if (cmd_tokens_vec.empty()) {
        err_string = "empty command";
        LOG(ERROR) << err_string;
        reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
        return;
    }

    const auto &command_type_str = *cmd_tokens_vec.begin();
    auto command_type            = wfa_ca_command_from_string(command_type_str);

    switch (command_type) {
    case eWfaCaCommand::CA_GET_VERSION: {
        // Send back second reply
        reply_ucc(fd, eWfaCaStatus::COMPLETE, std::string("version,") + BEEROCKS_VERSION);
        break;
    }
    case eWfaCaCommand::DEVICE_GET_INFO: {
        // Send back second reply
        reply_ucc(fd, eWfaCaStatus::COMPLETE, fill_version_reply_string());
        break;
    }
    case eWfaCaCommand::DEV_GET_PARAMETER: {
        std::unordered_map<std::string, std::string> params{{"parameter", ""}};
        if (!parse_params(cmd_tokens_vec, params, err_string)) {
            LOG(ERROR) << err_string;
            reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
            break;
        }
        std::string value;
        if (!handle_dev_get_param(params, value)) {
            LOG(ERROR) << "failed to get parameter " << params["parameter"] << "error: " << value;
            reply_ucc(fd, eWfaCaStatus::ERROR, value);
            break;
        }
        // Success
        reply_ucc(fd, eWfaCaStatus::COMPLETE, params["parameter"] + "," + value);
        break;
    }
    case eWfaCaCommand::START_WPS_REGISTRATION: {
        std::unordered_map<std::string, std::string> params{{"band", std::string()},
                                                            {"wpsconfigmethod", std::string()}};
        if (!parse_params(cmd_tokens_vec, params, err_string)) {
            LOG(ERROR) << err_string;
            reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
            break;
        }
        if (params["wpsconfigmethod"] != "PBC") {
            err_string = "invalid WpsConfigMethod, supporting only PBC";
            LOG(ERROR) << err_string;
            reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
            break;
        }
        if (!handle_start_wps_registration(params["band"], err_string)) {
            LOG(ERROR) << err_string;
            reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
            break;
        }
        // Send back second reply
        reply_ucc(fd, eWfaCaStatus::COMPLETE);
        break;
    }
    case eWfaCaCommand::DEV_RESET_DEFAULT: {
        // NOTE: Note sure this parameters are actually needed. There is a controversial
        // between TestPlan and CAPI specification regarding if these params are required.
        std::unordered_map<std::string, std::string> params{
            // {"name", std::string()},
            // {"program", std::string()},
            // {"devrole", std::string()},
            // {"type", std::string()}
        };

        if (!parse_params(cmd_tokens_vec, params, err_string)) {
            LOG(ERROR) << err_string;
            reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
            break;
        }

        // Input check
        if (params.find("devrole") != params.end()) {
            if (params["devrole"] != "controller" && params["devrole"] != "agent") {
                err_string = "invalid param value '" + params["devrole"] +
                             "' for param name 'devrole', accepted values can be only 'controller'"
                             " or 'agent";
                LOG(ERROR) << err_string;
                reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
                break;
            } else if (m_ucc_listener_run_on == eUccListenerRunOn::CONTROLLER &&
                       params["devrole"] != "controller") {
                err_string = "The Controller received a command that it no destined for it";
                LOG(ERROR) << err_string;
                reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
                break;
            } else if (m_ucc_listener_run_on == eUccListenerRunOn::AGENT &&
                       params["devrole"] != "agent") {
                err_string = "The Agent received a command that it no destined for it";
                LOG(ERROR) << err_string;
                reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
                break;
            }
        }

        if (params.find("program") != params.end()) {
            if (!validate_program_parameter(params["program"], err_string)) {
                // errors are logged by validate_program_parameter
                reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
                break;
            }
        }

        if (params.find("type") != params.end()) {
            if (params["type"] != "test bed" && params["type"] != "DUT") {
                err_string = "invalid param value '" + params["type"] +
                             "' for param name 'type', accepted values can be only 'test bed' or"
                             " 'dut'";
                LOG(ERROR) << err_string;
                reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
                break;
            }
        }

        // TODO: Find out what to do with value of param "name".

        // This command is processed asynchronously so second reply when processing is complete is
        // not sent here.
        // The file descriptor is passed as parameter to allow sending that reply later on.
        if (!handle_dev_reset_default(fd, params, err_string)) {
            reply_ucc(fd, eWfaCaStatus::ERROR, err_string);
        }

        break;
    }
    case eWfaCaCommand::DEV_SEND_1905: {
        std::unordered_map<std::string, std::string> params{{"destalid", std::string()},
                                                            {"messagetypevalue", std::string()}};

        if (!parse_params(cmd_tokens_vec, params, err_string)) {
            LOG(ERROR) << err_string;
            reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
            break;
        }

        // Input check
        auto &dest_alid = params["destalid"];
        std::transform(dest_alid.begin(), dest_alid.end(), dest_alid.begin(), ::tolower);

        auto &message_type_str = params["messagetypevalue"];
        auto message_type      = (uint16_t)(std::strtoul(message_type_str.c_str(), nullptr, 16));
        if (!ieee1905_1::eMessageTypeValidate::check(message_type)) {
            err_string = "invalid param value '" + message_type_str +
                         "' for param name 'MessageTypeValue', message type not found";
            LOG(ERROR) << err_string;
            reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
            break;
        }

        // TLV check
        std::list<tlv_hex_t> tlv_hex_list;
        if (!get_send_1905_1_tlv_hex_list(tlv_hex_list, params, err_string)) {
            LOG(ERROR) << err_string;
            reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
            break;
        }

        // Check if CMDU message type has prepared CMDU which can be loaded
        if (tlv_hex_list.empty() && m_cmdu_tx.is_finalized() &&
            m_cmdu_tx.getMessageType() == ieee1905_1::eMessageType(message_type)) {
            m_cmdu_tx.setMessageId(g_mid); // force mid
            LOG(DEBUG) << "Send preset cmdu with mid " << std::hex << g_mid;
            if (!send_cmdu_to_destination(m_cmdu_tx, dest_alid)) {
                LOG(ERROR) << "Failed to send CMDU";
                reply_ucc(fd, eWfaCaStatus::ERROR, err_internal);
                m_cmdu_tx.reset();
                break;
            }
            m_cmdu_tx.reset();
        } else {
            // CMDU was not loaded from prepared buffer, and need to be created manually, and
            // use TLV data from the command (if exists)
            m_cmdu_tx.reset();
            if (!m_cmdu_tx.create(g_mid, ieee1905_1::eMessageType(message_type))) {
                LOG(ERROR) << "cmdu creation of type 0x" << message_type_str << ", has failed";
                reply_ucc(fd, eWfaCaStatus::ERROR, err_internal);
                break;
            }

            // We need to fill in CmduMessage's class vector. However, we really don't care about
            // the classes, and we don't want any swapping to be done. Still, we need something
            // to make sure the length of the tlvs is taken into account.
            // Therefore, use the tlvPrefilledData class, which can update the end pointer.
            auto tlvs = m_cmdu_tx.addClass<tlvPrefilledData>();
            if (!tlvs) {
                LOG(ERROR) << "addClass tlvPrefilledData has failed";
                reply_ucc(fd, eWfaCaStatus::ERROR, err_internal);
                break;
            }

            if (!tlvs->add_tlvs_from_list(tlv_hex_list, err_string)) {
                LOG(ERROR) << err_string;
                reply_ucc(fd, eWfaCaStatus::ERROR, err_string);
                break;
            }

            LOG(INFO) << "Send TLV CMDU " << message_type << " with " << tlv_hex_list.size()
                      << " TLVs, length " << m_cmdu_tx.getMessageLength();

            if (!send_cmdu_to_destination(m_cmdu_tx, dest_alid)) {
                LOG(ERROR) << "Failed to send CMDU";
                reply_ucc(fd, eWfaCaStatus::ERROR, err_internal);
                break;
            }
        }

        std::string description = "mid,0x" + string_utils::int_to_hex_string(g_mid++, 4);

        // Send back second reply
        reply_ucc(fd, eWfaCaStatus::COMPLETE, description);
        break;
    }
    case eWfaCaCommand::DEV_SET_CONFIG: {
        // In general we expect each command to be with a format of key1,value1,key2,value2....
        // Since this is the assumption we parse the command into an unordered map of strings.
        // The R2 CAPI commands presented a new dev_set_config command with the format:
        //  dev_set_config,program,map,bss_info1,00:50:43:23:f6:ec 8x Multi-AP-24G-1 0x0020 0x0008 maprocks1 0 1 ,
        //  tlv_type1,0xB5,tlv_length1,0x0003,tlv_value1,{0x000A 0x00},tlv_type2,0xB6,tlv_length2,0x0012,tlv_value2,
        //  {0x01 {0x0E  Multi-AP-24G-1  0x000A}},
        //  bss_info2,00:50:43:23:f6:ec 8x Multi-AP-24G-3 0x0020 0x0008 maprocks3 0 1 ,
        //  tlv_type1,0xB5,tlv_length1,0x0003,tlv_value1,{0x000A 0x00},
        //  tlv_type2,0xB6,tlv_length2,0x0012,tlv_value2,{0x01 {0x0E  Multi-AP-24G-3  0x0014}}
        //
        // The TLV values are possibly added to each of the bss_infos. Since the TLV value have same "keys" -
        // tlv_type1, tlv_value1 - in all the bss_infos we had to break the parsing to handle each bss_info separately
        // to avoid overriding in the unordered_map.

        std::vector<std::string> common_tokens;
        // Each element on the outer vector contains a vector of tokens belongs to a specific bss
        // configuration.
        std::vector<std::vector<std::string>> bss_infos;
        static const std::string bss_info_prefix("bss_info");
        for (const auto &token : cmd_tokens_vec) {
            // Check if token starts with "bss_info", initialize the first element in the
            // outer vector with the common tokens.
            if (token.compare(0, bss_info_prefix.size(), bss_info_prefix) == 0) {
                // Push common tokens
                bss_infos.push_back(common_tokens);
                bss_infos.back().push_back(bss_info_prefix);
                continue;
            }
            // If "bss_info" string hasn't been found yet, push the token to the common tokens
            // vector.
            if (bss_infos.empty()) {
                common_tokens.push_back(token);
                continue;
            }
            // Push the token to the last bss_info vector that was added.
            bss_infos.back().push_back(token);
        }
        // If it is an agent command, the bss_infos vector will be empty, so push the the common
        // tokens as the first and only element.
        if (bss_infos.empty()) {
            // Push common tokens
            bss_infos.push_back(common_tokens);
        }

        // Input check
        // NOTE: Not sure these parameters are actually needed. There is a controversial
        // between TestPlan and CAPI specification regarding if these params are required.
        auto common_it = std::find(common_tokens.begin(), common_tokens.end(), "program");
        if (common_it != common_tokens.end()) {
            if (!validate_program_parameter(*std::next(common_it), err_string)) {
                LOG(ERROR) << err_string;
                reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
                break;
            }
        }

        bool first = true;
        for (const auto &bss_info_vector : bss_infos) {
            std::unordered_map<std::string, std::string> params;

            if (!parse_params(bss_info_vector, params, err_string)) {
                break;
            }
            // Add signal that this is the first element to handle. Originally all the bss_infos
            // were handled in one shot - all at once - so it was easy to know when it is safe to
            // clean previous configurations.
            // Since we broke the bss_info configuration to one at a time - we need to mark that we are starting
            // to handle a list of bss_infos and that it is safe to clear the previous configurations.
            if (first) {
                params["first_bss"] = std::string();
                first               = false;
            }
            if (!handle_dev_set_config(params, err_string)) {
                break;
            }
        }
        if (!err_string.empty()) {
            LOG(ERROR) << err_string;
            reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
            break;
        }

        // Send back second reply
        reply_ucc(fd, eWfaCaStatus::COMPLETE);
        break;
    }
    case eWfaCaCommand::DEV_SET_RFEATURE: {
        std::unordered_map<std::string, std::string> params{
            {"name", std::string()}, {"type", std::string()}, {"assoc_disallow", std::string()}};

        if (!parse_params(cmd_tokens_vec, params, err_string)) {
            LOG(ERROR) << err_string;
            reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
            break;
        }

        if (!handle_dev_set_rfeature(params, err_string)) {
            LOG(ERROR) << err_string;
            reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
            break;
        }

        // Send back second reply
        reply_ucc(fd, eWfaCaStatus::COMPLETE);
        break;
    }
    case eWfaCaCommand::CUSTOM_CMD: {
        std::unordered_map<std::string, std::string> params{{"cmd", std::string()}};

        if (!parse_params(cmd_tokens_vec, params, err_string)) {
            LOG(ERROR) << err_string;
            reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
            break;
        }

        if (!handle_custom_command(params, err_string)) {
            LOG(ERROR) << err_string;
            reply_ucc(fd, eWfaCaStatus::INVALID, err_string);
            break;
        }

        // Send back second reply
        reply_ucc(fd, eWfaCaStatus::COMPLETE);
        break;
    }
    default: {
        auto err_description = "Invalid WFA-CA command type: '" + command_type_str + "'";
        LOG(ERROR) << err_description;
        reply_ucc(fd, eWfaCaStatus::INVALID, err_description);
        break;
    }
    }
}

bool beerocks_ucc_listener::validate_program_parameter(const std::string &parameter,
                                                       std::string &err_string)
{
    if (std::find(supported_programs.begin(), supported_programs.end(), parameter) ==
        supported_programs.end()) {
        err_string = "invalid param value '" + parameter +
                     "' for param name 'program', accepted values are: ";
        for (const auto &p : supported_programs) {
            err_string += " '" + std::string(p) + "' ";
        }
        LOG(ERROR) << err_string;
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////
////////////////////// tlvPrefilledData Class functions //////////////////////
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief Writes hex string into the class buffer.
 * 
 * @param[in] value string in hex notation.
 * @param[out] length Length of current tlv.
 * @return false if length smaller than data, else true.
 */
bool tlvPrefilledData::add_tlv_value_hex_string(const std::string &value, uint16_t &length)
{
    // iterate every two chars (1 octet) and write it to buffer
    // start with char_idx = 2 to skip "0x"
    for (size_t char_idx = 2; char_idx < value.size(); char_idx += 2) {
        if (length == 0) {
            return false;
        }
        *m_buff_ptr__ = std::strtoul(value.substr(char_idx, 2).c_str(), nullptr, 16);
        m_buff_ptr__++;
        length--;
    }
    return true;
}

/**
 * @brief Writes a decimal string into the class buffer.
 * 
 * @param[in] value Decimal string.
 * @param[out] length Length of current tlv.
 * @return false if length smaller than data, else true.
 */
bool tlvPrefilledData::add_tlv_value_decimal_string(const std::string &value, uint16_t &length)
{
    // Convert to hex string
    std::stringstream ss;
    size_t num_of_bytes = 1;
    // We don't really know how to identify if it is one or two (or more) bytes.
    // Assume 2 bytes if it has more than 3 digits
    if (value.size() > 5) {
        num_of_bytes++;
    }
    ss << "0x" << std::setw(num_of_bytes * 2) << std::setfill('0') << std::hex
       << string_utils::stoi(value.substr(2, value.length()));

    return add_tlv_value_hex_string(ss.str(), length);
}

/**
 * @brief Writes a binary string into the class buffer.
 * 
 * @param[in] value Binary string.
 * @param[out] length Length of current tlv.
 * @return false if length smaller than data, else true.
 */
bool tlvPrefilledData::add_tlv_value_binary_string(const std::string &value, uint16_t &length)
{
    for (size_t char_idx = 2; char_idx < value.size(); char_idx += 8) {
        if (length == 0) {
            return false;
        }
        *m_buff_ptr__ = std::strtoul(value.substr(char_idx, 8).c_str(), nullptr, 2);
        m_buff_ptr__++;
        length--;
    }
    return true;
}

/**
 * @brief Writes a mac string into the class buffer.
 * 
 * @param[in] value mac address.
 * @param[out] length of current tlv.
 * @return false if length smaller than data, else true.
 */
bool tlvPrefilledData::add_tlv_value_mac(const std::string &value, uint16_t &length)
{
    for (size_t char_idx = 0; char_idx < value.size(); char_idx += 3) {
        if (length == 0) {
            return false;
        }
        *m_buff_ptr__ = std::strtoul(value.substr(char_idx, 2).c_str(), nullptr, 16);
        m_buff_ptr__++;
        length--;
    }
    return true;
}

bool tlvPrefilledData::add_tlv_from_strings(const beerocks_ucc_listener::tlv_hex_t &tlv,
                                            std::string &err_string)
{
    uint8_t type = std::strtoul(tlv.type.c_str(), nullptr, 16);

    uint16_t length = std::strtoul(tlv.length.c_str(), nullptr, 16);

    // "+3" = size of type and length fields
    if (getBuffRemainingBytes() < unsigned(length + 3)) {
        err_string = "not enough space on buffer";
        return false;
    }

    *m_buff_ptr__ = type;
    m_buff_ptr__++;
    *m_buff_ptr__ = uint8_t(length >> 8);
    m_buff_ptr__++;
    *m_buff_ptr__ = uint8_t(length);
    m_buff_ptr__++;

    auto values = string_utils::str_split(tlv.value, ' ');
    for (const auto &value : values) {
        //Do conversion if needed
        if (value[1] == 'x') {
            if (!add_tlv_value_hex_string(value, length)) {
                err_string = "length smaller than data";
                return false;
            }
        } else if (value[2] == ':') {
            if (!add_tlv_value_mac(value, length)) {
                err_string = "length smaller than data";
                return false;
            }
        } else if (value[1] == 'd') {
            if (!add_tlv_value_decimal_string(value, length)) {
                err_string = "length smaller than data";
                return false;
            }
        } else if (value[1] == 'b') {
            if (!add_tlv_value_binary_string(value, length)) {
                err_string = "length smaller than data";
                return false;
            }
        }
    }

    if (length != 0) {
        err_string = "data smaller than length";
        return false;
    }

    return true;
}

/**
 * @brief Writes TLVs from TLVs list 'tlv_hex_list' into the class buffer.
 * 
 * @param[in] tlv_hex_list An ordered list of TLVs of type "tlv_hex_t".
 * @param[out] err_string Contains an error description if the function fails.
 * @return true if successful, false if not.
 */
bool tlvPrefilledData::add_tlvs_from_list(
    const std::list<beerocks_ucc_listener::tlv_hex_t> &tlv_hex_list, std::string &err_string)
{
    for (const auto &tlv : tlv_hex_list) {
        if (!add_tlv_from_strings(tlv, err_string)) {
            return false;
        }
    }
    return true;
}
