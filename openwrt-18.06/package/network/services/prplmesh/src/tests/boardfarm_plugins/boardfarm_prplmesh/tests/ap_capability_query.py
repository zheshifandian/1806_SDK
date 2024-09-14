# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from opts import debug
import time


class ApCapabilityQuery(PrplMeshBaseTest):

    def runTest(self):
        # Locate test participants
        try:
            agent = self.dev.DUT.agent_entity
            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        controller.dev_send_1905(
            agent.mac, self.ieee1905['eMessageType']['AP_CAPABILITY_QUERY_MESSAGE'])
        time.sleep(1)

        debug("Confirming ap capability query has been received on agent")
        self.check_log(agent, "AP_CAPABILITY_QUERY_MESSAGE")

        debug("Confirming ap capability report has been received on controller")
        self.check_log(controller, "AP_CAPABILITY_REPORT_MESSAGE")
