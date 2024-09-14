
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from capi import tlv
from opts import debug

import time


class NbapiRadio(PrplMeshBaseTest):
    '''
        Test for NBAPI Radio object checks if values for all parameters
        and for its sub-object CurrentOperatingClasses were set properly.
    '''

    def assertEqual(self, path: str, name: str, expected: str):
        controller = self.dev.lan.controller_entity
        actual = controller.nbapi_get_parameter(path, name)
        assert actual == int(expected), f"Wrong value for {name}: {actual} expected {expected}"

    def runTest(self):
        try:
            agent = self.dev.DUT.agent_entity
            controller = self.dev.lan.controller_entity
            sta = self.dev.wifi
            vap = agent.radios[1].vaps[0]
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)
        self.device_reset_then_set_config()
        self.configure_ssids(['TestNBAPIRadio'])

        sta.wifi_connect(vap)
        time.sleep(2)

        topology = self.get_topology()
        repeater = topology[agent.mac]
        radio = repeater.radios[agent.radios[0].mac]

        debug("Send AP Metrics Query message to agent 1")
        mid = controller.dev_send_1905(agent.mac,
                                       self.ieee1905['eMessageType']['AP_METRICS_QUERY_MESSAGE'],
                                       tlv(self.ieee1905['eTlvTypeMap']['TLV_AP_METRIC_QUERY'],
                                           "0x01 {%s}" % (vap.bssid)))
        debug("Send empty Channel Selection Request")
        controller.dev_send_1905(agent.mac,
                                 self.ieee1905['eMessageType']['CHANNEL_SELECTION_REQUEST_MESSAGE'],
                                 tlv(0x00, "{}"))
        time.sleep(2)
        ap_metric_resp = self.check_cmdu_type_single("AP Metrics Response",
                                                     self.ieee1905['eMessageType']
                                                     ['AP_METRICS_RESPONSE_MESSAGE'],
                                                     agent.mac, controller.mac, mid)
        op_ch_reports = self.check_cmdu_type("Operating Channel Report",
                                             self.ieee1905['eMessageType']
                                             ['OPERATING_CHANNEL_REPORT_MESSAGE'], agent.mac,
                                             controller.mac)
        ap_metrics_tlv = self.check_cmdu_has_tlv_single(ap_metric_resp,
                                                        self.ieee1905['eTlvTypeMap']
                                                        ['TLV_AP_METRIC'])
        radio_metrics_tlv = self.check_cmdu_has_tlvs(ap_metric_resp,
                                                     self.ieee1905['eTlvTypeMap']
                                                     ['TLV_PROFILE2_RADIO_METRICS'])

        # The tshark version 2.6.20 used in boardfarm doesn't properly parse this TLV.
        assert len(radio_metrics_tlv[0].tlv_data), "tlv_data of Radio Metrics TLV is empty!"
        ruid = radio_metrics_tlv[0].tlv_data[0:17]
        noise = radio_metrics_tlv[0].tlv_data[18:20]
        transmit = radio_metrics_tlv[0].tlv_data[21:23]
        resive_self = radio_metrics_tlv[0].tlv_data[24:26]
        recive_other = radio_metrics_tlv[0].tlv_data[27:29]

        nbapi_ruid = controller.nbapi_get_parameter(radio.path, "ID")
        assert nbapi_ruid == agent.radios[0].mac, f"Wrong ruid: {nbapi_ruid}, expected {ruid}"

        self.assertEqual(radio.path, "Utilization", ap_metrics_tlv.ap_metrics_channel_util)
        self.assertEqual(radio.path, "ReceiveOther", recive_other)
        self.assertEqual(radio.path, "ReceiveSelf", resive_self)
        self.assertEqual(radio.path, "Transmit", transmit)
        self.assertEqual(radio.path, "Noise", noise)

        op_classes = controller.nbapi_get_list_instances(
            radio.path + ".CurrentOperatingClasses")
        for op_class in op_classes:
            nbapi_class = controller.nbapi_get_parameter(op_class, "Class")
            found = False
            for report in op_ch_reports:
                op_ch_tlvs = self.check_cmdu_has_tlvs(report,
                                                      self.ieee1905['eTlvTypeMap']
                                                      ['TLV_OPERATING_CHANNEL_REPORT'])
                for op_ch_tlv in op_ch_tlvs:
                    if op_ch_tlv.operating_channel_radio_id == agent.radios[0].mac:
                        matching_op_class = [op_class for op_class in op_ch_tlv.operating_classes
                                             if int(op_class.op_class) == nbapi_class]
                        assert len(matching_op_class) == 1, "More than one operating channel match."
                        self.assertEqual(op_class, "Channel", matching_op_class[0].chan_num)
                        self.assertEqual(op_class, "TxPower", op_ch_tlv.operating_channel_eirp)
                        # Verify that all op channels were checked:
                        op_ch_tlv.operating_channel_radio_id = ''
                        found = True
            assert found, f"No operating channel report TLV found for {agent.radios[0].mac}"

        missing_op_class = [op_ch_tlv for op_ch_tlv in [self.check_cmdu_has_tlv_single(
            report, self.ieee1905['eTlvTypeMap']['TLV_OPERATING_CHANNEL_REPORT'])
            for report in op_ch_reports]
            if op_ch_tlv.operating_channel_radio_id == agent.radios[0].mac]
        assert not missing_op_class, f"CurrentOperatingClasses missing value for {missing_op_class}"
