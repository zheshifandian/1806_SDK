///////////////////////////////////////
// AUTO GENERATED FILE - DO NOT EDIT //
///////////////////////////////////////

/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_TLVF_BEEROCKS_MESSAGE_APMANAGER_H_
#define _BEEROCKS_TLVF_BEEROCKS_MESSAGE_APMANAGER_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include <tuple>
#include <vector>
#include "beerocks/tlvf/beerocks_message_common.h"
#include "classes/ChannelList.h"
#include "structs/sCacStartedNotificationParams.h"
#include "tlvf/WSC/WSC_Attributes.h"

namespace beerocks_message {


class cACTION_APMANAGER_UP_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_UP_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_UP_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_UP_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_UP_NOTIFICATION);
        }
        uint8_t& iface_name_length();
        std::string iface_name_str();
        char* iface_name(size_t length = 0);
        bool set_iface_name(const std::string& str);
        bool set_iface_name(const char buffer[], size_t size);
        bool alloc_iface_name(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        uint8_t* m_iface_name_length = nullptr;
        char* m_iface_name = nullptr;
        size_t m_iface_name_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

class cACTION_APMANAGER_CONFIGURE : public BaseClass
{
    public:
        cACTION_APMANAGER_CONFIGURE(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_CONFIGURE(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_CONFIGURE();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_CONFIGURE);
        }
        uint8_t& channel();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        uint8_t* m_channel = nullptr;
};

class cACTION_APMANAGER_JOINED_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_JOINED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_JOINED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_JOINED_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_JOINED_NOTIFICATION);
        }
        sNodeHostap& params();
        sApChannelSwitch& cs_params();
        bool isPostInitSucceeded() override;
        std::shared_ptr<cChannelList> create_channel_list();
        bool add_channel_list(std::shared_ptr<cChannelList> ptr);
        std::shared_ptr<cChannelList> channel_list() { return m_channel_list_ptr; }
        sVapsList& vap_list();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sNodeHostap* m_params = nullptr;
        sApChannelSwitch* m_cs_params = nullptr;
        cChannelList *m_channel_list = nullptr;
        std::shared_ptr<cChannelList> m_channel_list_ptr = nullptr;
        bool m_channel_list_init = false;
        bool m_lock_allocation__ = false;
        int m_lock_order_counter__ = 0;
        sVapsList* m_vap_list = nullptr;
};

class cACTION_APMANAGER_ENABLE_APS_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_ENABLE_APS_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_ENABLE_APS_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_ENABLE_APS_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_ENABLE_APS_REQUEST);
        }
        uint8_t& channel();
        beerocks::eWiFiBandwidth& bandwidth();
        uint8_t& center_channel();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        uint8_t* m_channel = nullptr;
        beerocks::eWiFiBandwidth* m_bandwidth = nullptr;
        uint8_t* m_center_channel = nullptr;
};

class cACTION_APMANAGER_ENABLE_APS_RESPONSE : public BaseClass
{
    public:
        cACTION_APMANAGER_ENABLE_APS_RESPONSE(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_ENABLE_APS_RESPONSE(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_ENABLE_APS_RESPONSE();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_ENABLE_APS_RESPONSE);
        }
        uint8_t& success();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        uint8_t* m_success = nullptr;
};

class cACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_REQUEST);
        }
        sApSetRestrictedFailsafe& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sApSetRestrictedFailsafe* m_params = nullptr;
};

class cACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE);
        }
        uint8_t& success();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        uint8_t* m_success = nullptr;
};

class cACTION_APMANAGER_HOSTAP_AP_DISABLED_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_AP_DISABLED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_AP_DISABLED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_AP_DISABLED_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_AP_DISABLED_NOTIFICATION);
        }
        int8_t& vap_id();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        int8_t* m_vap_id = nullptr;
};

class cACTION_APMANAGER_HOSTAP_AP_ENABLED_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_AP_ENABLED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_AP_ENABLED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_AP_ENABLED_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_AP_ENABLED_NOTIFICATION);
        }
        int8_t& vap_id();
        sVapInfo& vap_info();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        int8_t* m_vap_id = nullptr;
        sVapInfo* m_vap_info = nullptr;
};

class cACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_REQUEST);
        }
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
};

class cACTION_APMANAGER_HOSTAP_GENERATE_CLIENT_ASSOCIATION_NOTIFICATIONS_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_GENERATE_CLIENT_ASSOCIATION_NOTIFICATIONS_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_GENERATE_CLIENT_ASSOCIATION_NOTIFICATIONS_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_GENERATE_CLIENT_ASSOCIATION_NOTIFICATIONS_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_GENERATE_CLIENT_ASSOCIATION_NOTIFICATIONS_REQUEST);
        }
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
};

class cACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION);
        }
        sVapsList& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sVapsList* m_params = nullptr;
};

class cACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START);
        }
        sApChannelSwitch& cs_params();
        int8_t& tx_limit();
        uint8_t& tx_limit_valid();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sApChannelSwitch* m_cs_params = nullptr;
        int8_t* m_tx_limit = nullptr;
        uint8_t* m_tx_limit_valid = nullptr;
};

class cACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST);
        }
        sApChannelSwitch& cs_params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sApChannelSwitch* m_cs_params = nullptr;
};

class cACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE);
        }
        uint8_t& success();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        uint8_t* m_success = nullptr;
};

class cACTION_APMANAGER_HOSTAP_CSA_ERROR_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_CSA_ERROR_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_CSA_ERROR_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_CSA_ERROR_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_CSA_ERROR_NOTIFICATION);
        }
        sApChannelSwitch& cs_params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sApChannelSwitch* m_cs_params = nullptr;
};

class cACTION_APMANAGER_HOSTAP_CSA_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_CSA_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_CSA_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_CSA_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_CSA_NOTIFICATION);
        }
        sApChannelSwitch& cs_params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sApChannelSwitch* m_cs_params = nullptr;
};

class cACTION_APMANAGER_HOSTAP_ACS_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_ACS_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_ACS_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_ACS_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_ACS_NOTIFICATION);
        }
        sApChannelSwitch& cs_params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sApChannelSwitch* m_cs_params = nullptr;
};

class cACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION);
        }
        sCacStartedNotificationParams& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sCacStartedNotificationParams* m_params = nullptr;
};

class cACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION);
        }
        sDfsCacCompleted& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sDfsCacCompleted* m_params = nullptr;
};

class cACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION);
        }
        sDfsChannelAvailable& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sDfsChannelAvailable* m_params = nullptr;
};

class cACTION_APMANAGER_HOSTAP_SET_NEIGHBOR_11K_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_SET_NEIGHBOR_11K_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_SET_NEIGHBOR_11K_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_SET_NEIGHBOR_11K_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_SET_NEIGHBOR_11K_REQUEST);
        }
        sNeighborSetParams11k& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sNeighborSetParams11k* m_params = nullptr;
};

class cACTION_APMANAGER_HOSTAP_REMOVE_NEIGHBOR_11K_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_REMOVE_NEIGHBOR_11K_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_REMOVE_NEIGHBOR_11K_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_REMOVE_NEIGHBOR_11K_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_REMOVE_NEIGHBOR_11K_REQUEST);
        }
        sNeighborRemoveParams11k& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sNeighborRemoveParams11k* m_params = nullptr;
};

class cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST);
        }
        uint8_t& ant_switch_on();
        uint8_t& channel();
        beerocks::eWiFiBandwidth& bandwidth();
        uint32_t& center_frequency();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        uint8_t* m_ant_switch_on = nullptr;
        uint8_t* m_channel = nullptr;
        beerocks::eWiFiBandwidth* m_bandwidth = nullptr;
        uint32_t* m_center_frequency = nullptr;
};

class cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE);
        }
        uint8_t& success();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        uint8_t* m_success = nullptr;
};

class cACTION_APMANAGER_HOSTAP_SET_PRIMARY_VLAN_ID_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_HOSTAP_SET_PRIMARY_VLAN_ID_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HOSTAP_SET_PRIMARY_VLAN_ID_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HOSTAP_SET_PRIMARY_VLAN_ID_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HOSTAP_SET_PRIMARY_VLAN_ID_REQUEST);
        }
        uint16_t& primary_vlan_id();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        uint16_t* m_primary_vlan_id = nullptr;
};

class cACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION);
        }
        sMacAddr& mac();
        sMacAddr& bssid();
        beerocks::message::sRadioCapabilities& capabilities();
        int8_t& vap_id();
        //0 - Not Multi-AP station
        //1 - Profile 1 Agent
        //2 - Profile 2 Agent
        uint8_t& multi_ap_profile();
        size_t association_frame_length() { return m_association_frame_idx__ * sizeof(uint8_t); }
        uint8_t* association_frame(size_t idx = 0);
        bool set_association_frame(const void* buffer, size_t size);
        bool alloc_association_frame(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sMacAddr* m_mac = nullptr;
        sMacAddr* m_bssid = nullptr;
        beerocks::message::sRadioCapabilities* m_capabilities = nullptr;
        int8_t* m_vap_id = nullptr;
        uint8_t* m_multi_ap_profile = nullptr;
        uint8_t* m_association_frame = nullptr;
        size_t m_association_frame_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

class cACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION);
        }
        sClientDisconnectionParams& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sClientDisconnectionParams* m_params = nullptr;
};

class cACTION_APMANAGER_CLIENT_DISCONNECT_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_CLIENT_DISCONNECT_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_CLIENT_DISCONNECT_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_CLIENT_DISCONNECT_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_CLIENT_DISCONNECT_REQUEST);
        }
        sMacAddr& mac();
        int8_t& vap_id();
        eDisconnectType& type();
        uint32_t& reason();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sMacAddr* m_mac = nullptr;
        int8_t* m_vap_id = nullptr;
        eDisconnectType* m_type = nullptr;
        uint32_t* m_reason = nullptr;
};

class cACTION_APMANAGER_CLIENT_DISCONNECT_RESPONSE : public BaseClass
{
    public:
        cACTION_APMANAGER_CLIENT_DISCONNECT_RESPONSE(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_CLIENT_DISCONNECT_RESPONSE(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_CLIENT_DISCONNECT_RESPONSE();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_CLIENT_DISCONNECT_RESPONSE);
        }
        sClientDisconnectResponse& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sClientDisconnectResponse* m_params = nullptr;
};

class cACTION_APMANAGER_CLIENT_DISALLOW_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_CLIENT_DISALLOW_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_CLIENT_DISALLOW_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_CLIENT_DISALLOW_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_CLIENT_DISALLOW_REQUEST);
        }
        sMacAddr& mac();
        sMacAddr& bssid();
        uint16_t& validity_period_sec();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sMacAddr* m_mac = nullptr;
        sMacAddr* m_bssid = nullptr;
        uint16_t* m_validity_period_sec = nullptr;
};

class cACTION_APMANAGER_CLIENT_ALLOW_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_CLIENT_ALLOW_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_CLIENT_ALLOW_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_CLIENT_ALLOW_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_CLIENT_ALLOW_REQUEST);
        }
        sMacAddr& mac();
        sMacAddr& bssid();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sMacAddr* m_mac = nullptr;
        sMacAddr* m_bssid = nullptr;
};

class cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_REQUEST);
        }
        sNodeRssiMeasurementRequest& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sNodeRssiMeasurementRequest* m_params = nullptr;
};

class cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE : public BaseClass
{
    public:
        cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE);
        }
        sNodeRssiMeasurement& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sNodeRssiMeasurement* m_params = nullptr;
};

class cACTION_APMANAGER_ACK : public BaseClass
{
    public:
        cACTION_APMANAGER_ACK(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_ACK(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_ACK();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_ACK);
        }
        uint8_t& reason();
        sMacAddr& sta_mac();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        uint8_t* m_reason = nullptr;
        sMacAddr* m_sta_mac = nullptr;
};

class cACTION_APMANAGER_CLIENT_BSS_STEER_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_CLIENT_BSS_STEER_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_CLIENT_BSS_STEER_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_CLIENT_BSS_STEER_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_CLIENT_BSS_STEER_REQUEST);
        }
        sNodeBssSteerRequest& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sNodeBssSteerRequest* m_params = nullptr;
};

class cACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE : public BaseClass
{
    public:
        cACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE);
        }
        sNodeBssSteerResponse& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sNodeBssSteerResponse* m_params = nullptr;
};

class cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE : public BaseClass
{
    public:
        cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE);
        }
        sMacAddr& mac();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sMacAddr* m_mac = nullptr;
};

class cACTION_APMANAGER_STEERING_CLIENT_SET_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_STEERING_CLIENT_SET_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_STEERING_CLIENT_SET_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_STEERING_CLIENT_SET_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_STEERING_CLIENT_SET_REQUEST);
        }
        sSteeringClientSetRequest& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sSteeringClientSetRequest* m_params = nullptr;
};

class cACTION_APMANAGER_STEERING_CLIENT_SET_RESPONSE : public BaseClass
{
    public:
        cACTION_APMANAGER_STEERING_CLIENT_SET_RESPONSE(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_STEERING_CLIENT_SET_RESPONSE(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_STEERING_CLIENT_SET_RESPONSE();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_STEERING_CLIENT_SET_RESPONSE);
        }
        sSteeringClientSetResponse& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sSteeringClientSetResponse* m_params = nullptr;
};

class cACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION);
        }
        sSteeringEvProbeReq& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sSteeringEvProbeReq* m_params = nullptr;
};

class cACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_NOTIFICATION);
        }
        sSteeringEvAuthFail& params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        sSteeringEvAuthFail* m_params = nullptr;
};

class cACTION_APMANAGER_WIFI_CREDENTIALS_UPDATE_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_WIFI_CREDENTIALS_UPDATE_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_WIFI_CREDENTIALS_UPDATE_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_WIFI_CREDENTIALS_UPDATE_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_WIFI_CREDENTIALS_UPDATE_REQUEST);
        }
        uint8_t& wifi_credentials_size();
        std::tuple<bool, WSC::cConfigData&> wifi_credentials(size_t idx);
        std::shared_ptr<WSC::cConfigData> create_wifi_credentials();
        bool add_wifi_credentials(std::shared_ptr<WSC::cConfigData> ptr);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        uint8_t* m_wifi_credentials_size = nullptr;
        WSC::cConfigData* m_wifi_credentials = nullptr;
        size_t m_wifi_credentials_idx__ = 0;
        std::vector<std::shared_ptr<WSC::cConfigData>> m_wifi_credentials_vector;
        bool m_lock_allocation__ = false;
        int m_lock_order_counter__ = 0;
};

class cACTION_APMANAGER_START_WPS_PBC_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_START_WPS_PBC_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_START_WPS_PBC_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_START_WPS_PBC_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_START_WPS_PBC_REQUEST);
        }
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
};

class cACTION_APMANAGER_SET_ASSOC_DISALLOW_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_SET_ASSOC_DISALLOW_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_SET_ASSOC_DISALLOW_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_SET_ASSOC_DISALLOW_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_SET_ASSOC_DISALLOW_REQUEST);
        }
        uint8_t& enable();
        sMacAddr& bssid();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        uint8_t* m_enable = nullptr;
        sMacAddr* m_bssid = nullptr;
};

class cACTION_APMANAGER_RADIO_DISABLE_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_RADIO_DISABLE_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_RADIO_DISABLE_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_RADIO_DISABLE_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_RADIO_DISABLE_REQUEST);
        }
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
};

class cACTION_APMANAGER_HEARTBEAT_NOTIFICATION : public BaseClass
{
    public:
        cACTION_APMANAGER_HEARTBEAT_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_HEARTBEAT_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_HEARTBEAT_NOTIFICATION();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_HEARTBEAT_NOTIFICATION);
        }
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
};

class cACTION_APMANAGER_CHANNELS_LIST_REQUEST : public BaseClass
{
    public:
        cACTION_APMANAGER_CHANNELS_LIST_REQUEST(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_CHANNELS_LIST_REQUEST(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_CHANNELS_LIST_REQUEST();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_CHANNELS_LIST_REQUEST);
        }
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
};

class cACTION_APMANAGER_CHANNELS_LIST_RESPONSE : public BaseClass
{
    public:
        cACTION_APMANAGER_CHANNELS_LIST_RESPONSE(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cACTION_APMANAGER_CHANNELS_LIST_RESPONSE(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cACTION_APMANAGER_CHANNELS_LIST_RESPONSE();

        static eActionOp_APMANAGER get_action_op(){
            return (eActionOp_APMANAGER)(ACTION_APMANAGER_CHANNELS_LIST_RESPONSE);
        }
        bool isPostInitSucceeded() override;
        std::shared_ptr<cChannelList> create_channel_list();
        bool add_channel_list(std::shared_ptr<cChannelList> ptr);
        std::shared_ptr<cChannelList> channel_list() { return m_channel_list_ptr; }
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eActionOp_APMANAGER* m_action_op = nullptr;
        cChannelList *m_channel_list = nullptr;
        std::shared_ptr<cChannelList> m_channel_list_ptr = nullptr;
        bool m_channel_list_init = false;
        bool m_lock_allocation__ = false;
        int m_lock_order_counter__ = 0;
};

}; // close namespace: beerocks_message

#endif //_BEEROCKS/TLVF_BEEROCKS_MESSAGE_APMANAGER_H_
