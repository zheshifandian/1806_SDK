/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _RDKB_WLAN_TASK_H_
#define _RDKB_WLAN_TASK_H_

#include "../../db/db.h"
#include "../task.h"
#include "../task_pool.h"
#include "rdkb_wlan_task_db.h"

#include <beerocks/tlvf/beerocks_message_common.h>

namespace son {
class rdkb_wlan_task : public task {
public:
    struct listener_general_register_unregister_event {
        int sd;
    };

    struct steering_set_group_request_event {
        int sd;
        uint32_t steeringGroupIndex;
        beerocks_message::sSteeringApConfig cfg_2;
        beerocks_message::sSteeringApConfig cfg_5;
        uint8_t remove;
    };

    struct steering_set_group_response_event {
        int32_t ret_code;
    };

    struct steering_client_set_request_event {
        int sd;
        uint32_t steeringGroupIndex;
        std::string bssid;
        sMacAddr client_mac;
        beerocks_message::sSteeringClientConfig config;
        uint8_t remove;
    };

    struct steering_client_set_response_event {
        int32_t ret_code;
    };

    struct steering_rssi_measurement_request_event {
        int sd;
        std::string bssid;
        beerocks_message::sNodeRssiMeasurementRequest params;
    };

    struct steering_rssi_measurement_response_event {
        int32_t ret_code;
    };

    struct steering_client_disconnect_request_event {
        int sd;
        uint32_t steeringGroupIndex;
        std::string bssid;
        sMacAddr client_mac;
        beerocks_message::eDisconnectType type;
        uint32_t reason;
    };

    struct steering_client_disconnect_response_event {
        int32_t ret_code;
    };

    struct steering_slave_join_event {
        std::string radio_mac;
    };

    enum events {
        STEERING_EVENT_PROBE_REQ_AVAILABLE,
        STEERING_EVENT_CLIENT_CONNECT_AVAILABLE,
        STEERING_EVENT_CLIENT_DISCONNECT_AVAILABLE,
        STEERING_EVENT_CLIENT_ACTIVITY_AVAILABLE,
        STEERING_EVENT_SNR_XING_AVAILABLE,
        STEERING_EVENT_SNR_AVAILABLE,
        STEERING_EVENT_AUTH_FAIL_AVAILABLE,
        STEERING_EVENT_REGISTER,
        STEERING_EVENT_UNREGISTER,
        STEERING_SET_GROUP_REQUEST,
        STEERING_SET_GROUP_RESPONSE,
        STEERING_CLIENT_SET_REQUEST,
        STEERING_CLIENT_SET_RESPONSE,
        STEERING_CLIENT_DISCONNECT_REQUEST,
        STEERING_CLIENT_DISCONNECT_RESPONSE,
        STEERING_RSSI_MEASUREMENT_REQUEST,
        STEERING_RSSI_MEASUREMENT_RESPONSE,
        STEERING_REMOVE_SOCKET,
        STEERING_SLAVE_JOIN,
    };

public:
    rdkb_wlan_task(db &database_, ieee1905_1::CmduMessageTx &cmdu_tx_, task_pool &tasks_);
    virtual ~rdkb_wlan_task() {}

protected:
    virtual void work() override;
    virtual void handle_event(int event_type, void *obj) override;

private:
    typedef struct {
        int sd;
        bool events_updates;
    } sBmlRdkbWlanListener;

    typedef struct {
        int bml_sd;
        std::chrono::steady_clock::time_point timeout;
    } sPendingEvent;

    std::vector<sBmlRdkbWlanListener> bml_rdkb_wlan_listeners_sockets;

    bool is_bml_rdkb_wlan_listener_socket(int sd);
    int get_bml_rdkb_wlan_socket_at(uint32_t idx);
    bool get_bml_rdkb_wlan_events_update_enable(int sd);
    bool set_bml_rdkb_wlan_events_update_enable(int sd, bool update_enable);
    void add_bml_rdkb_wlan_socket(int sd);
    void remove_bml_rdkb_wlan_socket(int sd);
    void send_bml_event_to_listeners(ieee1905_1::CmduMessageTx &cmdu_tx,
                                     const std::vector<int> &bml_listeners);
    bool send_steering_conf_to_agent(const std::string &radio_mac);
    int32_t steering_group_fill_ap_configuration(steering_set_group_request_event *event_obj,
                                                 beerocks_message::sSteeringApConfig &cfg_2,
                                                 beerocks_message::sSteeringApConfig &cfg_5);
    void send_bml_response(int event, int sd, int32_t ret = 0);
    void add_pending_events(int event, int bml_sd, uint32_t amount = 1);
    std::pair<bool, int> check_for_pending_events(int event);
    void pending_event_check_timeout();

    db &database;
    ieee1905_1::CmduMessageTx &cmdu_tx;
    task_pool &tasks;

    rdkb_wlan_task_db rdkb_db;

    std::unordered_multimap<int, sPendingEvent> pending_events;
};

} // namespace son

#endif
