# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
import environment


class ApConfigBSSTeardownCli(PrplMeshBaseTest):
    """Check SSID is still available after being torn down

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

        # Same test as the previous one but using CLI instead of dev_send_1905

        ssid = 'Boardfarm-Tests-24G-3-cli'
        controller.beerocks_cli_command('bml_clear_wifi_credentials {}'.format(agent.mac))
        controller.beerocks_cli_command('bml_set_wifi_credentials {} {} {} {} {}'
                                        .format(agent.mac,
                                                ssid,
                                                "maprocks1",
                                                "24g",
                                                "fronthaul"))
        controller.beerocks_cli_command('bml_update_wifi_credentials {}'.format(agent.mac))

        # Wait until the connection map is updated:
        self.check_log(controller,
                       rf"Setting node '{agent.radios[0].mac}' as active", timeout=10)
        self.check_log(controller,
                       rf"Setting node '{agent.radios[1].mac}' as active", timeout=10)

        radio_0_vap_0 = agent.radios[0].get_vap(ssid)
        if not radio_0_vap_0:
            self.fail("Radio 0 vap {} not found".format(ssid))

        vap_bss_type = radio_0_vap_0.get_bss_type()

        if vap_bss_type != environment.BssType.Fronthaul:
            self.fail(
                f"Radio 0 vap {ssid} bss type is {vap_bss_type.name} "
                "when it should be Fronthaul")

        self.check_log(agent.radios[1], r".* tear down radio")

        for vap in agent.radios[0].vaps:
            vap_ssid = vap.get_ssid()
            if vap_ssid not in (ssid, 'N/A'):
                self.fail(f'Wrong SSID: {vap_ssid} instead of {ssid}')

        for vap in agent.radios[1].vaps:
            vap_ssid = vap.get_ssid()
            if vap_ssid != 'N/A':
                self.fail(f'Wrong SSID: {vap_ssid} instead torn down')

        self.checkpoint()

        controller.beerocks_cli_command('bml_clear_wifi_credentials {}'.format(agent.mac))
        controller.beerocks_cli_command('bml_update_wifi_credentials {}'.format(agent.mac))

        # Wait until the connection map is updated:
        self.check_log(controller,
                       rf"Setting node '{agent.radios[0].mac}' as active", timeout=10)
        self.check_log(controller,
                       rf"Setting node '{agent.radios[1].mac}' as active", timeout=10)

        self.check_log(agent.radios[0], r".* tear down radio")
        conn_map = controller.get_conn_map()
        repeater1 = conn_map[agent.mac]
        repeater1_wlan0 = repeater1.radios[agent.radios[0].mac]
        for vap in repeater1_wlan0.vaps.values():
            if vap.ssid != 'N/A':
                self.fail('Wrong SSID: {vap.ssid} instead torn down'.format(vap=vap))
        repeater1_wlan2 = repeater1.radios[agent.radios[1].mac]
        for vap in repeater1_wlan2.vaps.values():
            if vap.ssid != 'N/A':
                self.fail('Wrong SSID: {vap.ssid} instead torn down'.format(vap=vap))
