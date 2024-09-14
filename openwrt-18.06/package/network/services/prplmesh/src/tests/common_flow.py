###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

from boardfarm_plugins.boardfarm_prplmesh.tests import prplmesh_base_test
from capi import tlv
from opts import debug

import time


class CommonFlows(prplmesh_base_test.PrplMeshBaseTest):
    """
    Contains common methods used by other(derived) prplmesh test cases.
    """

    def send_and_check_policy_config_metric_reporting(self, controller,
                                                      agent, include_sta_traffic_stats=True,
                                                      include_sta_link_metrics=True):
        debug("Send multi-ap policy config request with metric reporting policy to agent")
        reporting_value = 0
        if include_sta_traffic_stats:
            reporting_value |= 0x80
        if include_sta_link_metrics:
            reporting_value |= 0x40
        radio_policies = ["{%s 0x00 0x00 0x01 0x%02x}" % (radio.mac, reporting_value)
                          for radio in agent.radios]
        metric_reporting_tlv = tlv(self.ieee1905['eTlvTypeMap']['TLV_METRIC_REPORTING_POLICY'],
                                   "{0x00 0x%02x %s}" % (len(radio_policies),
                                                         " ".join(radio_policies)))
        mid = controller.dev_send_1905(agent.mac, 0x8003, metric_reporting_tlv)
        time.sleep(1)
        debug("Confirming multi-ap policy config request was acked by agent")
        self.check_cmdu_type_single("ACK", 0x8000, agent.mac, controller.mac, mid)

    def configure_multi_ap_policy_config_with_unsuccessful_association(
            self, agent, controller, enable: 0x80, max_repeat: 0x0A):
        debug("Send multi-ap policy config request with unsuccessful association policy to agent 1")
        mid = controller.dev_send_1905(agent.mac, 0x8003,
                                       tlv(self.ieee1905['eTlvTypeMap']
                                           ['TLV_PROFILE2_UNSUCCESSFUL_ASSOCIATION_POLICY'],
                                           "{{0x{:02X} 0x{:08X}}}"
                                           .format(enable, max_repeat)))
        time.sleep(1)
        debug("Confirming multi-ap policy config with unsuccessful association"
              "request has been received on agent")

        self.check_cmdu_type("AP_POLICY_CONFIG_QUERY_MESSAGE", 0x8003,
                             controller.mac, agent.mac, mid)
        time.sleep(1)
        debug("Confirming multi-ap policy config ack message has been received on the controller")
        self.check_cmdu_type_single("ACK", 0x8000, agent.mac, controller.mac, mid)

    def mismatch_psk(self, agent_radio, controller, sta, expect='yes'):
        '''
        expect: yes / no / exceed
        '''

        # Simulate Mismatch PSK sent by STA

        # Get the agent from the radio
        agent = agent_radio.agent

        # Simulate Failed Association Message
        agent_radio.send_bwl_event(
            "EVENT AP-STA-POSSIBLE-PSK-MISMATCH {}".format(sta.mac))

        # Wait for something to happen
        time.sleep(1)

        # Check correct flow

        if expect == 'yes':
            # Validate "Failed Connection Message" CMDU was sent
            response = self.check_cmdu_type_single(
                "Failed Connection Message", 0x8033, agent.mac, controller.mac)

            debug("Check Failed Connection Message has valid STA TLV")
            tlv_sta_mac = self.check_cmdu_has_tlv_single(response, 0x95)
            if hasattr(tlv_sta_mac, 'sta_mac_addr_type_mac_addr'):
                received_sta_mac = tlv_sta_mac.sta_mac_addr_type_mac_addr
            else:
                received_sta_mac = '00:00:00:00:00:00'

            # Validate Srouce Info STA MAC
            if received_sta_mac != sta.mac:
                self.fail("Source Info TLV has wrong STA MAC {} instead of {}".format(
                    received_sta_mac, sta.mac))
        elif expect == 'no':
            debug("expecting no cmdu, policy set to no report")
            self.check_no_cmdu_type("Failed Connection Message", 0x8033,
                                    agent.mac, controller.mac)
        elif expect == 'exceed':
            debug("expecting no cmdu, exceeded number of reports in a minute")
            self.check_no_cmdu_type("Failed Connection Message", 0x8033,
                                    agent.mac, controller.mac)
        else:
            debug("unknown 'expect' = {}".format(expect))
