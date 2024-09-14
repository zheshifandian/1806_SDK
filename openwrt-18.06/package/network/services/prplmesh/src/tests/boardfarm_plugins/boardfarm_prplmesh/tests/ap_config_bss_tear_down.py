# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from capi import tlv


class ApConfigBSSTeardown(PrplMeshBaseTest):
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
        # Configure the controller and send renew
        self.device_reset_default()
        controller.cmd_reply(
            "DEV_SET_CONFIG,bss_info1,"
            "{} 8x Boardfarm-Tests-24G-3 0x0020 0x0008 maprocks1 0 1".format(agent.mac))
        controller.dev_send_1905(agent.mac,
                                 self.ieee1905['eMessageType']
                                 ['AP_AUTOCONFIGURATION_RENEW_MESSAGE'],
                                 tlv(self.ieee1905['eTlvType']['TLV_AL_MAC_ADDRESS'],
                                     "{" + controller.mac + "}"),
                                 tlv(self.ieee1905['eTlvType']['TLV_SUPPORTED_ROLE'],
                                     "{" + f"""0x{self.ieee1905['tlvSupportedRole']
                                     ['eValue']['REGISTRAR']:02x}""" + "}"),
                                 tlv(self.ieee1905['eTlvType']['TLV_SUPPORTED_FREQ_BAND'],
                                     "{" + f"""0x{self.ieee1905['tlvSupportedFreqBand']
                                     ['eValue']['BAND_2_4G']:02x}""" + "}"))

        # Wait until the connection map is updated:
        self.check_log(controller,
                       rf"Setting node '{agent.radios[0].mac}' as active", timeout=10)
        self.check_log(controller,
                       rf"Setting node '{agent.radios[1].mac}' as active", timeout=10)

        self.check_log(agent.radios[0],
                       r"Autoconfiguration for ssid: Boardfarm-Tests-24G-3 .*"
                       r"fronthaul: true backhaul: false")
        self.check_log(agent.radios[1], r".* tear down radio")
        conn_map = controller.get_conn_map()
        repeater1 = conn_map[agent.mac]
        repeater1_wlan0 = repeater1.radios[agent.radios[0].mac]
        for vap in repeater1_wlan0.vaps.values():
            if vap.ssid not in ('Boardfarm-Tests-24G-3', 'N/A'):
                self.fail('Wrong SSID: {vap.ssid} instead of Boardfarm-Tests-24G-3'.format(vap=vap))
        repeater1_wlan2 = repeater1.radios[agent.radios[1].mac]
        for vap in repeater1_wlan2.vaps.values():
            if vap.ssid != 'N/A':
                self.fail('Wrong SSID: {vap.ssid} instead torn down'.format(vap=vap))

        self.checkpoint()
        # SSIDs have been removed for the CTT Agent1's front radio
        controller.cmd_reply(
            "DEV_SET_CONFIG,bss_info1,{} 8x".format(agent.mac))
        # Send renew message
        controller.dev_send_1905(agent.mac,
                                 self.ieee1905['eMessageType']
                                 ['AP_AUTOCONFIGURATION_RENEW_MESSAGE'],
                                 tlv(self.ieee1905['eTlvType']['TLV_AL_MAC_ADDRESS'],
                                     "{" + controller.mac + "}"),
                                 tlv(self.ieee1905['eTlvType']['TLV_SUPPORTED_ROLE'],
                                     "{" + f"""0x{self.ieee1905['tlvSupportedRole']
                                     ['eValue']['REGISTRAR']:02x}""" + "}"),
                                 tlv(self.ieee1905['eTlvType']['TLV_SUPPORTED_FREQ_BAND'],
                                     "{" + f"""0x{self.ieee1905['tlvSupportedFreqBand']
                                     ['eValue']['BAND_2_4G']:02x}""" + "}"))

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
