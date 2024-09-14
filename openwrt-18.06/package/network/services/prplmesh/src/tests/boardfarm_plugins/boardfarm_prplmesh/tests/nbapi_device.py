
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from opts import debug
from capi import tlv

import time


class NbapiDevice(PrplMeshBaseTest):
    '''
    Test for NBAPI device object.
    '''

    def runTest(self):
        try:
            agent = self.dev.DUT.agent_entity
            agent2 = self.dev.lan2.agent_entity
            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        sniffer = self.dev.DUT.wired_sniffer
        sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        # Send link metric query.
        mid1 = controller.dev_send_1905(agent.mac,
                                        self.ieee1905['eMessageType']['LINK_METRIC_QUERY_MESSAGE'],
                                        tlv(self.ieee1905['eTlvType']['TLV_LINK_METRIC_QUERY'],
                                            "0x00 0x02"))
        mid2 = controller.dev_send_1905(agent2.mac,
                                        self.ieee1905['eMessageType']['LINK_METRIC_QUERY_MESSAGE'],
                                        tlv(self.ieee1905['eTlvType']['TLV_LINK_METRIC_QUERY'],
                                            "0x00 0x02"))
        time.sleep(3)
        self.check_cmdu_type_single("link metric query controller-agent1",
                                    self.ieee1905['eMessageType']['LINK_METRIC_QUERY_MESSAGE'],
                                    controller.mac, agent.mac, mid1)
        self.check_cmdu_type_single("link metric query controller-agent2",
                                    self.ieee1905['eMessageType']['LINK_METRIC_QUERY_MESSAGE'],
                                    controller.mac, agent2.mac, mid2)
        members_ids = {controller.mac, agent.mac, agent2.mac}
        topology = self.get_topology()
        for device in topology.values():
            packets_found = False
            data_model_id = controller.nbapi_get_parameter(device.path, "ID")
            if data_model_id in members_ids:
                members_ids.remove(data_model_id)
            else:
                debug(f"Unknown participants with id [{data_model_id}].")
                continue
            for interface in device.interfaces.values():
                status = controller.nbapi_get_parameter(interface.path, "Status")
                mac = controller.nbapi_get_parameter(interface.path, "MACAddress")
                assert status == "Up", f"Interface {mac} is {status}"
                media_type = controller.nbapi_get_parameter(interface.path, "MediaType")
                assert 0 < int(media_type), f"Interface {mac} media type is {media_type} ."
                stats_path = interface.path + ".Stats"
                bytes_sent = controller.nbapi_get_parameter(stats_path, "BytesSent")
                bytes_received = controller.nbapi_get_parameter(stats_path, "BytesReceived")
                packets_sent = controller.nbapi_get_parameter(stats_path, "PacketsSent")
                packets_received = controller.nbapi_get_parameter(stats_path, "PacketsReceived")
                unicast_packets_received = controller.nbapi_get_parameter(
                    stats_path, "UnicastPacketsReceived")
                # We need to test if the packets counters on the interfaces are correct.
                # However, it's very difficult to say what exactly is "correct".
                # Therefore, we'll simply test that they are not 0.
                # However, some interfaces are not actually active so the counters *will*
                # be zero. Therefore, we just test that there's at least one interface
                # where the counters are non-zero.
                if packets_sent or packets_received:
                    packets_found = True
                    print(f"Packets found for interface {mac}, stats:\n "
                          f"BytesSent: {bytes_sent},\n "
                          f"BytesReceived: {bytes_received},\n "
                          f"UnicastPacketsReceived {unicast_packets_received},\n "
                          f"PacketsReceived: {packets_received},\n "
                          f"PacketsSent: {packets_sent}")
            # On the dummy controller, all packet counts *will* be zero, so ignore controller
            if device.mac == controller.mac:
                continue
            if packets_found is False:
                self.fail(f"Parameter packets sent and (or) packets received does"
                          f" not increase for device {device.mac}.")
