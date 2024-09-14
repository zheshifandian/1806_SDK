/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2019-2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */
#include <bpl/bpl_board.h>
#include <easylogging++.h>

extern "C" {
// Ignore some warnings from libubus
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <libubox/blobmsg.h>
#include <libubus.h>
}

const unsigned UBUS_CALL_TIMEOUT_MS = 100;
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

namespace beerocks {
namespace bpl {

struct sBoardReleaseParameters {
    std::string distribution;
    std::string version;
    std::string revision;
    std::string target;
    std::string description;
};

struct sBoardParameters {
    std::string kernel;
    std::string hostname;
    std::string system;
    std::string model;
    std::string board_name;
    sBoardReleaseParameters release;
};

// system board parameters policy
enum { KERNEL, HOSTNAME, SYSTEM, MODEL, BOARD_NAME, RELEASE };

static const blobmsg_policy board_params_policy[] = {
    [KERNEL]     = {.name = "kernel", .type = BLOBMSG_TYPE_STRING},
    [HOSTNAME]   = {.name = "hostname", .type = BLOBMSG_TYPE_STRING},
    [SYSTEM]     = {.name = "system", .type = BLOBMSG_TYPE_STRING},
    [MODEL]      = {.name = "model", .type = BLOBMSG_TYPE_STRING},
    [BOARD_NAME] = {.name = "board_name", .type = BLOBMSG_TYPE_STRING},
    [RELEASE]    = {.name = "release", .type = BLOBMSG_TYPE_ARRAY}};

// system board release parameters policy
enum { DISTRIBUTION, VERSION, REVISION, TARGET, DESCRIPTION };

static const blobmsg_policy board_release_params_policy[] = {
    [DISTRIBUTION] = {.name = "distribution", .type = BLOBMSG_TYPE_STRING},
    [VERSION]      = {.name = "version", .type = BLOBMSG_TYPE_STRING},
    [REVISION]     = {.name = "revision", .type = BLOBMSG_TYPE_STRING},
    [TARGET]       = {.name = "target", .type = BLOBMSG_TYPE_STRING},
    [DESCRIPTION]  = {.name = "description", .type = BLOBMSG_TYPE_STRING}};

/**
 * @brief System board data handler which passed as function callback to ubus_invoke.
 *
 * This handler procces system board parameters output.
 *
 * @param [in] req ubus request contains private pointer.
 * @param [in] type message type.
 * @param [in] msg data pointer which received from ubus.
 */
static void system_board_data_handler(ubus_request *req, int type, blob_attr *msg)
{
    if (!msg || !req->priv) {
        return;
    }

    // Passed private pointer to get data from function callback.
    auto board_params = static_cast<sBoardParameters *>(req->priv);

    blob_attr *tb[ARRAY_SIZE(board_params_policy)];

    if (blobmsg_parse(board_params_policy, ARRAY_SIZE(board_params_policy), tb, blobmsg_data(msg),
                      blobmsg_data_len(msg)) != 0) {
        LOG(ERROR) << "Parse failed with board_params_policy";
        return;
    }

    if (tb[KERNEL]) {
        board_params->kernel.assign(blobmsg_get_string(tb[KERNEL]));
    }
    if (tb[HOSTNAME]) {
        board_params->hostname.assign(blobmsg_get_string(tb[HOSTNAME]));
    }
    if (tb[SYSTEM]) {
        board_params->system.assign(blobmsg_get_string(tb[SYSTEM]));
    }
    if (tb[MODEL]) {
        board_params->model.assign(blobmsg_get_string(tb[MODEL]));
    }
    if (tb[BOARD_NAME]) {
        board_params->board_name.assign(blobmsg_get_string(tb[BOARD_NAME]));
    }

    if (tb[RELEASE]) {

        blob_attr *attr;
        auto head = blobmsg_data(tb[RELEASE]);
        auto len  = blobmsg_data_len(tb[RELEASE]);

        // Iterate over all release parameters
        __blob_for_each_attr(attr, head, len)
        {
            blob_attr *tb_release[ARRAY_SIZE(board_release_params_policy)];

            if (blobmsg_parse(board_release_params_policy, ARRAY_SIZE(board_release_params_policy),
                              tb_release, blobmsg_data(attr), blobmsg_data_len(attr)) != 0) {
                LOG(ERROR) << "Parse failed with board_release_params_policy";
                continue;
            }
            if (tb_release[DISTRIBUTION]) {
                board_params->release.distribution.assign(
                    blobmsg_get_string(tb_release[DISTRIBUTION]));
            }
            if (tb_release[VERSION]) {
                board_params->release.version.assign(blobmsg_get_string(tb_release[VERSION]));
            }
            if (tb_release[REVISION]) {
                board_params->release.revision.assign(blobmsg_get_string(tb_release[REVISION]));
            }
            if (tb_release[TARGET]) {
                board_params->release.target.assign(blobmsg_get_string(tb_release[TARGET]));
            }
            if (tb_release[DESCRIPTION]) {
                board_params->release.description.assign(
                    blobmsg_get_string(tb_release[DESCRIPTION]));
            }
        }
    }
}

/**
 * @brief Get all board info parameters from ubus.
 *
 * @param [out] board_params structure with all board parameters.
 * @return Returns true in case of success.
 */
bool get_all_board_info(sBoardParameters &board_params)
{
    int ret_val;
    uint32_t id;

    // Use default ubus socket
    auto s_pUbusCtx = ubus_connect(NULL);

    if (!s_pUbusCtx) {
        LOG(ERROR) << "ubus_connect() is failed";
        return false;
    }

    if (ubus_lookup_id(s_pUbusCtx, "system", &id)) {
        LOG(ERROR) << "Failed to look up system";
        return false;
    }

    ret_val = ubus_invoke(s_pUbusCtx, id, "board", nullptr, system_board_data_handler,
                          static_cast<void *>(&board_params), UBUS_CALL_TIMEOUT_MS);

    ubus_free(s_pUbusCtx);
    s_pUbusCtx = nullptr;

    return (ret_val == 0);
}

bool get_board_info(sDeviceInfo &device_info_params)
{
    sBoardParameters board_params;
    if (!get_all_board_info(board_params)) {
        LOG(ERROR) << "Failed to get board parameters";
        return false;
    }

    device_info_params.manufacturer =
        board_params.board_name.substr(0, board_params.board_name.find(","));
    device_info_params.serial_number      = "prpl12345";
    device_info_params.manufacturer_model = board_params.model;
    return true;
}

} // namespace bpl
} // namespace beerocks
