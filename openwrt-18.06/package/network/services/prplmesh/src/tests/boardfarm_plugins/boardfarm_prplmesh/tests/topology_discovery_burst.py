# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2022 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

import time
from boardfarm.exceptions import SkipTest
from opts import debug
from .prplmesh_base_test import PrplMeshBaseTest


class TopologyDiscoveryBurst(PrplMeshBaseTest):
    """Send Burst of Topology Discovery messages

    Devices used in test setup:
    AP1 - Agent1 [DUT]
    GW - Controller

    This test verifies that the handling of a burst of 214 discovery messages sent from different
    source macs is done quickly by the agent. This test imitates step 20 of the R3 4.7.10_ETH test.

    """

    def runTest(self):
        # Locate test participants
        try:
            controller = self.dev.lan.controller_entity
            agent = self.dev.DUT.agent_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        start = time.time()
        controller.cmd_reply(
            "CUSTOM_CMD,cmd,discovery_burst,base_mac,84:d3:2a:04:02:00,repeats,214"
        )

        # Check that the last topology discovery message (contains al_mac 84:d3:2a:04:02:d5)
        # received in the Agent side in less than 0.5 seconds.
        # Note that it is important that the timeout shall be extremely low on linux server/PC
        # environment. On real platform it shouldn't take more than 2 seconds.
        self.check_log(agent, "84:d3:2a:04:02:d5", timeout=0.5)
        end = time.time()
        debug(f"dt={end - start} seconds\n")

        # 60 seconds is the time defined by the standard to remove neighbors if no topology
        # discovery message has been received from it.
        # Sleep 65 seconds (+5 seconds as safety threshold) to promise that no topology
        # notifications will be sent during the proceeding tests.
        time.sleep(65)
