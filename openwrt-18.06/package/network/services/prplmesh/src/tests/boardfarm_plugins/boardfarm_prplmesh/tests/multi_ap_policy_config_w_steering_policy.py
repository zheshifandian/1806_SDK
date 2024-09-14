# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from capi import tlv
from opts import debug

import time


class MultiApPolicyConfigWSteeringPolicy(PrplMeshBaseTest):
    """
        Devices used in test setup:
        AP1 - Agent1 [DUT]

        GW - Controller
    """

    def runTest(self):
        # Locate test participants
        try:
            agent = self.dev.DUT.agent_entity
            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        debug("Send multi-ap policy config request with steering policy to agent 1")
        mid = controller.dev_send_1905(agent.mac, self.ieee1905['eMessageType']
                                        ['MULTI_AP_POLICY_CONFIG_REQUEST_MESSAGE'],
                                        tlv(self.ieee1905['eTlvTypeMap']
                                        ['TLV_STEERING_POLICY'],
                                        "{0x00 0x00 0x01 {%s 0x01 0xFF 0x14}}" % agent.radios[0].mac))  # noqa E501
        time.sleep(1)
        debug("Confirming multi-ap policy config request has been received on agent")

        self.check_cmdu_type("MULTI_AP_POLICY_CONFIG_REQUEST_MESSAGE",
                             self.ieee1905['eMessageType']
                             ['MULTI_AP_POLICY_CONFIG_REQUEST_MESSAGE'],
                             controller.mac, agent.mac, mid)

        time.sleep(1)
        debug("Confirming multi-ap policy config ack message has been received on the controller")
        self.check_cmdu_type_single(
            "ACK", self.ieee1905['eMessageType']['ACK_MESSAGE'], agent.mac, controller.mac, mid)
