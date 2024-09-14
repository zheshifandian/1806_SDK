###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################
import time

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from capi import tlv
from environment import ChannelTlvs
from opts import debug


class ChannelSelection(PrplMeshBaseTest):
    """
    Devices used in test setup:
            AP1 - Agent1 [DUT]
            GW - Controller

    Check if the channel is switched when there's a channel preference but
     an empty channel selection request
    - Should fail if channels are different
    Check if the channel is switched when there's an empty channel selection
     request changing tx_power_limit
    - Should fail if channels are different
    Trigger channel selection to channel 6 and 36. Check that operating channel
     report was sent.
    - Should fail if channels haven't changed to 6 and 36
    """

    def runTest(self):

        def check_single_channel_response(self, resp_code) -> None:
            cs_resp = self.check_cmdu_type_single("channel selection response",
            self.ieee1905['eMessageType']['CHANNEL_SELECTION_RESPONSE_MESSAGE'],
            agent.mac, controller.mac, cs_req_mid)  # noqa E501
            if cs_resp:
                cs_resp_tlvs = self.check_cmdu_has_tlvs(cs_resp, 0x8e)
                radio_macs = [radio.mac for radio in agent.radios]

                for cs_resp_tlv in cs_resp_tlvs:
                    if cs_resp_tlv.channel_select_radio_id not in radio_macs:
                        if int(cs_resp_tlv.channel_select_response_code, 16) != resp_code:
                            self.fail("Unexpected radio ID {}, expecting one of {}\n"
                                      "Channel selection unexpected response code {}".format(
                                cs_resp_tlv.channel_select_radio_id,  # noqa E501
                                ", ".join(radio_macs),
                                cs_resp_tlv.channel_select_response_code
                            ))
                        self.fail("Unexpected radio ID {}, expecting one of {}".format(
                            cs_resp_tlv.channel_select_radio_id,
                            ", ".join(radio_macs)))

        # Locate test participants
        try:
            agent = self.dev.DUT.agent_entity
            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        orig_chan_0 = agent.radios[0].get_current_channel()
        orig_chan_1 = agent.radios[1].get_current_channel()
        debug("Starting channel wlan0: {}, wlan2: {}".format(orig_chan_0, orig_chan_1))

        debug("Send channel preference query")
        ch_pref_query_mid = controller.dev_send_1905(agent.mac,
                                                     self.ieee1905['eMessageType']
                                                     ['CHANNEL_PREFERENCE_QUERY_MESSAGE'])
        time.sleep(3)
        debug("Confirming channel preference query has been received on agent")

        # TODO should be a single response (currently two are sent)
        self.check_cmdu_type("channel preference response",
                             self.ieee1905['eMessageType']['CHANNEL_PREFERENCE_REPORT_MESSAGE'],
                             agent.mac, controller.mac, ch_pref_query_mid)

        debug("Send empty channel selection request")
        cs_req_mid = controller.dev_send_1905(agent.mac,
                                              self.ieee1905['eMessageType']
                                              ['CHANNEL_SELECTION_REQUEST_MESSAGE'],
                                              tlv(0x00, "{}"))

        time.sleep(1)

        check_single_channel_response(self, 0x00)

        cur_chan_0 = agent.radios[0].get_current_channel()
        cur_chan_1 = agent.radios[1].get_current_channel()

        if cur_chan_0 != orig_chan_0:
            self.fail("Radio 0 channel switched to {}".format(cur_chan_0))
        if cur_chan_1 != orig_chan_1:
            self.fail("Radio 1 channel switched to {}".format(cur_chan_1))

        oper_channel_reports = self.check_cmdu_type("operating channel report",
                                                    self.ieee1905['eMessageType']
                                                    ['OPERATING_CHANNEL_REPORT_MESSAGE'],
                                                    agent.mac, controller.mac)
        for report in oper_channel_reports:
            self.check_cmdu_type_single("ACK", self.ieee1905['eMessageType']['ACK_MESSAGE'],
                                        controller.mac, agent.mac,
                                        report.ieee1905_mid)

        self.checkpoint()

        tp20dBm = 0x14
        tp21dBm = 0x15

        for payload_transmit_power in (tp20dBm, tp21dBm):
            debug("Send empty channel selection request with changing tx_power_limit")
            cs_req_mid = controller.dev_send_1905(
                agent.mac,
                self.ieee1905['eMessageType']['CHANNEL_SELECTION_REQUEST_MESSAGE'],
                tlv(self.ieee1905['eTlvTypeMap']['TLV_TRANSMIT_POWER_LIMIT'],
                    '{} 0x{:02x}'.format(agent.radios[0].mac,
                                         payload_transmit_power)),
                tlv(self.ieee1905['eTlvTypeMap']['TLV_TRANSMIT_POWER_LIMIT'],
                    '{} 0x{:02x}'.format(agent.radios[1].mac,
                                         payload_transmit_power))
            )
            time.sleep(1)

            cur_power_0 = agent.radios[0].get_power_limit()
            cur_power_1 = agent.radios[1].get_power_limit()
            if cur_power_0 != payload_transmit_power:
                self.fail("Radio 0 tx_power switched to {}".format(cur_power_0))
            if cur_power_1 != payload_transmit_power:
                self.fail("Radio 1 tx_power switched to {}".format(cur_power_1))

            # TODO should be a single response (currently two are sent)
            self.check_cmdu_type("channel selection response",
                                 self.ieee1905['eMessageType']
                                 ['CHANNEL_SELECTION_RESPONSE_MESSAGE'],
                                 agent.mac,
                                 controller.mac, cs_req_mid)

            cur_chan_0 = agent.radios[0].get_current_channel()
            cur_chan_1 = agent.radios[1].get_current_channel()
            if cur_chan_0 != orig_chan_0:
                self.fail("Radio 0 channel switched to {}".format(cur_chan_0))
            if cur_chan_1 != orig_chan_1:
                self.fail("Radio 1 channel switched to {}".format(cur_chan_1))

            oper_channel_reports = self.check_cmdu_type("operating channel report",
                                                        self.ieee1905['eMessageType']
                                                        ['OPERATING_CHANNEL_REPORT_MESSAGE'],
                                                        agent.mac, controller.mac)
            for report in oper_channel_reports:
                for ocr in report.ieee1905_tlvs:
                    if ocr.tlv_type != 0x8F:
                        self.fail("Unexpected TLV in operating channel report: {}".format(ocr))
                        continue
                    if int(ocr.operating_channel_eirp) != payload_transmit_power:
                        self.fail("Unexpected transmit power {} instead of {} for {}".format(
                            ocr.operating_channel_eirp, payload_transmit_power,
                            ocr.operating_channel_radio_id))
                self.check_cmdu_type_single("ACK",
                                            self.ieee1905['eMessageType']['ACK_MESSAGE'],
                                            controller.mac, agent.mac,
                                            report.ieee1905_mid)

            self.checkpoint()

        debug("Send invalid channel selection request to radio 0")
        cs_req_mid = controller.dev_send_1905(
            agent.mac, self.ieee1905['eMessageType']['CHANNEL_SELECTION_REQUEST_MESSAGE'],
            # Single operating class with a single channel that doesn't exist in it.
            tlv(self.ieee1905['eTlvTypeMap']['TLV_CHANNEL_PREFERENCE'],
                agent.mac + ' 0x01 {0x52 {0x01 {0x01}} 0x00}'))
        time.sleep(1)

        check_single_channel_response(self, f"""0x{self.ieee1905['tlvChannelPreference']
                                     ['ePreference']['PREFERRED2']:02x}""")

        self.checkpoint()

        # payload_wlan0 - request for change channel on 6
        payload_wlan0 = ChannelTlvs.CHANNEL_6.value

        # payload_wlan2  - request for change channel on 36
        payload_wlan2 = ChannelTlvs.CHANNEL_36.value

        """
        Step 1: Trigger channel selection to channel 6 and 36. Check that
                operating channel report was sent.

        Step 2: Trigger channel selection to channel 6 and 36 again - check that
                operating channel report is sent again. This is to catch bugs
                when we don't send channel report when there is no need to
                switch channel
        """
        for i in range(1, 3):
            debug("Send channel selection request, step {}".format(i))
            cs_req_mid = controller.dev_send_1905(
                agent.mac,
                0x8006,
                tlv(self.ieee1905['eTlvTypeMap']['TLV_CHANNEL_PREFERENCE'],
                    '{} {}'.format(agent.radios[0].mac, payload_wlan0)),
                tlv(self.ieee1905['eTlvTypeMap']['TLV_TRANSMIT_POWER_LIMIT'],
                    '{} 0x{:2x}'.format(agent.radios[0].mac, tp20dBm)),
                tlv(self.ieee1905['eTlvTypeMap']['TLV_CHANNEL_PREFERENCE'],
                    '{} {}'.format(agent.radios[1].mac, payload_wlan2)),
                tlv(self.ieee1905['eTlvTypeMap']['TLV_TRANSMIT_POWER_LIMIT'],
                    '{} 0x{:2x}'.format(agent.radios[1].mac, tp20dBm))
            )
            time.sleep(1)

            debug("Confirming tlvTransmitPowerLimit has been received with correct value on agent,"
                  " step {}".format(i))

            cur_power_0 = agent.radios[0].get_power_limit()
            cur_power_1 = agent.radios[1].get_power_limit()
            if cur_power_0 != tp20dBm:
                self.fail("Radio 0 tx_power switched to {}".format(cur_power_0))
            if cur_power_1 != tp20dBm:
                self.fail("Radio 1 tx_power switched to {}".format(cur_power_1))

            check_single_channel_response(self, 0x00)

            # payload_wlan0 and payload_wlan1 forced to channel 6 resp. 36, check that this happened
            (cur_chan_channel_0, _, _) = agent.radios[0].get_current_channel()
            (cur_chan_channel_1, _, _) = agent.radios[1].get_current_channel()
            if cur_chan_channel_0 != 6:
                self.fail("Radio 0 channel switched to {} instead of 6".format(cur_chan_channel_0))
            if cur_chan_channel_1 != 36:
                self.fail("Radio 1 channel switched to {} instead of 36".format(cur_chan_channel_1))

            oper_channel_reports = self.check_cmdu_type("operating channel report",
                                                        self.ieee1905['eMessageType']
                                                        ['OPERATING_CHANNEL_REPORT_MESSAGE'],
                                                        agent.mac, controller.mac)
            for report in oper_channel_reports:
                for ocr in report.ieee1905_tlvs:
                    if ocr.tlv_type != self.ieee1905['eTlvTypeMap']['TLV_OPERATING_CHANNEL_REPORT']:
                        self.fail("Unexpected TLV in operating channel report: {}".format(ocr))
                        continue
                    if int(ocr.operating_channel_eirp) != tp20dBm:
                        self.fail("Unexpected transmit power {} instead of {} for {}".format(
                            ocr.operating_channel_eirp, payload_transmit_power,
                            ocr.operating_channel_radio_id))
                self.check_cmdu_type_single("ACK",
                                            self.ieee1905['eMessageType']['ACK_MESSAGE'],
                                            controller.mac, agent.mac,
                                            report.ieee1905_mid)

            self.checkpoint()
