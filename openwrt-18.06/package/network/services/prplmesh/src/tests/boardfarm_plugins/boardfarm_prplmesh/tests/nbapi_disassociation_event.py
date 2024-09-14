
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from capi import tlv
from opts import debug

import time
from datetime import datetime, timedelta
import dateutil.parser
import pytz
import re


class NbapiDisassociationEvent(PrplMeshBaseTest):
    '''
    Test for NBAPI Device.WiFi.DataElements.Network.Device.Radio.DisassociationEvent object.
    This object describes an event generated when a STA disassociates from a BSS.
    Also, test Network object.
    '''

    def runTest(self):
        try:
            controller = self.dev.lan.controller_entity
            agent = self.dev.DUT.agent_entity
            vap = agent.radios[0].vaps[0]
            sta = self.dev.wifi
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)
        self.configure_ssids(["NbapiDisassociationEvent"])
        time.sleep(3)

        sta.wifi_connect(vap)
        self.check_log(
            controller, f"client connected, client_mac={sta.mac}, bssid={vap.bssid}", timeout=10)

        debug(f"Topology map after client [{sta.mac}] connected to [{vap.bssid}].")
        topology = self.get_topology()
        for device in topology.values():
            print(device)

        debug('Send AP Metrics Query Message')
        mid = controller.dev_send_1905(agent.mac,
                                       self.ieee1905['eMessageType']['AP_METRICS_QUERY_MESSAGE'],
                                       tlv(self.ieee1905['eTlvTypeMap']['TLV_AP_METRIC_QUERY'],
                                           "0x01 {%s}" % (vap.bssid)))
        time.sleep(2)
        ap_metrics_resp = self.check_cmdu_type_single("AP metrics response",
                                                      self.ieee1905['eMessageType']
                                                      ['AP_METRICS_RESPONSE_MESSAGE'], agent.mac,
                                                      controller.mac, mid)
        debug("Check AP metrics response has STA traffic stats")
        traffic_stats = self.check_cmdu_has_tlv_single(ap_metrics_resp,
                                                       self.ieee1905['eTlvTypeMap']
                                                       ['TLV_ASSOCIATED_STA_TRAFFIC_STATS'])

        debug(f"Client [{sta.mac}] disconnected from [{vap.bssid}].")
        sta.wifi_disconnect(vap)
        time_disconnect = datetime.now()
        time_disconnect = pytz.utc.localize(time_disconnect)
        time.sleep(2)
        self.check_log(
            controller, f"client disconnected, client_mac={sta.mac}, bssid={vap.bssid}", timeout=5)

        # Test for NBAPI Network.
        network_time_stamp = controller.nbapi_get_parameter(
            "Device.WiFi.DataElements.Network", "TimeStamp")
        controller_id = controller.nbapi_get_parameter(
            "Device.WiFi.DataElements.Network", "ControllerID")
        network_id = controller.nbapi_get_parameter("Device.WiFi.DataElements.Network", "ID")
        assert controller_id == controller.mac, \
            "Wrong controller mac: {controller_id}, expected: {controller.mac}."
        assert network_id == controller.mac, \
            "Wrong network mac: {network_id}, expected: {controller.mac}."

        expected_time_format = r'\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}.\d+(Z|[-+]\d{2}:\d{2})'
        if re.match(expected_time_format, network_time_stamp) is None:
            self.fail(f'Fail. Network time stamp has incorrect format: {network_time_stamp}')

        # Test for NBAPI DisassociationEventData.
        disassoc_data_path = "Device.WiFi.DataElements."\
            + "DisassociationEvent.DisassociationEventData"
        disassoc_data_paths_list = controller.nbapi_get_list_instances(disassoc_data_path)
        events_count = 0
        for disassoc_data_path in disassoc_data_paths_list:
            time_stamp = controller.nbapi_get_parameter(disassoc_data_path, "TimeStamp")
            time_nbapi = dateutil.parser.isoparse(time_stamp)
            # Check only last added DisassociationEvents
            if time_nbapi - timedelta(seconds=3) <= time_disconnect and\
                    time_disconnect <= time_nbapi + timedelta(seconds=3):

                self.assertEqual(disassoc_data_path, "BytesSent",
                                 traffic_stats.assoc_sta_traffic_stats_bytes_sent)
                self.assertEqual(disassoc_data_path, "BytesReceived",
                                 traffic_stats.assoc_sta_traffic_stats_bytes_rcvd)
                self.assertEqual(disassoc_data_path, "PacketsSent",
                                 traffic_stats.assoc_sta_traffic_stats_packets_sent)
                self.assertEqual(disassoc_data_path, "PacketsReceived",
                                 traffic_stats.assoc_sta_traffic_stats_packets_rcvd)
                self.assertEqual(disassoc_data_path, "ErrorsReceived",
                                 traffic_stats.assoc_sta_traffic_stats_rx_packet_errs)
                self.assertEqual(disassoc_data_path, "ErrorsSent",
                                 traffic_stats.assoc_sta_traffic_stats_tx_pkt_errs)
                self.assertEqual(disassoc_data_path, "RetransCount",
                                 traffic_stats.assoc_sta_traffic_stats_retrans_count)
                # Need to re-check after PPM-1905
                self.assertEqual(disassoc_data_path, "ReasonCode", 0)

                bssid = controller.nbapi_get_parameter(disassoc_data_path, "BSSID")
                sta_mac = controller.nbapi_get_parameter(disassoc_data_path, "MACAddress")

                assert bssid == vap.bssid, f"Wrong value for BSSID {bssid}, expect {vap.bssid}"
                assert sta_mac == sta.mac, f"Wrong value for MACAddress {sta_mac}, expect {sta.mac}"
                events_count += 1

        # TO DO: PPM-1272
        assert 0 < events_count <= 2, \
            f"Wrong amount [{events_count}] of DisassociationEvents for one client disassociation."

        if re.match(expected_time_format, time_stamp) is None:
            self.fail(f'Fail. DisassociationEvent time stamp has incorrect format: {time_stamp}')
