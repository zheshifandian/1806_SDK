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


class LinkMetrics(PrplMeshBaseTest):
    '''
        Checks Link Metric Response CMDU.

        Devices used in test setup:
        STA1 - WIFI repeater
        STA2 - WIFI repeater
        AP1 - Agent1 [DUT]
        AP2 - Agent2 [LAN2]

        GW - Controller
    '''

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
        self.configure_ssids(['LinkMetrics-1'])

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

        debug("Send 1905 Link metric query to agent 1 (neighbor STA)")
        mid = controller.dev_send_1905(agent1.mac,
                                       self.ieee1905['eMessageType']['LINK_METRIC_QUERY_MESSAGE'],
                                       tlv(self.ieee1905['eTlvType']['TLV_LINK_METRIC_QUERY'],
                                           "0x01 {%s} 0x02" % sta1.mac))
        time.sleep(1)
        response = self.check_cmdu_type_single("Link metrics response",
                                               self.ieee1905['eMessageType']
                                               ['LINK_METRIC_RESPONSE_MESSAGE'],
                                               agent1.mac, controller.mac, mid)
        # We requested specific neighbor, so only one transmitter and receiver link metrics TLV
        time.sleep(1)

        debug("Check link metrics response has transmitter link metrics")
        self.check_cmdu_has_tlv_single(response, 9)
        debug("Check link metrics response has receiver link metrics")
        self.check_cmdu_has_tlv_single(response, 10)

        sta1.wifi_disconnect(vap1)
        sta2.wifi_disconnect(vap2)
