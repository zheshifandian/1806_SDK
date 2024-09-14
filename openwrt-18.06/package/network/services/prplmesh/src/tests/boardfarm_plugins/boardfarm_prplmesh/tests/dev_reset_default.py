###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################
import time

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest


class DevResetDefault(PrplMeshBaseTest):
    """Test that dev_reset_default resets the agent (even when it's called
        multiple times).
    Check that no autoconfig search is sent while in reset.

    Devices used in test setup:
        - Agent1 [DUT]

    """

    def runTest(self):
        # Locate test participants
        try:
            agent = self.dev.DUT.agent_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        for attempt in range(1, 4):
            print("Resetting agent 1")
            self.checkpoint()
            agent.ucc_socket.cmd_reply("dev_reset_default,devrole,agent,program,map,type,DUT")

            time.sleep(2)
            self.check_no_cmdu_type("autoconfig search while in reset",
                                    self.ieee1905['eMessageType']
                                    ['AP_AUTOCONFIGURATION_SEARCH_MESSAGE'],
                                    agent.mac)

        print("Configuring agent 1")
        self.checkpoint()
        agent.ucc_socket.cmd_reply("dev_set_config,backhaul,eth")
        time.sleep(2)
        self.check_cmdu_type("autoconfig search",
                             self.ieee1905['eMessageType']['AP_AUTOCONFIGURATION_SEARCH_MESSAGE'],
                             agent.mac)

        # After dev_reset_default there is a delay between the auto_config message to the moment,
        # that the sockets to the son_slaves are open. Add a delay to make sure that the son_slaves
        # are operational before continuing to the next test.
        time.sleep(3)
