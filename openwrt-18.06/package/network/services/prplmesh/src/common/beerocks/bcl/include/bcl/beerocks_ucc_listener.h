/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef __BEEROCKS_UCC_LISTENER_H__
#define __BEEROCKS_UCC_LISTENER_H__

#include <bcl/beerocks_ucc_server.h>

#include <tlvf/CmduMessageTx.h>

#include <list>
#include <string>
#include <unordered_map>

namespace beerocks {

// Defined by CAPI specifications:
static constexpr uint8_t UCC_REPLY_RUNNING_TIMEOUT_SEC  = 1;
static constexpr uint8_t UCC_REPLY_COMPLETE_TIMEOUT_SEC = 120;

// list of values we support for "program":
static constexpr std::array<const char *, 3> supported_programs = {"map", "mapr2", "mapr3"};

class beerocks_ucc_listener {
public:
    /**
     * Error string included in WFA-CA reply when no handler has been set by the application to 
     * handle received command.
     */
    static constexpr auto unhandled_command_error_string = "No handler for command was set";

    /** 
     * Error string included in WFA-CA reply when an internal error has occurred while handling 
     * received command.
     */
    static constexpr auto command_failed_error_string = "Command failed";

    /**
     * @brief Handler function for "dev_reset_default" WFA-CA command.
     *
     * @param[in] fd File descriptor of the socket connection the command was received through. The 
     * second reply to the command must be sent through this connection.
     * @param[in] params Command parameters.
     */
    using DevResetDefaultHandler =
        std::function<void(int fd, const std::unordered_map<std::string, std::string> &params)>;

    /**
     * @brief Handler function for "dev_set_config" WFA-CA command.
     *
     * @param[in] params Command parameters.
     * @param[out] err_string Contains an error description if the function fails.
     * 
     * @return true on success and false otherwise.
     */
    using DevSetConfigHandler = std::function<bool(
        const std::unordered_map<std::string, std::string> &params, std::string &err_string)>;

    /**
     * Set of command handler functions, one function to handle each possible WFA-CA command 
     * received.
     * Handlers are grouped into a struct to facilitate passing them as a single parameter to the
     * method used to set the handlers.
     * Command handlers are optional and if not set for a given command, that command will be 
     * rejected with an error.
     */
    struct CommandHandlers {
        /**
         * Handler function called back by the UCC listener to process "dev_reset_default".
         */
        DevResetDefaultHandler on_dev_reset_default;

        /**
         * Handler function called back by the UCC listener to process "dev_set_config".
         */
        DevSetConfigHandler on_dev_set_config;
    };

    beerocks_ucc_listener(ieee1905_1::CmduMessageTx &cmdu,
                          std::unique_ptr<beerocks::UccServer> ucc_server);
    virtual ~beerocks_ucc_listener();

    /**
     * @brief Sets the command handler functions.
     *
     * Sets the callback functions to be executed whenever a WFA-CA command is received.
     * The command handler functions are all optional and if any of them is not set, the 
     * corresponding command will be rejected with an error.
     *
     * @param handlers Command handler functions.
     */
    void set_handlers(const CommandHandlers &handlers) { m_handlers = handlers; }

    /**
     * @brief Clears previously set command handler functions.
     */
    void clear_handlers() { m_handlers = {}; }

    /** 
     * @brief Sends second WFA-CA reply message to UCC client.
     * 
     * This method must be invoked when the processing of a WFA-CA command is complete, in order to 
     * send the second reply to UCC client. 
     * 
     * The status code included in the reply is eWfaCaStatus::COMPLETE on success and 
     * eWfaCaStatus::ERROR otherwise (i.e.: if given error description is not empty).
     * 
     * @param[in] fd File descriptor of the socket connection the command was received through. The 
     * reply to the command will be sent through this connection too.
     * @param[in] err_string Empty on success and error description otherwise.
     * 
     * @return true on success and false otherwise.
     */
    bool send_reply(int fd, const std::string &err_string = std::string());

    /**
     * @brief Calls back handler function for "dev_reset_default" WFA-CA command.
     *
     * @param[in] fd File descriptor of the socket connection the command was received through.
     * @param[in] params Command parameters.
     * @param[out] err_string Contains an error description if the function fails.
     * 
     * @return true on success and false otherwise.
     */
    bool handle_dev_reset_default(int fd,
                                  const std::unordered_map<std::string, std::string> &params,
                                  std::string &err_string) const
    {
        if (!m_handlers.on_dev_reset_default) {
            err_string = unhandled_command_error_string;
            return false;
        }

        m_handlers.on_dev_reset_default(fd, params);
        return true;
    }

    /**
     * @brief Calls back handler function for "dev_set_config" WFA-CA command.
     *
     * @param[in] params Command parameters.
     * @param[out] err_string Contains an error description if the function fails.
     * 
     * @return true on success and false otherwise.
     */
    bool handle_dev_set_config(const std::unordered_map<std::string, std::string> &params,
                               std::string &err_string) const
    {
        if (!m_handlers.on_dev_set_config) {
            err_string = unhandled_command_error_string;
            return false;
        }

        return m_handlers.on_dev_set_config(params, err_string);
    }

private:
    /**
     * Set of command handler functions that are called back whenever a WFA-CA command is received 
     * on this listener.
     */
    CommandHandlers m_handlers;

protected:
    // Helper functions
    static std::string check_forbidden_chars(const std::string &str);
    static bool validate_hex_notation(const std::string &str, uint8_t expected_octets = 0);
    static bool validate_binary_notation(const std::string &str);
    static bool validate_decimal_notation(const std::string &str);

    // Virtual functions
    virtual std::string fill_version_reply_string()                                    = 0;
    virtual bool send_cmdu_to_destination(ieee1905_1::CmduMessageTx &cmdu_tx,
                                          const std::string &dest_mac = std::string()) = 0;
    virtual bool handle_start_wps_registration(const std::string &band,
                                               std::string &err_string)                = 0;
    virtual bool handle_dev_get_param(std::unordered_map<std::string, std::string> &params,
                                      std::string &value)                              = 0;
    virtual bool handle_dev_set_rfeature(const std::unordered_map<std::string, std::string> &params,
                                         std::string &err_string)                      = 0;

    virtual bool handle_custom_command(const std::unordered_map<std::string, std::string> &params,
                                       std::string &err_string) = 0;

    enum class eUccListenerRunOn : uint8_t {
        CONTROLLER,
        AGENT,
        NONE,
    };
    eUccListenerRunOn m_ucc_listener_run_on = eUccListenerRunOn::NONE;

    struct tlv_hex_t {
        std::string type;
        std::string length;
        std::string value;
    };
    static bool
    get_send_1905_1_tlv_hex_list(std::list<tlv_hex_t> &tlv_hex_list,
                                 const std::unordered_map<std::string, std::string> &params,
                                 std::string &err_string);
    // Variables
    ieee1905_1::CmduMessageTx &m_cmdu_tx;

private:
    // Helper functions
    enum class eWfaCaCommand : uint8_t {
        CA_GET_VERSION,
        DEVICE_GET_INFO,
        DEV_GET_PARAMETER,
        DEV_RESET_DEFAULT,
        DEV_SEND_1905,
        DEV_SET_CONFIG,
        START_WPS_REGISTRATION,
        DEV_SET_RFEATURE,
        CUSTOM_CMD,
        WFA_CA_COMMAND_MAX,
    };
    static eWfaCaCommand wfa_ca_command_from_string(std::string command);

    enum class eWfaCaStatus : uint8_t { RUNNING, INVALID, ERROR, COMPLETE };
    static const std::string wfa_ca_status_to_string(eWfaCaStatus status);

    static bool parse_params(const std::vector<std::string> &command_tokens,
                             std::unordered_map<std::string, std::string> &params,
                             std::string &err_string);

    // Class functions
    void handle_wfa_ca_command(int fd, const std::string &command);
    bool reply_ucc(int fd, eWfaCaStatus status, const std::string &description = std::string());
    /**
     * @brief Helper function to validate the 'program' parameter
     * given by the UCC and log errors if any.
     *
     * @param[in] parameter the 'program' parameter.
     * @param[out] err_string an error string, if the parameter is not valid.
     * @return true if the parameter is valid, false otherwise.
     **/
    bool validate_program_parameter(const std::string &parameter, std::string &err_string);

    friend class tlvPrefilledData;

    /**
     * UCC server to communicate with a UCC client by exchanging commands and replies.
     */
    std::unique_ptr<beerocks::UccServer> m_ucc_server;
};

class tlvPrefilledData : public BaseClass {
public:
    tlvPrefilledData(uint8_t *buff, size_t buff_len, bool parse = false)
        : BaseClass(buff, buff_len, parse)
    {
        m_init_succeeded = true;
    };
    explicit tlvPrefilledData(std::shared_ptr<BaseClass> base, bool parse = false)
        : BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse)
    {
        m_init_succeeded = true;
    };
    ~tlvPrefilledData(){};

    // No swapping is needed for a prefilled TLV list
    void class_swap() override{};
    // No finalize is needed for a prefilled TLV list
    bool finalize() override { return true; };
    static size_t get_initial_size() { return 0; };

    bool add_tlv_value_hex_string(const std::string &value, uint16_t &length);
    bool add_tlv_value_decimal_string(const std::string &value, uint16_t &length);
    bool add_tlv_value_binary_string(const std::string &value, uint16_t &length);
    bool add_tlv_value_mac(const std::string &value, uint16_t &length);

    /**
     * @brief Add a tlv to the buffer from a single tlv_hex struct of strings.
     * 
     * @param[in] tlv_hex Struct of strings.
     * @param[out] err_string string to report incase of an error.
     * 
     * @return True on success, False on failure.
     */
    bool add_tlv_from_strings(const beerocks_ucc_listener::tlv_hex_t &tlv_hex,
                              std::string &err_string);
    bool add_tlvs_from_list(const std::list<beerocks_ucc_listener::tlv_hex_t> &tlv_hex_list,
                            std::string &err_string);
};
} // namespace beerocks

#endif // __BEEROCKS_UCC_LISTENER_H__
