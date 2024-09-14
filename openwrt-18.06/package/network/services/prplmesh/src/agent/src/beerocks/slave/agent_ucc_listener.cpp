/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "agent_ucc_listener.h"
#include "agent_db.h"

#include <bcl/network/network_utils.h>
#include <beerocks/tlvf/beerocks_message.h>

#include <easylogging++.h>

#include "backhaul_manager/backhaul_manager.h"

using namespace beerocks;
using namespace net;

agent_ucc_listener::agent_ucc_listener(BackhaulManager &btl_ctx, ieee1905_1::CmduMessageTx &cmdu,
                                       std::unique_ptr<beerocks::UccServer> ucc_server)
    : beerocks_ucc_listener(cmdu, std::move(ucc_server)), m_btl_ctx(btl_ctx)

{
    m_ucc_listener_run_on = eUccListenerRunOn::AGENT;
}

/**
 * @brief Returns string filled with reply to "DEVICE_GET_INFO" command.
 *
 * @return const std::string Device info in UCC reply format.
 */
std::string agent_ucc_listener::fill_version_reply_string()
{
    auto db = AgentDB::get();
    return std::string("vendor,") + db->device_conf.vendor + std::string(",model,") +
           db->device_conf.model + std::string(",version,") + BEEROCKS_VERSION;
}

/**
 * @brief get parameter command
 *
 * get agent parameter
 *
 * @param[in] params command parsed parameter map
 * @param[out] value returned parameter or error on failure
 * @return true on success
 * @return false on failure
 */
bool agent_ucc_listener::handle_dev_get_param(std::unordered_map<std::string, std::string> &params,
                                              std::string &value)
{
    auto parameter     = params["parameter"];
    auto db            = AgentDB::get();
    sMacAddr mac_value = net::network_utils::ZERO_MAC;

    std::transform(parameter.begin(), parameter.end(), parameter.begin(), ::tolower);
    if (parameter == "alid") {
        value = tlvf::mac_to_string(db->bridge.mac);
        return true;
    } else if (parameter == "macaddr") {
        if (params.find("ruid") == params.end()) {
            value = "missing ruid";
            return false;
        }
        auto ruid = tlvf::mac_from_string(params["ruid"]);
        if (params.find("ssid") == params.end()) {
            // No ssid was given, we need to return the backhaul sta mac.
            auto radio = db->get_radio_by_mac(ruid, AgentDB::eMacType::RADIO);
            if (!radio) {
                LOG(ERROR) << "No radio with ruid '" << params["ruid"] << "' found!";
                value = "No radio with ruid " + params["ruid"];
                return false;
            }
            value = tlvf::mac_to_string(radio->back.iface_mac);
            return true;
        }
        auto ssid = params["ssid"];
        // there is an ssid, lookup the corresponding mac address
        if (!db->get_mac_by_ssid(ruid, ssid, mac_value)) {
            LOG(ERROR) << " failed to find the MAC address for ruid '" << ruid << "'"
                       << " ssid '" << ssid << "'";
            value =
                "macaddr/bssid not found for ruid " + tlvf::mac_to_string(ruid) + " ssid " + ssid;
            return false;
        }
        value = tlvf::mac_to_string(mac_value);
        return true;
    } else if (parameter == "bssid") {
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

        if (!db->get_mac_by_ssid(ruid, ssid, mac_value)) {
            LOG(ERROR) << " failed to find the BSSID for ruid '" << ruid << "'"
                       << " ssid '" << ssid << "'";
            value =
                "macaddr/bssid not found for ruid " + tlvf::mac_to_string(ruid) + " ssid " + ssid;
            return false;
        }
        value = tlvf::mac_to_string(mac_value);
        return true;
    }
    value = "parameter " + parameter + " not supported";
    return false;
}

/**
 * @brief Send CMDU to destined Agent.
 *
 * @param[in] dest_mac Controllers mac address
 * @param[in] cmdu_tx CMDU object
 * @return true if successful, false if not.
 */
bool agent_ucc_listener::send_cmdu_to_destination(ieee1905_1::CmduMessageTx &cmdu_tx,
                                                  const std::string &dest_mac)
{
    auto db = AgentDB::get();
    return m_btl_ctx.send_cmdu_to_broker(cmdu_tx, tlvf::mac_from_string(dest_mac), db->bridge.mac);
}

static enum eFreqType band_to_freq(const std::string &band)
{
    if (band == "24G") {
        return eFreqType::FREQ_24G;
    } else if (band == "5GL") {
        return eFreqType::FREQ_5G;
    } else if (band == "5GH") {
        return eFreqType::FREQ_5G;
    } else {
        return eFreqType::FREQ_UNKNOWN;
    }
}

bool agent_ucc_listener::handle_start_wps_registration(const std::string &band,
                                                       std::string &err_string)
{
    auto freq          = band_to_freq(band);
    auto radio_mac_str = m_btl_ctx.freq_to_radio_mac(freq);
    if (radio_mac_str.empty()) {
        err_string = "Failed to get radio for " + band;
        return false;
    }

    LOG(DEBUG) << "Trigger WPS PBC on radio mac " << radio_mac_str;
    err_string = "Failed to start wps pbc";
    return m_btl_ctx.start_wps_pbc(tlvf::mac_from_string(radio_mac_str));
}

/**
 * @brief Handle DEV_SET_RFEATURE command. Parse the command and send it to the agent.
 *
 * @param[in] params Command parameters.
 * @param[out] err_string Contains an error description if the function fails.
 * @return true if successful, false if not.
 */
bool agent_ucc_listener::handle_dev_set_rfeature(
    const std::unordered_map<std::string, std::string> &params, std::string &err_string)
{
    // The expected command is in the following format:
    // dev_set_rfeature,NAME,$DUT_Name,type,MBO,ruid,$MAUT_RUID,Assoc_Disallow,Enable
    // dev_set_rfeature,NAME,$DUT_Name,type,MBO,bssid,$MAUT_FH_MACAddress,Assoc_Disallow,Enable
    auto type = params.at("type");
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);
    if (type != "mbo") {
        err_string =
            "invalid param value '" + type + "' for param name 'type', accepted value is: MBO";
        LOG(ERROR) << err_string;
        return false;
    }

    sMacAddr ruid, bssid;

    auto ruid_it  = params.find("ruid");
    auto bssid_it = params.find("bssid");
    if (ruid_it != params.end()) {
        ruid  = tlvf::mac_from_string(ruid_it->second);
        bssid = net::network_utils::ZERO_MAC;
    } else if (bssid_it != params.end()) {
        bssid      = tlvf::mac_from_string(bssid_it->second);
        auto db    = AgentDB::get();
        auto radio = db->get_radio_by_mac(bssid, AgentDB::eMacType::BSSID);
        if (radio == nullptr) {
            err_string = "Cannot find radio for the provided bssid " + bssid_it->second;
            LOG(ERROR) << err_string;
            return false;
        }
        ruid = radio->front.iface_mac;
        if (ruid == bssid) {
            err_string = "The provided BSSID is RUID";
            LOG(ERROR) << err_string;
            return false;
        }
    } else {
        err_string = "Command must include RUID or BSSID";
        LOG(ERROR) << err_string;
        return false;
    }

    auto assoc_disallow = params.at("assoc_disallow");
    std::transform(assoc_disallow.begin(), assoc_disallow.end(), assoc_disallow.begin(), ::tolower);

    bool enable = false;
    if (assoc_disallow == "enable") {
        enable = true;
    } else if (assoc_disallow == "disable") {
        enable = false;
    } else {
        err_string = "invalid param value '" + assoc_disallow +
                     "' for param name 'assoc_disallow', accepted value are: ENABLE, DISABLE";
        LOG(ERROR) << err_string;
        return false;
    }

    if (!m_btl_ctx.set_mbo_assoc_disallow(ruid, bssid, enable)) {
        err_string = "Failed to set rfeature";
        LOG(ERROR) << err_string;
        return false;
    }
    return true;
}

bool agent_ucc_listener::handle_custom_command(
    const std::unordered_map<std::string, std::string> &params, std::string &err_string)
{
    err_string = "dev set handle_custom_command not supported in agent mode";
    return false;
}
