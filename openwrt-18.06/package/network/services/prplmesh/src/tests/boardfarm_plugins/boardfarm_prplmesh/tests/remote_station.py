# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from opts import debug


class RemoteStation(PrplMeshBaseTest):
    """Check initial configuration on device."""

    def runTest(self):
        try:
            agent = self.dev.DUT.agent_entity
            remote_sta = self.get_device_by_name('remote_station')

        except AttributeError as ae:
            raise SkipTest(ae)

        self.configure_passphrase()

        result = remote_sta.wifi_connect_check(agent.radios[0].vaps[0])

        if not result:
            self.fail(f'Connection status: {result}')

        tp = remote_sta.iperf_throughput(to_dut=True, protocol='tcp')

        debug('throughtput result - ' + str(tp))
        if not tp:
            self.fail("Throughput test from boardfarm host to DUT failed: no results available.")
