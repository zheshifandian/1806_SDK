###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from opts import debug

import time


class ApOperationalBss(PrplMeshBaseTest):
    ''' Check fields in ApOperationalBSS TLV '''

    def runTest(self):
        try:
            controller = self.dev.lan.controller_entity
            agent = self.dev.DUT.agent_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)
        self.checkpoint()
        ssid = 'Boardfarm-Tests-24G-5g-1'
        controller.beerocks_cli_command('bml_clear_wifi_credentials {}'.format(agent.mac))
        controller.beerocks_cli_command('bml_set_wifi_credentials {} {} {} {} {}'
                                        .format(agent.mac,
                                                ssid,
                                                "maprocks1",
                                                "24g-5g",
                                                "fronthaul"))
        controller.beerocks_cli_command('bml_update_wifi_credentials {}'.format(agent.mac))
        time.sleep(3)
        debug("Send 1905 a Topology Query message")
        mid = controller.dev_send_1905(
            agent.mac, self.ieee1905['eMessageType']['TOPOLOGY_QUERY_MESSAGE'])
        time.sleep(1)
        topology_resp = self.check_cmdu_type_single(
            "Topology Response message", self.ieee1905['eMessageType']
            ['TOPOLOGY_RESPONSE_MESSAGE'],
            agent.mac, controller.mac, mid)
        ap_op_bss_tlv = self.check_cmdu_has_tlv_single(
            topology_resp, self.ieee1905['eTlvTypeMap']['TLV_AP_OPERATIONAL_BSS'])

        if int(ap_op_bss_tlv.ap_bss_radio_count) != len(agent.radios):
            self.fail("No radios found in AP Operational TLV.")
        if not hasattr(ap_op_bss_tlv, 'ap_operational_bss_radio'):
            self.fail("Missing ap_operational_bss_radio.")

        for radio in ap_op_bss_tlv.ap_operational_bss_radio:
            if not hasattr(radio, 'ap_radio_identifier'):
                self.fail("Missing ap_radio_identifier.")
            radio_id = radio.ap_radio_identifier
            current_radio = next((r for r in agent.radios if r.mac == radio_id), None)
            if current_radio is None:
                self.fail("Reported non-existent radio {}".format(radio_id))
            if int(radio.ap_bss_intf_count) == 0:
                self.fail("No AP BSS interface found. bss_local_interface_list is empty.")
            if hasattr(radio, 'ap_operational_bss_local_interface') == 0:
                self.fail("Missing ap_operational_bss_local_interface.")
            radio_vap = current_radio.get_vap(ssid)
            for bss_interface in radio.ap_operational_bss_local_interface:
                self.safe_check_obj_attribute(bss_interface, 'ap_bss_local_intf_addr',
                                              radio_vap.bssid,
                                              "Wrong BSSID: {}, expected: {}".format(
                                                  bss_interface.ap_bss_local_intf_addr,
                                                  radio_vap.bssid))
                self.safe_check_obj_attribute(bss_interface, 'ap_bss_local_intf_ssid',
                                              ssid,
                                              "Wrong mac SSID: {}, expected: {}"
                                              .format(bss_interface.ap_bss_local_intf_ssid,
                                                      ssid))
        debug("No errors found in the AP Operational TLV.")
