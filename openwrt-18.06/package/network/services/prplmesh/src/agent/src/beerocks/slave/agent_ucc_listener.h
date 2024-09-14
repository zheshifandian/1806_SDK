/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef __AGENT_UCC_LISTENER_H__
#define __AGENT_UCC_LISTENER_H__

#include <bcl/beerocks_ucc_listener.h>
#include <beerocks/tlvf/beerocks_message.h>

namespace beerocks {

static const auto DEV_SET_ETH = std::string("eth");

// Forward declaration for BackhaulManager context saving
class BackhaulManager;

class agent_ucc_listener : public beerocks_ucc_listener {
public:
    agent_ucc_listener(BackhaulManager &btl_ctx, ieee1905_1::CmduMessageTx &cmdu,
                       std::unique_ptr<beerocks::UccServer> ucc_server);

    void update_vaps_list(std::string ruid, beerocks_message::sVapsList &vaps);

private:
    std::string fill_version_reply_string() override;
    bool send_cmdu_to_destination(ieee1905_1::CmduMessageTx &cmdu_tx,
                                  const std::string &dest_mac = std::string()) override;
    bool handle_start_wps_registration(const std::string &band, std::string &err_string) override;
    bool handle_dev_get_param(std::unordered_map<std::string, std::string> &params,
                              std::string &value) override;
    bool handle_dev_set_rfeature(const std::unordered_map<std::string, std::string> &params,
                                 std::string &err_string) override;

    bool handle_custom_command(const std::unordered_map<std::string, std::string> &params,
                               std::string &err_string) override;

    BackhaulManager &m_btl_ctx;
};

} // namespace beerocks

#endif // __AGENT_UCC_LISTENER_H__
