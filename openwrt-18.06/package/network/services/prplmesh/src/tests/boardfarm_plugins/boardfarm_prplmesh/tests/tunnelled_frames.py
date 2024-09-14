# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from opts import debug
from time import sleep


class TunnelledFrames(PrplMeshBaseTest):

    def runTest(self):
        # Locate test participants
        try:
            sta1 = self.dev.wifi
            sta2 = self.get_device_by_name('wifi2')

            controller = self.dev.lan.controller_entity

            agent1 = self.dev.DUT.agent_entity
            agent2 = self.dev.lan2.agent_entity

            vap1 = agent1.radios[0].vaps[0]
            vap2 = agent2.radios[1].vaps[0]
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        # Associate the STAs
        vap1.associate(sta1)
        vap2.associate(sta2)

        vap1_mac_hex = vap1.bssid.replace(':', '').upper()
        vap2_mac_hex = vap2.bssid.replace(':', '').upper()
        sta1_mac_hex = sta1.mac.replace(':', '').upper()
        sta2_mac_hex = sta2.mac.replace(':', '').upper()

        # Simulated events data
        event1_type = 2  # BTQ Query
        event1_data = "D0003A01{}{}{}60010A060100".format(
            vap1_mac_hex,  # DA
            sta1_mac_hex,  # SA
            vap1_mac_hex)  # BSSID

        event2_type = 4  # ANQP REQUEST
        event2_data = "D0083A01{}{}{}C000040A7D6C0200000E0000010A0002010601070108010C01".format(
            vap2_mac_hex,  # DA
            sta2_mac_hex,  # SA
            vap2_mac_hex)  # BSSID

        debug("Simulate BTM Query management frame event")
        agent1.radios[0].send_bwl_event(
            "EVENT MGMT-FRAME DATA={}".format(event1_data))

        debug("Simulate ANQP Request management frame event")
        agent2.radios[1].send_bwl_event(
            "EVENT MGMT-FRAME DATA={}".format(event2_data))

        # Allow the events to propagate
        sleep(1)

        # Since only the frame's body is tunnelled, strip the header (24 bytes)
        # from the event data passed to the validation function

        # Validate the first (WNM Request) event
        self.validate_tunnelled_frame(controller, agent1.mac, sta1.mac,
                                      event1_type, event1_data[24*2:])

        # Validate the second (ANQP REQUEST) event
        self.validate_tunnelled_frame(controller, agent2.mac, sta2.mac,
                                      event2_type, event2_data[24*2:])

        # Disconnect the stations
        vap1.disassociate(sta1)
        vap2.disassociate(sta2)

    def validate_tunnelled_frame(self, controller,
                                 agent_mac, sta_mac, payload_type, payload_data):
        '''Validates the CMDU of Controller reception of the tennulled frame.'''

        # Validate "Tunnelled Message" CMDU was sent
        response = self.check_cmdu_type_single(
            "Tunnelled Message", 0x8026, agent_mac, controller.mac)

        # This function validates R2 messages, which are not yet defined
        # in tshark 2.6.x which is the default version in Ubuntu 18.04.
        # Undefined message values are stored in "tlv_data" attributes.
        # tshark 3.x (Ubuntu 20.04) fully recognizes these messages.
        # In order to support both versions, this function checks if the
        # fully named attribute is available. If not, it simply reads the
        # value from the generic "tlv_data" attribute.

        debug("Check Tunnelled Message has valid Source Info TLV")
        tlv_source_info = self.check_cmdu_has_tlv_single(response,
                                                         self.ieee1905['eTlvTypeMap']
                                                         ['TLV_TUNNELLED_SOURCE_INFO'])
        if hasattr(tlv_source_info, 'source_info_tunneled_source_mac_address'):
            source_sta_mac = tlv_source_info.source_info_tunneled_source_mac_address
        else:
            source_sta_mac = tlv_source_info.tlv_data

        # Validate Srouce Info STA MAC
        if source_sta_mac != sta_mac:
            self.fail("Source Info TLV has wrong STA MAC {} instead of {}".format(
                source_sta_mac, sta_mac))

        debug("Check Tunnelled Message has valid Type TLV")
        tlv_type = self.check_cmdu_has_tlv_single(response,
                                                  self.ieee1905['eTlvTypeMap']
                                                  ['TLV_TUNNELLED_PROTOCOL_TYPE'])
        if hasattr(tlv_type, 'tunneled_message_type_tunneled_payload_type'):
            source_payload_type = int(tlv_type.tunneled_message_type_tunneled_payload_type, 16)
        else:
            source_payload_type = int(tlv_type.tlv_data, 16)

        if source_payload_type != payload_type:
            self.fail("Type TLV has wrong value of {} instead of {}".format(
                source_payload_type, payload_type))

        debug("Check Tunnelled Message has valid Data TLV")
        tlv_data = self.check_cmdu_has_tlv_single(response, 0xc2)
        if hasattr(tlv_type, 'tunneled_message_type_tunneled_payload_type'):
            source_payload = tlv_data.tunneled_tunneled_protocol_payload.replace(":", "")
        else:
            source_payload = tlv_data.tlv_data.replace(":", "")

        if source_payload.lower() != payload_data.lower():
            self.fail("Type TLV has wrong value of {} instead of {}".format(
                source_payload, payload_data))

        debug("Confirming Tunnelled Message was received on the Controller")
        self.check_log(
            controller, r"Received Tunnelled Message from {}".format(agent_mac))
        self.check_log(
            controller, r"Tunnelled Message STA MAC: {}, Type: 0x{:x}".format(
                sta_mac, payload_type))
