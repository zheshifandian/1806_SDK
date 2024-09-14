# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from opts import debug

import time


class Topology(PrplMeshBaseTest):

    def runTest(self):
        # Locate test participants
        try:
            agent = self.dev.DUT.agent_entity
            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        # Agent Topology updated when Controller gets Topology Discovery
        # (which is sent every 60 sec.)
        time.sleep(61)

        # Send Topology Query
        controller.dev_send_1905(agent.mac,
                                 self.ieee1905['eMessageType']['TOPOLOGY_QUERY_MESSAGE'])
        time.sleep(1)

        debug("\nAfter Topology Query Message:\nCurrent network topology:")
        topology = self.get_topology()
        for value in topology.values():
            debug(value)

        debug("Confirming Controller MAC appears on Agent Interface Neighbors")
        found = False
        map_agent = topology[agent.mac]
        for map_interface in map_agent.interfaces.values():
            for map_neighbor in map_interface.neighbors:
                if map_neighbor == controller.mac:
                    found = True

        assert found, \
            "Controller MAC " + controller.mac + "is not found in Neighbor List"
