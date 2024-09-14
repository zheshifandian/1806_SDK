/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "controller_ucc_listener.h"
#include "son_actions.h"

#include <tlvf/ieee_1905_1/tlvAlMacAddress.h>
#include <tlvf/ieee_1905_1/tlvMacAddress.h>
#include <tlvf/wfa_map/tlvProfile2Default802dotQSettings.h>
#include <tlvf/wfa_map/tlvProfile2TrafficSeparationPolicy.h>

using namespace beerocks;

controller_ucc_listener::controller_ucc_listener(db &database, ieee1905_1::CmduMessageTx &cmdu,
                                                 std::unique_ptr<beerocks::UccServer> ucc_server)
    : beerocks_ucc_listener(cmdu, std::move(ucc_server)), m_database(database)
{
    m_ucc_listener_run_on = eUccListenerRunOn::CONTROLLER;

    // Install handlers for WFA-CA commands
    beerocks::beerocks_ucc_listener::CommandHandlers handlers;
    handlers.on_dev_reset_default =
        [&](int fd, const std::unordered_map<std::string, std::string> &params) {
            handle_dev_reset_default(fd, params);
        };
    handlers.on_dev_set_config = [&](const std::unordered_map<std::string, std::string> &params,
                                     std::string &err_string) {
        return handle_dev_set_config(params, err_string);
    };
    set_handlers(handlers);
}

controller_ucc_listener::~controller_ucc_listener() { clear_handlers(); }

/**
 * @brief Returns string filled with reply to "DEVICE_GET_INFO" command.
 *
 * @return const std::string Device info in UCC reply format.
 */
std::string controller_ucc_listener::fill_version_reply_string()
{
    return std::string("vendor,") + m_database.settings_vendor() + std::string(",model,") +
           m_database.settings_model() + std::string(",version,") + BEEROCKS_VERSION;
}

/**
 * @brief Clear configuration on Controller database.
 *
 * @return true on success and false otherwise.
 */
bool controller_ucc_listener::clear_configuration()
{
    m_database.clear_bss_info_configuration();
    m_database.clear_traffic_separation_configurations();
    m_database.clear_default_8021q_settings();
    m_database.disable_periodic_link_metrics_requests();
    return true;
}

/**
 * @brief Clear configuration on Controller database for an Agent.
 *
 * @return true on success and false otherwise.
 */
bool controller_ucc_listener::clear_configuration(const sMacAddr &al_mac)
{
    m_database.clear_bss_info_configuration(al_mac);
    m_database.clear_traffic_separation_configurations(al_mac);
    m_database.clear_default_8021q_settings(al_mac);
    return true;
}

/**
 * @brief get parameter command
 *
 * get controller parameter
 *
 * @param[in] params command parsed parameter map
 * @param[out] value returned parameter or error on failure
 * @return true on success
 * @return false on failure
 */
bool controller_ucc_listener::handle_dev_get_param(
    std::unordered_map<std::string, std::string> &params, std::string &value)
{
    auto parameter = params["parameter"];
    std::transform(parameter.begin(), parameter.end(), parameter.begin(), ::tolower);
    if (parameter == "alid") {
        value = tlvf::mac_to_string(m_database.get_local_bridge_mac());
        return true;
    } else if (parameter == "macaddr" || parameter == "bssid") {
        if (params.find("ruid") == params.end()) {
            value = "missing ruid";
            return false;
        }
        if (params.find("ssid") == params.end()) {
            value = "missing ssid";
            return false;
        }
        auto ruid = tlvf::mac_from_string(params["ruid"]);
        auto ssid = params["ssid"];
        auto vaps = m_database.get_hostap_vap_list(ruid);
        if (vaps.empty()) {
            value = "ruid " + tlvf::mac_to_string(ruid) + " not found";
            return false;
        }
        for (const auto &vap : vaps) {
            if (std::string(vap.second.ssid) == ssid) {
                value = vap.second.mac;
                return true;
            }
        }
        value = "macaddr/bssid not found for ruid " + tlvf::mac_to_string(ruid) + " ssid " + ssid;
        return false;
    }
    value = "parameter " + parameter + " not supported";
    return false;
}

/**
 * @brief Send CMDU to destined Agent.
 *
 * @param[in] dest_mac Agents mac address.
 * @param[in] cmdu_tx CMDU object.
 * @return true if successful, false if not.
 */
bool controller_ucc_listener::send_cmdu_to_destination(ieee1905_1::CmduMessageTx &cmdu_tx,
                                                       const std::string &dest_mac)
{
    return son_actions::send_cmdu_to_agent(tlvf::mac_from_string(dest_mac), cmdu_tx, m_database);
}

bool controller_ucc_listener::handle_start_wps_registration(const std::string &band,
                                                            std::string &err_string)
{
    err_string = "wps registration not supported in controller mode";
    return false;
}

bool controller_ucc_listener::handle_dev_set_rfeature(
    const std::unordered_map<std::string, std::string> &params, std::string &err_string)
{
    err_string = "dev set rfeature not supported in controller mode";
    return false;
}

bool controller_ucc_listener::handle_custom_command(
    const std::unordered_map<std::string, std::string> &params, std::string &err_string)
{
    const auto &cmd = params.at("cmd");
    if (cmd == "discovery_burst") {
        /**
         * @brief Send a burst of a given number @a repeats of Topology Discovery message from a
         * given basic source mac @a base_mac.
         * 
         * @param base_mac Basic mac that will be used as source mac address and will be included
         * in the TLV content. For every repeat, the mac least significate byte will increment.
         * @param repeats Number of messages that will be sent. Limited to 255.
         */

        // Extract command parameters
        static const std::string base_mac_param("base_mac");
        auto it = params.find(base_mac_param);
        if (it == params.end()) {
            err_string = "missing " + base_mac_param;
            return false;
        }
        auto base_mac = tlvf::mac_from_string(it->second);

        static const std::string repeats_param("repeats");
        it = params.find(repeats_param);
        if (it == params.end()) {
            err_string = "missing " + repeats_param;
            return false;
        }
        unsigned repeats = beerocks::string_utils::stoi(it->second);
        repeats &= 0xFF;

        static const std::string dst_param("dst");
        it       = params.find(dst_param);
        auto dst = it == params.end() ? net::network_utils::MULTICAST_1905_MAC_ADDR
                                      : tlvf::mac_from_string(it->second);

        // Creatring the message
        auto cmdu_hdr = m_cmdu_tx.create(0, ieee1905_1::eMessageType::TOPOLOGY_DISCOVERY_MESSAGE);
        auto tlvAlMacAddress = m_cmdu_tx.addClass<ieee1905_1::tlvAlMacAddress>();
        if (!tlvAlMacAddress) {
            LOG(ERROR) << "Failed to create tlvAlMacAddress tlv";
            return false;
        }
        tlvAlMacAddress->mac() = base_mac;

        auto tlvMacAddress = m_cmdu_tx.addClass<ieee1905_1::tlvMacAddress>();
        if (!tlvMacAddress) {
            LOG(ERROR) << "Failed to create tlvMacAddress tlv";
            return false;
        }
        tlvMacAddress->mac() = base_mac;

        auto controller_ctx = m_database.get_controller_ctx();
        if (!controller_ctx) {
            LOG(ERROR) << "controller_ctx == nullptr";
            return false;
        }

        auto start = std::chrono::steady_clock::now();

        for (uint8_t i = 0; i < repeats; ++i) {
            cmdu_hdr->message_id() = i;
            cmdu_hdr->class_swap();
            tlvAlMacAddress->mac().oct[5] = i;
            tlvMacAddress->mac().oct[5]   = i;
            controller_ctx->send_cmdu_to_broker(m_cmdu_tx, dst, tlvAlMacAddress->mac());
        }

        auto delta = std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::steady_clock::now() - start)
                         .count();
        LOG(DEBUG) << "Sent " << repeats << " discovery messages in " << delta << " us";
    }
    return true;
}

void controller_ucc_listener::handle_dev_reset_default(
    int fd, const std::unordered_map<std::string, std::string> &params)
{
    // Clear configuration on Controller's database.
    clear_configuration();

    // Send back second reply to UCC client.
    send_reply(fd);
}

bool controller_ucc_listener::handle_dev_set_config(
    const std::unordered_map<std::string, std::string> &params, std::string &err_string)
{
    if (params.find("backhaul") != params.end()) {
        err_string = "parameter 'backhaul' is not relevant to the controller";
        return false;
    }

    static const std::string key("bss_info");
    if (params.find(key) == params.end()) {
        err_string = "command has no bss_info configuration";
        return false;
    }

    if (params.find("first_bss") != params.end()) {
        m_bss_info_cleared_mac.clear();
    }

    son::wireless_utils::sBssInfoConf bss_info_conf;
    auto al_mac = parse_bss_info(params.at(key), bss_info_conf, err_string);
    if (al_mac.empty()) {
        err_string += (" on " + key);
        return false;
    }

    auto mac = tlvf::mac_from_string(al_mac);
    if (m_bss_info_cleared_mac.find(mac) == m_bss_info_cleared_mac.end()) {
        clear_configuration(mac);
        m_bss_info_cleared_mac.insert(mac);
    }

    // If SSID is empty, tear down - clear configuration for this mac.
    if (bss_info_conf.ssid.empty()) {
        clear_configuration(mac);
        return true;
    }
    m_database.add_bss_info_configuration(mac, bss_info_conf);

    std::list<tlv_hex_t> tlv_hex_list;
    if (!get_send_1905_1_tlv_hex_list(tlv_hex_list, params, err_string)) {
        return false;
    }

    if (tlv_hex_list.empty()) {
        // TLV list is not mandatory
        return true;
    }

    // Use CMDU_TX buffer as a temporary buffer
    uint8_t *buffer      = m_cmdu_tx.getMessageBuff();
    size_t buffer_length = m_cmdu_tx.getMessageBuffLength();
    for (const auto &tlv : tlv_hex_list) {
        tlvPrefilledData prefilled_tlv(buffer, buffer_length);

        if (!prefilled_tlv.add_tlv_from_strings(tlv, err_string)) {
            LOG(ERROR) << err_string;
            return false;
        }

        wfa_map::eTlvTypeMap type =
            wfa_map::eTlvTypeMap(std::strtoul(tlv.type.c_str(), nullptr, 16));
        switch (type) {
        case wfa_map::eTlvTypeMap::TLV_PROFILE2_DEFAULT_802_1Q_SETTINGS: {
            wfa_map::tlvProfile2Default802dotQSettings default_802_1q_tlv(buffer, buffer_length,
                                                                          true);

            wireless_utils::s8021QSettings config;
            config.primary_vlan_id = default_802_1q_tlv.primary_vlan_id();
            config.default_pcp     = default_802_1q_tlv.default_pcp();
            m_database.add_default_8021q_settings(mac, config);
        } break;
        case wfa_map::eTlvTypeMap::TLV_PROFILE2_TRAFFIC_SEPARATION_POLICY: {
            wfa_map::tlvProfile2TrafficSeparationPolicy traffic_separation_policy(
                buffer, buffer_length, true);

            uint8_t ssids_length = traffic_separation_policy.ssids_vlan_id_list_length();
            for (uint8_t idx = 0; idx < ssids_length; idx++) {
                auto ssid_tuple = traffic_separation_policy.ssids_vlan_id_list(idx);
                if (!std::get<0>(ssid_tuple)) {
                    err_string = "Failed getting ssid on index " + std::to_string(idx);
                    LOG(ERROR) << err_string;
                    return false;
                }

                auto &ssid_conf = std::get<1>(ssid_tuple);

                wireless_utils::sTrafficSeparationSsid config;
                config.ssid    = ssid_conf.ssid_name_str();
                config.vlan_id = ssid_conf.vlan_id();
                m_database.add_traffic_separataion_configuration(mac, config);
            }
        } break;
        default: {
            LOG(WARNING) << "Unexpected TLV type " << std::hex << int(type) << std::dec;
        } break;
        }
    }

    return true;
}

/**
 * @brief Parse bss_info string into bss_info_conf_struct.
 *
 * @param[in] bss_info_str String containing bss info configuration.
 * @param[out] bss_info_conf Controller database struct filled with configuration.
 * @param[out] err_string Contains an error description if the function fails.
 * @return al_mac on success, empty string if not.
 */
std::string
controller_ucc_listener::parse_bss_info(const std::string &bss_info_str,
                                        son::wireless_utils::sBssInfoConf &bss_info_conf,
                                        std::string &err_string)
{
    auto confs = string_utils::str_split(bss_info_str, ' ');

    /*
    The Control API specification defines 8 parameters except for the case of
    clearing the BSS info stored for a specific operating class, define only
    two parameters: ALID and operating class.
    */
    if ((confs.size() != 8) && (confs.size() != 2)) {
        err_string = "missing configuration";
        return std::string();
    }

    // Alid
    std::string al_mac(confs[0]);
    if (!net::network_utils::is_valid_mac(al_mac)) {
        err_string = "invalid al_mac";
        return std::string();
    }

    bss_info_conf = {};

    // Operating class
    const auto &operating_class_str = confs[1];
    if (operating_class_str == "8x") {
        bss_info_conf.operating_class = {81, 83, 84};
    } else if (operating_class_str == "11x") {
        bss_info_conf.operating_class = {115, 116};
    } else if (operating_class_str == "12x") {
        bss_info_conf.operating_class = {124, 125, 126};
    } else {
        err_string = "invalid operating class " + operating_class_str;
        return std::string();
    }

    if (confs.size() == 2) {
        return al_mac;
    }

    // SSID
    bss_info_conf.ssid = confs[2];
    if (bss_info_conf.ssid.size() > WSC::eWscLengths::WSC_MAX_SSID_LENGTH) {
        err_string = "ssid is too long";
        return std::string();
    }

    // Authentication type
    auto &authentication_type_str = confs[3];
    if (!beerocks_ucc_listener::validate_hex_notation(authentication_type_str, 2)) {
        err_string = "invalid authentication type format";
        return std::string();
    }

    uint16_t authentication_type = std::strtol(authentication_type_str.c_str(), nullptr, 16);

    bss_info_conf.authentication_type = static_cast<WSC::eWscAuth>(authentication_type);

    // Encryption type
    auto &encryption_type_str = confs[4];
    if (!beerocks_ucc_listener::validate_hex_notation(encryption_type_str, 2)) {
        err_string = "invalid encryption type format";
        return std::string();
    }

    uint16_t encryption_type = std::strtol(encryption_type_str.c_str(), nullptr, 16);

    if (!WSC::eWscEncrValidate::check(encryption_type)) {
        err_string = "invalid encryption type value";
        return std::string();
    }
    bss_info_conf.encryption_type = static_cast<WSC::eWscEncr>(encryption_type);

    // Network key
    bss_info_conf.network_key = confs[5];
    if (bss_info_conf.network_key.size() > WSC::eWscLengths::WSC_MAX_NETWORK_KEY_LENGTH) {
        err_string = "network key is too long";
        return std::string();
    }

    // Bit 6 of Multi-AP IE's extention attribute, aka "Backhaul BSS"
    const auto &bit_6_str = confs[6];
    if (bit_6_str != "0" && bit_6_str != "1") {
        err_string = "invalid bit 6 of Multi-AP IE's extention attribute";
        return std::string();
    }
    if (bit_6_str == "1") {
        bss_info_conf.backhaul = true;
    }

    // Bit 5 of Multi-AP IE's extention attribute, aka "Fronthaul BSS"
    const auto &bit_5_str = confs[7];
    if (bit_5_str != "0" && bit_5_str != "1") {
        err_string = "invalid bit 5 of Multi-AP IE's extention attribute";
        return std::string();
    }
    if (bit_5_str == "1") {
        bss_info_conf.fronthaul = true;
    }

    return al_mac;
}
