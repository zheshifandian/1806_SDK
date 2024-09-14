###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from capi import tlv
from opts import debug

import time


class CombinedInfraMetrics(PrplMeshBaseTest):
    """
        Devices used in test setup:
        STA1 - WIFI repeater
        STA2 - WIFI repeater
        AP1 - Agent1 [DUT]
        AP2 - Agent2 [LAN2]

        GW - Controller
    """

    def runTest(self):
        # Locate test participants
        try:
            sta1 = self.dev.wifi
            sta2 = self.get_device_by_name('wifi2')

            controller = self.dev.lan.controller_entity

            agent1 = self.dev.DUT.agent_entity
            agent2 = self.dev.lan2.agent_entity

            vap1 = agent1.radios[0].vaps[0]
            vap2 = agent2.radios[1].vaps[0]
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        self.device_reset_then_set_config()
        self.configure_ssids(['CombInfraMetrics-1'])

        sta1.wifi_connect(vap1)
        sta2.wifi_connect(vap2)

        # Set station link metrics
        # TODO make abstraction for this in Radio
        agent2.radios[1].send_bwl_event(
            "DATA STA-UPDATE-STATS {} rssi=-38,-39,-40,-41 snr=38,39,40,41 "
            "uplink=1000 downlink=800".format(sta2.mac))

        time.sleep(1)

        # Check on controller if stations have associated
        map_devices = self.get_topology()
        map_agent1 = map_devices[agent1.mac]
        map_radio1 = map_agent1.radios[agent1.radios[0].mac]
        map_vap1 = map_radio1.vaps[vap1.bssid]
        map_sta1 = map_vap1.clients[sta1.mac]
        debug("Found sta1 in topology: {}".format(map_sta1.path))
        map_agent2 = map_devices[agent2.mac]
        map_radio2 = map_agent2.radios[agent2.radios[1].mac]
        map_vap2 = map_radio2.vaps[vap2.bssid]
        map_sta2 = map_vap2.clients[sta2.mac]
        debug("Found sta2 in topology: {}".format(map_sta2.path))

        debug("Send AP Metrics query message to agent 1 expecting"
              "Traffic Stats for {}".format(sta1.mac))
        self.send_and_check_policy_config_metric_reporting(controller, agent1, True, False)
        mid = controller.dev_send_1905(agent1.mac,
                                       self.ieee1905['eMessageType']['AP_METRICS_QUERY_MESSAGE'],
                                       tlv(self.ieee1905['eTlvTypeMap']['TLV_AP_METRIC_QUERY'],
                                           "0x01 {%s}" % (vap1.bssid)))

        time.sleep(1)
        response = self.check_cmdu_type_single("AP metrics response",
                                               self.ieee1905['eMessageType']
                                               ['AP_METRICS_RESPONSE_MESSAGE'], agent1.mac,
                                               controller.mac, mid)
        debug("Check AP metrics response has AP metrics")
        ap_metrics_1 = self.check_cmdu_has_tlv_single(response,
                                                      self.ieee1905['eTlvTypeMap']['TLV_AP_METRIC'])
        if ap_metrics_1:
            if ap_metrics_1.ap_metrics_bssid != vap1.bssid:
                self.fail("AP metrics response with wrong BSSID {} instead of {}".format(
                    ap_metrics_1.ap_metrics_bssid, vap1.bssid))

        debug("Check AP metrics response has STA traffic stats")
        sta_stats_1 = self.check_cmdu_has_tlv_single(response,
                                                     self.ieee1905['eTlvTypeMap']
                                                     ['TLV_ASSOCIATED_STA_TRAFFIC_STATS'])
        if sta_stats_1:
            if sta_stats_1.assoc_sta_traffic_stats_mac_addr != sta1.mac:
                self.fail("STA traffic stats with wrong MAC {} instead of {}".format(
                    sta_stats_1.assoc_sta_traffic_stats_mac_addr, sta1.mac))

        debug("Send AP Metrics query message to agent 2 expecting"
              " STA Metrics for {}".format(sta2.mac))
        self.send_and_check_policy_config_metric_reporting(controller, agent2, False, True)
        mid = controller.dev_send_1905(agent2.mac,
                                       self.ieee1905['eMessageType']['AP_METRICS_QUERY_MESSAGE'],
                                       tlv(self.ieee1905['eTlvTypeMap']['TLV_AP_METRIC_QUERY'],
                                           "0x01 {%s}" % vap2.bssid))

        time.sleep(1)
        response = self.check_cmdu_type_single("AP metrics response",
                                               self.ieee1905['eMessageType']
                                               ['AP_METRICS_RESPONSE_MESSAGE'], agent2.mac,
                                               controller.mac, mid)
        debug("Check AP Metrics Response message has AP Metrics TLV")
        ap_metrics_2 = self.check_cmdu_has_tlv_single(response,
                                                      self.ieee1905['eTlvTypeMap']['TLV_AP_METRIC'])
        if ap_metrics_2:
            if ap_metrics_2.ap_metrics_bssid != vap2.bssid:
                self.fail("AP metrics response with wrong BSSID {} instead of {}".format(
                    ap_metrics_2.ap_metrics_bssid, vap2.bssid))

        debug("Check AP metrics response has STA Link Metrics")
        sta_metrics_2 = self.check_cmdu_has_tlv_single(response,
                                                       self.ieee1905['eTlvTypeMap']
                                                       ['TLV_ASSOCIATED_STA_LINK_METRICS'])
        if sta_metrics_2:
            if sta_metrics_2.assoc_sta_link_metrics_mac_addr != sta2.mac:
                self.fail("STA metrics with wrong MAC {} instead of {}".format(
                    sta_metrics_2.assoc_sta_link_metrics_mac_addr, sta2.mac))
            if len(sta_metrics_2.bss) != 1:
                self.fail("STA metrics with multiple BSSes: {}".format(sta_metrics_2.bss))
            elif sta_metrics_2.bss[0].bssid != vap2.bssid:
                self.fail("STA metrics with wrong BSSID {} instead of {}".format(
                    sta_metrics_2.bss[0].bssid, vap2.bssid))

        debug("Send 1905 Link metric query to agent 1 (neighbor STA)")
        mid = controller.dev_send_1905(agent1.mac,
                                       self.ieee1905['eMessageType']['LINK_METRIC_QUERY_MESSAGE'],
                                       tlv(self.ieee1905['eTlvType']['TLV_LINK_METRIC_QUERY'],
                                           "0x01 {%s} 0x02" % sta1.mac))
        time.sleep(1)
        response = self.check_cmdu_type_single("Link metrics response",
                                               self.ieee1905['eMessageType']
                                               ['LINK_METRIC_RESPONSE_MESSAGE'], agent1.mac,
                                               controller.mac, mid)
        # We requested specific neighbor, so only one transmitter and receiver link metrics TLV
        time.sleep(1)

        debug("Check link metrics response has transmitter link metrics")
        tx_metrics_1 = self.check_cmdu_has_tlv_single(response, 9)
        debug("Check link metrics response has receiver link metrics")
        rx_metrics_1 = self.check_cmdu_has_tlv_single(response, 10)

        # Trigger combined infra metrics
        debug("Send Combined infrastructure metrics message to agent 1")
        mid = controller.dev_send_1905(agent1.mac,
                                       self.ieee1905['eMessageType']
                                       ['COMBINED_INFRASTRUCTURE_METRICS_MESSAGE'])

        time.sleep(1)
        combined_infra_metrics = self.check_cmdu_type_single(
            "Combined infra metrics",
            self.ieee1905['eMessageType']
            ['COMBINED_INFRASTRUCTURE_METRICS_MESSAGE'],
            controller.mac, agent1.mac,
            mid)

        # Combined infra metrics should *not* contain STA stats/metrics
        expected_tlvs = filter(None, [ap_metrics_1, ap_metrics_2, tx_metrics_1, rx_metrics_1])
        self.check_cmdu_contains_tlvs(combined_infra_metrics, expected_tlvs)
        # TODO for now, just check that it has link metrics
        self.check_cmdu_has_tlv_single(response, 9)
        self.check_cmdu_has_tlv_single(response, 10)
        (combined_infra_metrics, expected_tlvs)  # Work around unused variable flake8 check

        self.check_cmdu_type_single("ACK", self.ieee1905['eMessageType']['ACK_MESSAGE'],
                                    agent1.mac, controller.mac, mid)

        sta1.wifi_disconnect(vap1)
        sta2.wifi_disconnect(vap2)
