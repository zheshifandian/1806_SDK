
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest

import time


class NbapiAccessPoint(PrplMeshBaseTest):
    '''
    This test supposed to test all possible parameters of AccessPoint object..
    '''

    def check_bss_is_disabled(self, ssid: str, radio, controller):
        for bss in radio.vaps.values():
            enabled = controller.nbapi_get_parameter(bss.path, "Enabled")
            if bss.ssid == ssid:
                assert not enabled, f"BSS {bss.ssid} is enabled."
                self.fail(f"BSS with SSID: {ssid}, expect does not appear on radio "
                          "uid: {radio.uid}, path: {radio.path}.")

    def check_bss_in_radio(self, ssid: str, radio, ssids, controller):
        found = False
        for bss in radio.vaps.values():
            enabled = controller.nbapi_get_parameter(bss.path, "Enabled")
            if bss.ssid == ssid:
                found = True
                assert enabled, f"BSS {bss.ssid} is not enabled."
            else:
                assert next((ssid_name for ssid_name, ssid_val in ssids.items()
                             if ssid_val == bss.ssid), False),\
                    f"BSS {bss.bssid} is configured with ssid {bss.ssid}."
        assert found, f"BSS with SSID: {ssid}, doesn't appear on radio "
        "uid: {radio.uid}, path: {radio.path}."

    def check_bss_conf(self, radio, ssid: str, config: {}):
        self.check_log(radio,
                       "ssid: {} auth_type: {}  "
                       "encr_type: {} network_key: {} "
                       "fronthaul: {} backhaul: {}"
                       .format(ssid,
                               config.get('auth_type', 'NONE'),
                               config.get('encr_type', 'NONE'),
                               config.get('network_key', ''),
                               config.get('fronthaul', 'true'),
                               config.get('backhaul', 'false')),
                       timeout=60)

    def runTest(self):
        try:
            agent = self.dev.DUT.agent_entity
            agent2 = self.dev.lan2.agent_entity
            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        ''' Test Access Point object '''

        # Add Access Point object and set up parameters for it
        self.configure_ssids_clear()

        ssid = {
            "all_bands": "Test-all-bands",
            "5GH_24G": "Test-5GH-24G",
            "5GL": "Test-5GL",
            "6G": "Test-6G",
            "F+B": "Test-FronthaulBackhaul"
        }

        all_bands_security_obj_path = self.configure_ssid(ssid["all_bands"]) + ".Security"
        self.configure_ssid(ssid["5GH_24G"], "Fronthaul", {"Band2_4G": True, "Band5GH": True})
        self.configure_ssid(ssid["5GL"], "Fronthaul", {"Band5GL": True})
        self.configure_ssid(ssid["6G"], "Fronthaul", {"Band6G": True})
        self.configure_ssid(ssid["F+B"], "Fronthaul+Backhaul")

        controller.nbapi_set_parameters(all_bands_security_obj_path,
                                        {"ModeEnabled": "WPA2-Personal"})
        controller.nbapi_set_parameters(all_bands_security_obj_path,
                                        {"KeyPassphrase": "key_passphrease_value"})

        controller.nbapi_command("Device.WiFi.DataElements.Network", "AccessPointCommit")
        time.sleep(10)

        topology = self.get_topology()
        for device in topology.values():
            print(device)

        config_all_bands = {
            "fronthaul": "true", "backhaul": "false",
            "auth_type": "WPA2-PSK", "encr_type": "AES",
            "network_key": "key_passphrease_value"
        }

        self.check_bss_conf(agent.radios[0], ssid["all_bands"], config_all_bands)
        self.check_bss_conf(agent.radios[1], ssid["all_bands"], config_all_bands)
        self.check_bss_conf(agent2.radios[0], ssid["all_bands"], config_all_bands)
        self.check_bss_conf(agent2.radios[1], ssid["all_bands"], config_all_bands)

        self.check_bss_conf(agent.radios[0], ssid["5GH_24G"], {"fronthaul": "true"})
        self.check_bss_conf(agent.radios[1], ssid["5GH_24G"], {"fronthaul": "true"})
        self.check_bss_conf(agent2.radios[0], ssid["5GH_24G"], {"fronthaul": "true"})
        self.check_bss_conf(agent2.radios[1], ssid["5GH_24G"], {"fronthaul": "true"})

        self.check_bss_conf(agent.radios[1], ssid["5GL"], {"fronthaul": "true"})
        self.check_bss_conf(agent2.radios[1], ssid["5GL"], {"fronthaul": "true"})

        self.check_bss_conf(agent.radios[0], ssid["F+B"], {"backhaul": "true"})
        self.check_bss_conf(agent.radios[1], ssid["F+B"], {"backhaul": "true"})
        self.check_bss_conf(agent2.radios[0], ssid["F+B"], {"backhaul": "true"})
        self.check_bss_conf(agent2.radios[1], ssid["F+B"], {"backhaul": "true"})

        bssid_all_bands = agent.ucc_socket.dev_get_parameter('macaddr',
                                                             ruid='0x' +
                                                             agent.radios[1].mac.replace(':', ''),
                                                             ssid=ssid["all_bands"])
        bssid_5GH_24G = agent.ucc_socket.dev_get_parameter('macaddr',
                                                           ruid='0x' +
                                                           agent.radios[1].mac.replace(':', ''),
                                                           ssid=ssid["5GH_24G"])
        if not bssid_all_bands:
            self.fail(f"Repeater1 didn't configure {ssid['all_bands']} on radio 1.")
        if not bssid_5GH_24G:
            self.fail(f"Repeater1 didn't configure {ssid['5GH_24G']} on radio 1.")

        # Check security settings
        # Should be fixed in PPM-1041
        # dm_key_passphrase = controller.nbapi_get_parameter(
        #     all_bands_security_obj_path, "KeyPassphrase")
        # assert dm_key_passphrase == "",\
        #     f"KeyPassphrase for {all_bands_security_obj_path} should be hidden."

        repeater1 = topology[agent.mac]
        repeater2 = topology[agent2.mac]
        self.check_bss_in_radio(
            ssid["5GL"], repeater1.radios[agent.radios[1].mac], ssid, controller)
        self.check_bss_in_radio(
            ssid["5GL"], repeater2.radios[agent2.radios[1].mac], ssid, controller)
        self.check_bss_is_disabled(
            ssid["5GL"], repeater1.radios[agent.radios[0].mac], controller)
        self.check_bss_is_disabled(
            ssid["5GL"], repeater2.radios[agent2.radios[0].mac], controller)

        # Verify Access Point with all bands enabled: 2/4G, 5GH, 5GL, 6G
        for device in topology.values():
            for radio in device.radios.values():
                self.check_bss_in_radio(ssid["all_bands"], radio, ssid, controller)
                self.check_bss_in_radio(ssid["5GH_24G"], radio, ssid, controller)
                self.check_bss_in_radio(ssid["F+B"], radio, ssid, controller)
                self.check_bss_is_disabled(ssid["6G"], radio, controller)
