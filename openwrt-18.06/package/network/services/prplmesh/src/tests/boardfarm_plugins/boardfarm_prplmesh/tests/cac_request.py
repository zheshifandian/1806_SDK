# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from capi import tlv
#  from opts import debug
import time


class CacRequest(PrplMeshBaseTest):

    def runTest(self):
        # Locate test participants
        try:
            agent = self.dev.DUT.agent_entity
            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        number_of_radios = 1
        operating_class = 121
        channel = 104
        cac_request_tlv = tlv(
            self.ieee1905['eTlvTypeMap']['TLV_PROFILE2_CAC_REQUEST'],
            '0x{:02x} {} 0x{:01x} 0x{:01x} 0x00'
            .format(number_of_radios, agent.radios[1].mac, operating_class, channel))

        print("CAC Request TLV: ", cac_request_tlv)

        time.sleep(30)
        print("sleep for 30 seconds...")

        req_mid = controller.dev_send_1905(
            agent.mac, self.ieee1905['eMessageType']['CAC_REQUEST_MESSAGE'], cac_request_tlv)
        time.sleep(1)

        self.check_cmdu_type_single("ACK", self.ieee1905['eMessageType']['ACK_MESSAGE'],
                                    agent.mac, controller.mac, req_mid)

        time.sleep(2)

        cac_termination_tlv = tlv(
            self.ieee1905['eTlvTypeMap']['TLV_PROFILE2_CAC_TERMINATION'],
            '0x{:02x} {} 0x{:01x} 0x{:01x} 0x00'
            .format(number_of_radios, agent.radios[1].mac, operating_class, channel))

        print("CAC Termination TLV: ", cac_termination_tlv)

        req_mid = controller.dev_send_1905(agent.mac,
                                           self.ieee1905['eMessageType']['CAC_TERMINATION_MESSAGE'],
                                           cac_termination_tlv)
        time.sleep(1)

        self.check_cmdu_type_single("ACK", self.ieee1905['eMessageType']['ACK_MESSAGE'],
                                    agent.mac, controller.mac, req_mid)

        # TODO
        # verify that the agent sends beacons
