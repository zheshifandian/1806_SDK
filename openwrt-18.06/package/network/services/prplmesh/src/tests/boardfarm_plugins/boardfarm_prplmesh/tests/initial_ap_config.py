# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest


class InitialApConfig(PrplMeshBaseTest):
    """Check initial configuration on device."""

    def runTest(self):
        # Locate test participants
        agent = self.dev.DUT.agent_entity

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        agent.ucc_socket.cmd_reply("dev_reset_default,devrole,agent,program,map,type,DUT",
                                   timeout=60)
        agent.ucc_socket.cmd_reply("dev_set_config,backhaul,eth")

        self.prplmesh_status_check(agent)
        self.check_log(agent.radios[0],
                       r"\(WSC M2 Encrypted Settings\)", timeout=30)
        self.check_log(agent.radios[1],
                       r"\(WSC M2 Encrypted Settings\)")
        self.check_log(agent.radios[0],
                       r"WSC Global authentication success")
        self.check_log(agent.radios[1],
                       r"WSC Global authentication success")
        self.check_log(agent.radios[0],
                       r"KWA \(Key Wrap Auth\) success")
        self.check_log(agent.radios[1],
                       r"KWA \(Key Wrap Auth\) success")
