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


class BeaconReportQueryAndResponse(PrplMeshBaseTest):
    ''' This test verifies that a MAUT with an associated STA responds
    to a Beacon Metrics Query by sending a Beacon Report request to its associated STA,
    receiving a response from the STA, and sending the contents of that response
    in a Beacon Metrics Response message to the Controller '''

    def runTest(self):
        # Locate test participants
        try:
            sta = self.dev.wifi
            agent = self.dev.DUT.agent_entity
            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        sniffer = self.dev.DUT.wired_sniffer
        sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        # Step 3. MAUT sends Association Response frame to STA
        sta.wifi_connect_check(agent.radios[0].vaps[0])
        time.sleep(1)
        debug("Send Associated STA Link Metrics Query message")
        mid = controller.ucc_socket.dev_send_1905(
            agent.mac, self.ieee1905['eMessageType']['ASSOCIATED_STA_LINK_METRICS_QUERY_MESSAGE'],
            tlv(self.ieee1905['eTlvTypeMap']['TLV_STAMAC_ADDRESS_TYPE'], sta.mac))
        time.sleep(5)
        debug("STA sends a valid Association Request frame to MAUT")
        self.check_log(agent,
                       "Send AssociatedStaLinkMetrics to controller, mid = {}".format(mid),
                       timeout=20)
        self.check_cmdu_type_single("Associated STA Link Metrics Response", 0x800E,
                                    agent.mac, controller.mac, mid)

        # Step 4. Send Beacon Metrics Query to agent.
        agent.radios[0].send_bwl_event(
            "DATA RRM-BEACON-REP-RECEIVED {} channel=1 dialog_token=0 measurement_rep_mode=0 \
            op_class=0 duration=50 rcpi=-80 rsni=10 bssid=aa: bb:cc:11:00:10".format(sta.mac))

        '''- Operating Class field equal to 115
        - Channel Number field equal to 255
        - BSSID field equal to wildcard (0xFFFFFFFFFFFF)
        - Reporting Detail equal to 2
        - SSID length field equal to 0 (SSID field missing)
        - Number of AP Channel Reports equal to 1
        - Length of AP Channel Report equal to 0x03
        - Operating Class in AP Channel Report equal to 115
        - Channel List in AP Channel Report equal to 36 and 48 '''
        beacon_query_tlv_val = "{sta_mac} ".format(sta_mac=sta.mac)
        beacon_query_tlv_val += "{0x73 0xFF 0xFFFFFFFFFFFF 0x02 0x00 0x01 0x03 0x73 0x24 0x30}"

        debug("Send Beacon Metrics Query from controller to agent.")
        mid = controller.ucc_socket.dev_send_1905(
            agent.mac, self.ieee1905['eMessageType']['BEACON_METRICS_QUERY_MESSAGE'],
            tlv(self.ieee1905['eTlvTypeMap']['TLV_BEACON_METRICS_QUERY'],
                beacon_query_tlv_val))

        # Step 5. Verify that MAUT sends a 1905 ACK to Controller.
        time.sleep(1)
        self.check_cmdu_type_single(
            "ACK", self.ieee1905['eMessageType']['ACK_MESSAGE'], agent.mac, controller.mac, mid)
        debug("Confirming ACK message was received.")

        # Step 6. Verify that MAUT sends a correct Beacon request to STA.
        time.sleep(1)
        self.check_log(agent.radios[0], r"BEACON_METRICS_QUERY")
        debug("Confirming that MAUT sends a Beacon request to STA.")

        # Step 7. STA responds with Beacon report
        time.sleep(1)
        self.check_log(controller, r"got beacon response from STA. mid:", timeout=10)

        # Step 8. MAUT sends Beacon Metrics Response to Controller
        beacon_resp = self.check_cmdu_type_single(
            "Agent send Beacon Response to controller.",
            self.ieee1905['eMessageType']['BEACON_METRICS_RESPONSE_MESSAGE'],
            agent.mac, controller.mac)
        debug("Confirming MAUT sends Beacon Metrics Response to Controller.")

        beacon_resp_tlv = self.check_cmdu_has_tlv_single(beacon_resp, 154)
        ''' Don't check Beacon Metrics measurement report, as it's always empty
            https://jira.prplfoundation.org/browse/PPM-52 '''
        assert beacon_resp_tlv.beacon_metrics_mac_addr == sta.mac, \
            "Wrong beacon metrics mac addr in Beacon Respond"

        sta.wifi_disconnect(agent.radios[0].vaps[0])
