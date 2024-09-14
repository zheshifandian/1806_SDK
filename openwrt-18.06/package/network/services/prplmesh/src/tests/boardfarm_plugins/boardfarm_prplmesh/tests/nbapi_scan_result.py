# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from opts import debug
from capi import tlv
import sniffer

import time

# TO DO: Add this test to testsuites after PPM-491 merged.
# Add only supported channels to Channel Scan Request TLV


class NbapiScanResult(PrplMeshBaseTest):
    '''
        Test for NBAPI ScanResult object.
        This object describes the list of neighboring APs discovered by a radio
        organized per OpClass and Channel tuple.
    '''

    def check_neighbors(self, neighbor_paths, neighbors_num, pos, ch_scan_tlv: sniffer.TlvStruct):
        controller = self.dev.lan.controller_entity
        for x in range(neighbors_num):
            # The tshark version 2.6.20 used in boardfarm
            # doesn't properly parse Channel Scan Result TLV.
            bssid = ch_scan_tlv.tlv_data[pos: pos + 17]
            ssid_len = int(ch_scan_tlv.tlv_data[pos + 18: pos + 20], 16)
            pos += 21
            ssid = ch_scan_tlv.tlv_data[pos: pos + ssid_len]
            pos += ssid_len + 1
            ch_bandwidth_len = int(ch_scan_tlv.tlv_data[pos: pos + 2], 16)
            ch_bandwidth = ch_scan_tlv.tlv_data[pos + 3: pos + 3 + ch_bandwidth_len]
            pos += ch_bandwidth_len + 4
            int(ch_scan_tlv.tlv_data[pos: pos + 2], 16)  # BSS Load Element Present
            utilization = int(ch_scan_tlv.tlv_data[pos + 3: pos + 5], 16)
            sta_count = int(ch_scan_tlv.tlv_data[pos + 6: pos + 12], 16)
            pos += 13

            # Search for corresponding NBAPI NeighborBSS object.
            neighbor = [neighbor for neighbor in neighbor_paths
                        if bssid == controller.nbapi_get_parameter(neighbor, "BSSID")]

            assert len(neighbor) == 1, \
                f"Wrong amount of matching NBAPI NeighborBSS: {neighbor}."

            nbapi_ch_bandwidth = controller.nbapi_get_parameter(
                neighbor[0], "ChannelBandwidth")
            nbapi_ssid = controller.nbapi_get_parameter(neighbor[0], "SSID")

            assert nbapi_ch_bandwidth == ch_bandwidth, \
                f"Wrong value for ChannelBandwidth "\
                f"expected {ch_bandwidth}, actual: {nbapi_ch_bandwidth}."
            assert nbapi_ssid == ssid, \
                f"Wrong value for SSID expected {ssid}, actual: {nbapi_ssid}."

            self.assertEqual(neighbor[0], "ChannelUtilization", utilization)
            self.assertEqual(neighbor[0], "StationCount", sta_count)

    def check_op_class(self, ch_scan_tlv: sniffer.TlvStruct, nbapi_channel_paths: str):
        controller = self.dev.lan.controller_entity

        # The tshark version 2.6.20 used in boardfarm
        # doesn't properly parse Channel Scan Result TLV.
        assert 26 <= len(
            ch_scan_tlv.tlv_data), "Too small content in Channel Scan TLV."
        ch_scan_tlv.tlv_data[0:17]  # ruid
        channel = int(ch_scan_tlv.tlv_data[21:23], 16)
        scan_status = int(ch_scan_tlv.tlv_data[24:26], 16)

        # Search for corresponding NBAPI ChannelScan object. ch is
        # Device.WiFi.DataElements.Network.Device.Radio.ScanResult.OpClassScan.{i}.ChannelScan.{i}
        matching_channel = [ch for ch in nbapi_channel_paths
                            if channel == int(controller.nbapi_get_parameter(ch, "Channel"))]

        assert len(matching_channel) == 1, \
            f"Wrong amount of NBAPI matching channels: {len(matching_channel)}."

        if scan_status != 0:  # Not successful scan followed fiels omited.
            return

        assert 56 <= len(
            ch_scan_tlv.tlv_data), "Too small content in Channel Scan TLV."

        ts_len = int(ch_scan_tlv.tlv_data[27:29], 16)
        ch_scan_tlv.tlv_data[30:30 + ts_len]  # timestamp
        ts_len += 31
        utilization = int(ch_scan_tlv.tlv_data[ts_len: ts_len + 3], 16)
        noise = int(ch_scan_tlv.tlv_data[ts_len + 4: ts_len + 6], 16)
        neighbors_num = int(ch_scan_tlv.tlv_data[ts_len + 7: ts_len + 11], 16)

        self.assertEqual(matching_channel[0] + ".NeighborBSS",
                         "NeighborNumberOfEntries", neighbors_num)
        self.assertEqual(matching_channel[0], "Utilization", utilization)
        self.assertEqual(matching_channel[0], "Noise", noise)

        neighbor_paths = controller.nbapi_get_list_instances(
            matching_channel + ".NeighborBSS")
        pos = ts_len + 12
        self.check_neighbors(neighbor_paths, neighbors_num, pos)

    def runTest(self):
        try:
            agent = self.dev.DUT.agent_entity
            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        channel_scan_request_tlv = (
            "0x3 "
            "{0x73 0x03 {0x28 0x2C 0x30} 0x00} "
            "{0x74 0x01 {0x2C} 0x00} "
            "{0x80 0x05 {0x3A 0x6A 0x7A 0x8A 0x9B} 0x00} "
        )

        debug("Send Channel Scan Request message")
        controller.ucc_socket.dev_send_1905(agent.mac, self.ieee1905['eMessageType']
                                            ['CHANNEL_SCAN_REQUEST_MESSAGE'],
                                            tlv(self.ieee1905['eTlvTypeMap']
                                                ['TLV_CHANNEL_SCAN_REQUEST'],
                                                "0x80 0x01 {} {}".format(
                                                    agent.radios[0].mac, channel_scan_request_tlv)))
        time.sleep(5)
        self.check_log(
            agent, "Sending Channel Scan Report Message", timeout=10)

        ch_scan_reports = self.check_cmdu_type(
            "Channel Scan Report", self.ieee1905['eMessageType']['CHANNEL_SCAN_REPORT_MESSAGE'],
            agent.mac, controller.mac)

        topology = self.get_topology()
        repeater = topology[agent.mac]
        radio = repeater.radios[agent.radios[0].mac]

        # Example of path: Device.WiFi.DataElements.Network.Device.1.Radio.2.ScanResult
        nbapi_scans_paths = controller.nbapi_get_list_instances(
            radio.path + ".ScanResult")

        controller.nbapi_get_parameter(nbapi_scans_paths[-1], "TimeStamp")
        nbapi_op_class_paths = controller.nbapi_get_list_instances(
            nbapi_scans_paths[-1] + ".OpClassScan")

        for nbapi_scan in nbapi_op_class_paths:
            found = False
            nbapi_class = controller.nbapi_get_parameter(nbapi_scan, "OperatingClass")
            nbapi_channel_paths = controller.nbapi_get_list_instances(
                nbapi_scan + ".ChannelScan")
            for report in ch_scan_reports:
                scan_tlvs = self.check_cmdu_has_tlvs(report,
                                                     self.ieee1905['eTlvTypeMap']
                                                     ['TLV_CHANNEL_SCAN_RESULT'])

                matching_tlvs = [scan_tlv for scan_tlv in scan_tlvs
                                 if int(scan_tlv.tlv_data[18:20], 16) == int(nbapi_class)]
                if len(matching_tlvs):
                    found = True
                    for ch_scan_tlv in matching_tlvs:
                        self.check_op_class(ch_scan_tlv, nbapi_channel_paths)
            assert found, \
                f"NBAPI class: [{nbapi_class}] not found in Channel Scan Result TLV."
