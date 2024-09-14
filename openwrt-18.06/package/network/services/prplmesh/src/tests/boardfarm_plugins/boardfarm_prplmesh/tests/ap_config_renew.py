###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################
import time

from .prplmesh_base_test import PrplMeshBaseTest
from capi import tlv
import environment


class ApConfigRenew(PrplMeshBaseTest):
    """Check initial configuration on device."""

    def runTest(self):
        # Locate test participants
        agent = self.dev.DUT.agent_entity
        controller = self.dev.lan.controller_entity

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)
        # Regression test: MAC address should be case insensitive
        mac_repeater1_upper = agent.mac.upper()
        self.device_reset_default()
        controller.ucc_socket.cmd_reply(
            "DEV_SET_CONFIG,"
            "bss_info1,{} 8x Boardfarm-Tests-24G-1 0x0020 0x0008 maprocks1 0 1,"
            "bss_info2,{} 8x Boardfarm-Tests-24G-2 0x0020 0x0008 maprocks2 1 0"
            .format(mac_repeater1_upper, agent.mac))
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

        time.sleep(5)

        ssid_1 = 'Boardfarm-Tests-24G-1'
        radio_0_vap_0 = agent.radios[0].get_vap(ssid_1)
        if not radio_0_vap_0:
            self.fail("Radio 0 vap {} not found".format(ssid_1))

        vap_bss_type = radio_0_vap_0.get_bss_type()

        if vap_bss_type != environment.BssType.Fronthaul:
            self.fail(
                f"Radio 0 vap {ssid_1} bss type is {vap_bss_type.name}"
                " when it should be Fronthaul")

        ssid_2 = 'Boardfarm-Tests-24G-2'
        radio_0_vap_1 = agent.radios[0].get_vap(ssid_2)
        if not radio_0_vap_1:
            self.fail("Radio 1 vap {} not found".format(ssid_2))

        vap_bss_type = radio_0_vap_1.get_bss_type()

        if vap_bss_type != environment.BssType.Backhaul:
            self.fail(
                f"Radio 1 vap {ssid_2} bss type is {vap_bss_type.name}"
                " when it should be Backhaul")

        self.check_log(agent.radios[1],
                       r"tear down radio",
                       timeout=60)
        bssid1 = agent.ucc_socket.dev_get_parameter('macaddr',
                                                    ruid='0x' +
                                                    agent.radios[0].mac.replace(':', ''),
                                                    ssid=ssid_1)
        if not bssid1:
            self.fail(f"repeater1 didn't configure {ssid_1}")
