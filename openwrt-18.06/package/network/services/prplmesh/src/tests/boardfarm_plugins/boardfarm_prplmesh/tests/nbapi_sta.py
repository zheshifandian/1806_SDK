
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


class NbapiSta(PrplMeshBaseTest):
    '''
    This test checks various parameters of NBAPI sta object.
    Path to this object: Device.WiFi.DataElements.Network.Device.Radio.BSS.STA
    '''

    def runTest(self):
        try:
            agent = self.dev.DUT.agent_entity
            sta1 = self.dev.wifi
            controller = self.dev.lan.controller_entity
            vap1 = agent.radios[1].vaps[0]
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)
        self.configure_ssids_clear()
        self.configure_ssid("ImSSID24G", "Fronthaul", {"Band2_4G": True})
        self.configure_ssid("ImSSID5GH", "Fronthaul", {"Band5GH": True})
        controller.nbapi_command("Device.WiFi.DataElements.Network", "AccessPointCommit")

        time.sleep(5)

        time_before_sta_connect = datetime.now()
        time_before_sta_connect = pytz.utc.localize(time_before_sta_connect)

        sta1.wifi_connect(vap1)

        agent.radios[1].send_bwl_event(
            "DATA STA-UPDATE-STATS {} rssi=-38,-39,-40,-41 snr=38,39,40,41 "
            "uplink=1000 downlink=800".format(sta1.mac))

        time_before_query = datetime.now()
        time_before_query = pytz.utc.localize(time_before_query)

        debug('Send AP Metrics Query Message')
        mid1 = controller.dev_send_1905(agent.mac,
                                        self.ieee1905['eMessageType']['AP_METRICS_QUERY_MESSAGE'],
                                        tlv(self.ieee1905['eTlvTypeMap']['TLV_AP_METRIC_QUERY'],
                                            "0x01 {%s}" % (vap1.bssid)))
        debug("Send Associated STA Link Metrics Query message")
        controller.ucc_socket.dev_send_1905(agent.mac,
                                            self.ieee1905['eMessageType']
                                            ['ASSOCIATED_STA_LINK_METRICS_QUERY_MESSAGE'],
                                            tlv(self.ieee1905['eTlvTypeMap']
                                                ['TLV_STAMAC_ADDRESS_TYPE'],
                                                sta1.mac))
        time.sleep(5)

        ap_metrics_resp = self.check_cmdu_type_single("AP metrics response",
                                                      self.ieee1905['eMessageType']
                                                      ['AP_METRICS_RESPONSE_MESSAGE'], agent.mac,
                                                      controller.mac, mid1)

        debug("Check AP metrics response has STA traffic stats")
        traffic_stats = self.check_cmdu_has_tlv_single(ap_metrics_resp,
                                                       self.ieee1905['eTlvTypeMap']
                                                       ['TLV_ASSOCIATED_STA_TRAFFIC_STATS'])
        debug("Check AP metrics response has STA Link Metrics")
        sta_link_metrics = self.check_cmdu_has_tlv_single(ap_metrics_resp,
                                                          self.ieee1905['eTlvTypeMap']
                                                          ['TLV_ASSOCIATED_STA_LINK_METRICS'])

        print("\nNetwork topology after settings:")
        topology = self.get_topology()
        for device in topology.values():
            print(device)

        for radio in topology[agent.mac].radios.values():
            for bss in radio.vaps.values():
                for sta in bss.clients.values():
                    sta_mac = controller.nbapi_get_parameter(sta.path, "MACAddress")
                    time_stamp = controller.nbapi_get_parameter(sta.path, "TimeStamp")
                    signal_strength = controller.nbapi_get_parameter(sta.path, "SignalStrength")
                    time.sleep(3)  # This sleep needed by dhcp_task itself to register DHCP Leases.
                    ipv4_address = controller.nbapi_get_parameter(sta.path, "IPV4Address")
                    ipv6_address = controller.nbapi_get_parameter(sta.path, "IPV6Address")
                    hostname = controller.nbapi_get_parameter(sta.path, "Hostname")

                    assert sta_mac == sta1.mac, f"Wrong sta mac {sta_mac} expect {sta1.mac}"
                    self.assertEqual(sta.path, "BytesSent",
                                     traffic_stats.assoc_sta_traffic_stats_bytes_sent)
                    self.assertEqual(sta.path, "BytesReceived",
                                     traffic_stats.assoc_sta_traffic_stats_bytes_rcvd)
                    self.assertEqual(sta.path, "PacketsSent",
                                     traffic_stats.assoc_sta_traffic_stats_packets_sent)
                    self.assertEqual(sta.path, "PacketsReceived",
                                     traffic_stats.assoc_sta_traffic_stats_packets_rcvd)
                    self.assertEqual(sta.path, "ErrorsReceived",
                                     traffic_stats.assoc_sta_traffic_stats_rx_packet_errs)
                    self.assertEqual(sta.path, "ErrorsSent",
                                     traffic_stats.assoc_sta_traffic_stats_tx_pkt_errs)
                    self.assertEqual(sta.path, "RetransCount",
                                     traffic_stats.assoc_sta_traffic_stats_retrans_count)
                    self.assertEqual(sta.path, "EstMACDataRateDownlink",
                                     sta_link_metrics.bss[0].down_rate)
                    self.assertEqual(sta.path, "EstMACDataRateUplink",
                                     sta_link_metrics.bss[0].up_rate)
                    assert signal_strength == int(sta_link_metrics.bss[0].rssi),\
                        f"Wrong value for SignalStrength {signal_strength}"
                    assert sta_link_metrics.bss[0].bssid == bss.bssid,\
                        f"Wrong BSSID [{bss.bssid}] specified for sta {sta.mac}"
                    assert time_stamp != 0, "Value for TimeStamp is not specified."

                    self.get_nbapi_ht_capabilities(sta.path)
                    self.get_nbapi_vht_capabilities(sta.path)

                    time_sta = dateutil.parser.isoparse(time_stamp)

                    # Verify Station connection time
                    if time_sta <= time_before_sta_connect:
                        self.fail('Fail. Sta connection time earlier '
                                  'than test triggering.')

                    # AP Metrics Query updates STA timestamp.
                    # Time between query and STA should be close.
                    if time_before_query + timedelta(seconds=2) <= time_sta:
                        self.fail('Fail. Sta time group was collected '
                                  'more than 10s after AP Metrics Query was send.')
                    print(
                        f'Sta group was collected at: [{time_sta}], '
                        f'time before query: [{time_before_query}],'
                        f'time before connection: {time_before_sta_connect}')

                    check_time_format = re.match(
                        r'\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}.\d+(Z|[-+]\d{2}:\d{2})',
                        time_stamp)
                    if check_time_format is None:
                        self.fail(f'Fail. NBAPI time stamp has inncorrect format: {time_stamp}')

                    assert hostname == sta1.hostname, \
                        f"Wrong hostname {hostname} expect {sta1.hostname}"
                    assert ipv4_address == sta1.ipv4, \
                        f"Wrong ipv4_address {ipv4_address} expect {sta1.ipv4}"
                    assert ipv6_address == sta1.ipv6, \
                        f"Wrong ipv6_address {ipv6_address} expect {sta1.ipv6}"

        sta1.wifi_disconnect(vap1)
