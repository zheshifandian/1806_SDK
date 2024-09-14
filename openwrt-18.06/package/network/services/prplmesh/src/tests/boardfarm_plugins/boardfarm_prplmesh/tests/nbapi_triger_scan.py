# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from opts import debug


class NbapiScanTrigger(PrplMeshBaseTest):
    '''Executes NBAPI ScanTrigger command. '''

    def runTest(self):
        try:
            agent = self.dev.DUT.agent_entity
            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        debug(f'Triger scan for radio {agent.radios[1].mac}')
        error_msg = "\033[91mNBAPI command TriggerScan should not be executed with "
        end = '\033[0m'

        topology = self.get_topology()
        repeater1 = topology[agent.mac]
        radio = repeater1.radios[agent.radios[1].mac]

        # Test wrong amount of channels in the channel list
        ret = controller.nbapi_command_not_fail(radio.path, "ScanTrigger",
                                                {"channels_list": "52, 56, 36",
                                                 "channels_num": "42"})
        assert not ret, error_msg + " wrong amount of channels in the channel list" + end

        # Test out of range
        ret = controller.nbapi_command_not_fail(radio.path, "ScanTrigger",
                                                {"channels_list": "2147483648, -2147483648",
                                                 "channels_num": "2"})
        assert not ret, error_msg + " out of range channels" + end

        # Test invalid channel number
        ret = controller.nbapi_command_not_fail(radio.path, "ScanTrigger",
                                                {"channels_list": "52, 56, 36",
                                                 "channels_num": "-42"})
        assert not ret, error_msg + " invalid channel number" + end

        # Test trigger scan for all channels
        ret = controller.nbapi_command_not_fail(radio.path, "ScanTrigger",
                                                {"channels_list": "",
                                                 "channels_num": "0"})
        assert ret, "NBAPI command ScanTrigger should trigger scan for all channels"

        # Test valid input
        ret = controller.nbapi_command(radio.path, "ScanTrigger",
                                       {"channels_list": "52, 56, 36",
                                        "channels_num": "3"})
        assert ret, "NBAPI command ScanTrigger should trigger scan for channels: 52, 56, 36"
