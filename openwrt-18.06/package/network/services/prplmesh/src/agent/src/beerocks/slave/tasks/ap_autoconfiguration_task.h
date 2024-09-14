/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _AP_AUTOCONFIGURATION_TASK_H_
#define _AP_AUTOCONFIGURATION_TASK_H_

#include "task.h"

#include <mapf/common/encryption.h>
#include <tlvf/CmduMessageTx.h>
#include <tlvf/WSC/configData.h>
#include <tlvf/WSC/m2.h>
#include <tlvf/wfa_map/tlvProfile2ErrorCode.h>

namespace beerocks {

// Forward declaration for Agent context saving
class slave_thread;

class ApAutoConfigurationTask : public Task {
public:
    ApAutoConfigurationTask(slave_thread &btl_ctx, ieee1905_1::CmduMessageTx &cmdu_tx);
    ~ApAutoConfigurationTask() {}

    void work() override;

    enum eEvent : uint8_t {
        INIT_TASK,
        START_AP_AUTOCONFIGURATION,
    };

    void handle_event(uint8_t event_enum_value, const void *event_obj) override;

    bool handle_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx, uint32_t iface_index,
                     const sMacAddr &dst_mac, const sMacAddr &src_mac, int fd,
                     std::shared_ptr<beerocks_header> beerocks_header) override;

private:
    /* Class members */

    /**
     * @brief AP-Autoconfiguration Task states.
     * 
     * The AP-Autoconfiguration task states represent the AP-Autoconfiguration phases described on
     * IEEE 1905.1-2013 standard, section 10.1.
     * The 'CONTROLLER_DISCOVERY' state refers to the 'Registrar Discovery' phase.
     * The 'AP_CONFIGURATION' state refers to the 'Parameter configuration' phase.
     */
    enum class eState : uint8_t {
        UNCONFIGURED,
        CONTROLLER_DISCOVERY,
        WAIT_FOR_CONTROLLER_DISCOVERY_COMPLETE,
        SEND_AP_AUTOCONFIGURATION_WSC_M1,
        WAIT_AP_AUTOCONFIGURATION_WSC_M2,
        SEND_MONITOR_SON_CONFIG,
        CONFIGIRED
    };

    struct sConfigurationParams {
        eState state = eState::UNCONFIGURED;
        std::chrono::steady_clock::time_point timeout;
        std::unique_ptr<mapf::encryption::diffie_hellman> dh = nullptr;
        //copy of M1 message used for authentication
        uint8_t *m1_auth_buf   = nullptr;
        size_t m1_auth_buf_len = 0;
    };

    /**
     * @brief State of AP-Autoconfiguration task on mapped by front radio interface name.
     * 
     * Key:     Front radio interface name.
     * Value:   AP-Autoconfiguration task state struct of the mapped Front radio interface name.
     * 
     * According to IEEE 1905.1-2013, the AP-Autoconfiguration routine shall occur separately
     * for every 1905.1 device supported band. Multi-AP standard extend the definition and add that
     * the 'AP_CONFIGURATION' phase shall occur for each radio of the Multi-AP device (instead of
     * a band).
     */
    std::unordered_map<std::string, sConfigurationParams> m_radios_conf_params;

    /**
     * @brief Convert enum of task state to string.
     * 
     * @param status Enum of task state. 
     * @return state as string.
     */
    static const std::string fsm_state_to_string(eState status);

    /**
     * @brief A map that contains the discovery status on each of the Agent supported bands.
     * 
     * Since the discovery states are attached to each bands instead of each radio, need to hold,
     * helper container to manage it.
     * 
     * Key: Band type.
     * Value: Struct that contain flags on the discovery phase of mapped band.
     */
    struct sDiscoveryStatus {
        bool completed = false;
        bool msg_sent  = false;
    };
    // Decalaring unordered_map with key which is an enum, does not compiles on older gcc version.
    // It was considered a defect in the standard, and was fixed in C++14, and also fixed in the
    // version of libstdc++ shipping with gcc as of 6.1.
    // To make unordered_map work with an enum as key, std::hash<int> function was added as third
    // template argument.
    std::unordered_map<eFreqType, sDiscoveryStatus, std::hash<int>> m_discovery_status;

    bool m_task_is_active = false;

    slave_thread &m_btl_ctx;
    ieee1905_1::CmduMessageTx &m_cmdu_tx;

    /* Message handlers: */

    /**
    * @brief Handles 1905 AP Autoconfiguration message.
    * 
    * @param[in] cmdu_rx Received CMDU.
    * @param[in] src_mac MAC address of the message sender.
    */
    void handle_ap_autoconfiguration_response(ieee1905_1::CmduMessageRx &cmdu_rx,
                                              const sMacAddr &src_mac);

    /**
     * @brief Parse AP-Autoconfiguration and apply configuration if M2 is present.
     *
     * @param cmdu_rx Received CMDU.
     * @return true on success, otherwise false.
     */
    void handle_ap_autoconfiguration_wsc(ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Parse AP-Autoconfiguration Renew message.
     *
     * This function checks the TLVs in the AP-Autoconfiguration Renew message. If OK, it triggers
     * autoconfiguration.
     *
     * @param cmdu_rx received CMDU containing AP-Autoconfiguration Renew.
     */
    void handle_ap_autoconfiguration_wsc_renew(ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Parse Multi-AP Policy Configuration message.
     * 
     * The function parse the message, set it on the database and apply it on the Agent platform.
     * 
     * @param cmdu_rx received CMDU containing AP-Autoconfiguration Renew.
     */
    void handle_multi_ap_policy_config_request(ieee1905_1::CmduMessageRx &cmdu_rx);

    /* TLV handlers: */
    bool handle_profile2_default_802dotq_settings_tlv(ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_profile2_traffic_separation_policy_tlv(
        ieee1905_1::CmduMessageRx &cmdu_rx, std::unordered_set<std::string> &misconfigured_ssids);
    bool handle_wsc_m2_tlv(ieee1905_1::CmduMessageRx &cmdu_rx, const std::string &radio_iface,
                           const std::vector<WSC::m2> &m2_list,
                           std::vector<WSC::configData::config> &configs,
                           std::unordered_set<std::string> &misconfigured_ssids);

    bool handle_ap_autoconfiguration_wsc_vs_extension_tlv(ieee1905_1::CmduMessageRx &cmdu_rx,
                                                          const std::string &radio_iface);

    /* Helper functions */
    bool send_ap_autoconfiguration_search_message(const std::string &radio_iface);

    bool send_ap_autoconfiguration_wsc_m1_message(const std::string &radio_iface);

    bool send_ap_bss_configuration_message(const std::string &radio_iface,
                                           const std::vector<WSC::configData::config> &configs);

    bool send_ap_bss_info_update_request(const std::string &radio_iface);

    bool send_ap_connected_sta_notifications_request(const std::string &radio_iface);

    bool send_platform_version_notification(const std::string &radio_iface,
                                            const std::string &controller_version);

    bool send_monitor_son_config(const std::string &radio_iface,
                                 const beerocks_message::sSonConfig &son_config);

    bool send_error_response_message(
        const std::vector<std::pair<wfa_map::tlvProfile2ErrorCode::eReasonCode, sMacAddr>>
            &bss_errors);

    /**
     * @brief Diffie-Hellman public key exchange keys calculation class member params authkey and
     * keywrapauth are computed on success.
     *
     * @param[in] m2 WSC M2 received from the controller.
     * @param[out] authkey 32 bytes calculated authentication key.
     * @param[out] keywrapkey 16 bytes calculated key wrap key.
     * @return true on success, otherwise false.
     */
    bool ap_autoconfiguration_wsc_calculate_keys(const std::string &fronthaul_iface, WSC::m2 &m2,
                                                 uint8_t authkey[32], uint8_t keywrapkey[16]);

    /**
     * @brief ap autoconfiguration global authenticator attribute calculation.
     *
     * Calculate authentication on the Full M1 || M2* whereas M2* = M2 without the authenticator
     * attribute. M1 is a saved buffer of the swapped M1 sent in the WSC autoconfiguration sent by
     * the agent.
     *
     * @param [in] m2 WSC M2 attribute list from the Controller.
     * @param [out] authkey Authentication key.
     * @return true on success, otherwise false.
     */
    bool ap_autoconfiguration_wsc_authenticate(const std::string &fronthaul_iface, WSC::m2 &m2,
                                               uint8_t authkey[32]);

    /**
     * @brief Parse the encrypted settings from m2, and load the into the BSS configuration
     * @a config.
     * 
     * @param [in] m2 WSC M2 attribute list from the Controller.
     * @param [in] authkey Authentication key.
     * @param [in] keywrapkey Key wrapper.
     * @param [out] config BSS configuration.
     * @return true on success, otherwise false.
     */
    bool ap_autoconfiguration_wsc_parse_encrypted_settings(WSC::m2 &m2, uint8_t authkey[32],
                                                           uint8_t keywrapkey[16],
                                                           WSC::configData::config &config);

    bool add_wsc_m1_tlv(const std::string &radio_iface);
};

} // namespace beerocks

#endif // _AP_AUTOCONFIGURATION_TASK_H_
