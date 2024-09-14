# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from capi import tlv
from opts import debug
import time


class HigherLayerDataPayloadTrigger(PrplMeshBaseTest):

    def runTest(self):
        # Locate test participants
        try:
            agent = self.dev.DUT.agent_entity
            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        mac_gateway_hex = '0x' + controller.mac.replace(':', '')
        debug("mac_gateway_hex = " + mac_gateway_hex)
        payload = 199 * (mac_gateway_hex + " ") + mac_gateway_hex

        debug("Send Higher Layer Data message")
        # MCUT sends Higher Layer Data message to CTT Agent1 by providing:
        # Higher layer protocol = "0x00"
        # Higher layer payload = 200 concatenated copies of the ALID of the MCUT (1200 octets)
        mid = controller.dev_send_1905(agent.mac,
                                       self.ieee1905['eMessageType']['HIGHER_LAYER_DATA_MESSAGE'],
                                       tlv(self.ieee1905['eTlvTypeMap']['TLV_HIGHER_LAYER_DATA'],
                                           "{0x00 %s}" % payload))

        debug(
            "Confirming higher layer data message was received in one of the agent's radios")

        self.check_log(agent.radios[0], r"HIGHER_LAYER_DATA_MESSAGE", fail_on_mismatch=False)

        debug("Confirming matching protocol and payload length")
        self.check_log(agent.radios[0], r"Protocol: 0")
        self.check_log(agent.radios[0], r"Payload-Length: 0x4b0")

        debug("Confirming ACK message was received in the controller")
        time.sleep(1)

        self.check_cmdu_type_single("ACK", self.ieee1905['eMessageType']['ACK_MESSAGE'],
                                    agent.mac, controller.mac, mid)
