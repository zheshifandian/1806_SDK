
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from opts import debug
from typing import List
import sniffer

import time


class NbapiCapabilities(PrplMeshBaseTest):
    '''
       Test for NBAPI Device.WiFi.DataElements.Network.Device.Radio.Capabilities object.
       Object describes the Wi-Fi capabilities of the radio.
    '''

    def assertEqualInt(self, name: str, actual: int, expected: str):
        assert int(actual) == int(expected, 0),\
            f"Wrong value for {name}: {int(actual)} expected {int(expected, 0)}"

    def check_op_class(self, supported_op_classes: List[sniffer.TlvStruct],
                       nbapi_op_class_path: str, controller):
        class_nbapi = controller.nbapi_get_parameter(nbapi_op_class_path, "Class")
        max_tx_power_nbapi = controller.nbapi_get_parameter(nbapi_op_class_path, "MaxTxPower")
        non_op_ch_count_nbapi = controller.nbapi_get_parameter(
            nbapi_op_class_path, "NumberOfNonOperChan")

        matching_op_class = [op_class for op_class in supported_op_classes if int(
            op_class.op_class) == class_nbapi]

        assert len(matching_op_class) == 1, f"Wrong NBAPI operating class [{class_nbapi}]."
        op_class = matching_op_class[0]
        self.assertEqualInt("MaxTxPower", max_tx_power_nbapi, op_class.max_power)
        self.assertEqualInt("NumberOfNonOperChan", non_op_ch_count_nbapi, op_class.non_op_channels)
        if non_op_ch_count_nbapi != 0:
            non_op_channels_nbapi = controller.nbapi_get_list_instances(
                nbapi_op_class_path + ".NonOperable")
            for non_op_channel_nbapi in non_op_channels_nbapi:
                channel = controller.nbapi_get_parameter(
                    non_op_channel_nbapi, "NonOpChannelNumber")

                non_op_channels = [o for o in op_class.non_operating_channel if int(
                    o.non_op_channel) == channel]

                if non_op_ch_count_nbapi == 2 and not non_op_channels:
                    non_op_channels = [o for o in op_class.non_operating_channel if int(
                        o.non_op_channel_2) == channel]

                assert len(non_op_channels) == 1, f"Non-operable channel {channel} was not found."
        supported_op_classes.remove(op_class)

    def runTest(self):
        try:
            agent = self.dev.DUT.agent_entity
            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        # Send AP capability query as we need TLVs from returning response.
        debug("Send AP capability query to agent")
        mid = controller.dev_send_1905(agent.mac,
                                       self.ieee1905['eMessageType']['AP_CAPABILITY_QUERY_MESSAGE'])
        time.sleep(1)

        debug("Confirming AP capability query has been received on agent")
        self.check_log(agent, "AP_CAPABILITY_QUERY_MESSAGE")
        debug("Confirming AP capability report has been received on controller")
        self.check_log(controller, "AP_CAPABILITY_REPORT_MESSAGE")
        ap_cap_report = self.check_cmdu_type(
            "AP_CAPABILITY_REPORT_MESSAGE",
            self.ieee1905['eMessageType']['AP_CAPABILITY_REPORT_MESSAGE'],
            agent.mac, controller.mac, mid)

        topology = self.get_topology()
        repeater = topology[agent.mac]
        for ap_cap_report in ap_cap_report:
            for tlv in ap_cap_report.ieee1905_tlvs:
                if tlv.tlv_type == self.ieee1905['eTlvTypeMap']['TLV_AP_RADIO_BASIC_CAPABILITIES']:
                    radio = repeater.radios[tlv.ap_radio_identifier]
                    op_classes_nbapi = controller.nbapi_get_list_instances(
                        radio.path + ".Capabilities.OperatingClasses")
                    for op_class_nbapi in op_classes_nbapi:
                        self.check_op_class(
                            tlv.supported_operating_classes, op_class_nbapi, controller)
                    assert not tlv.supported_operating_classes, \
                        "Not all operating classes was reported in data model."
                if tlv.tlv_type == self.ieee1905['eTlvTypeMap']['TLV_AP_HT_CAPABILITIES']:
                    radio = repeater.radios[tlv.ap_ht_radio_id]
                    ht_caps = self.get_nbapi_ht_capabilities(radio.path + ".Capabilities")
                    self.assertEqualInt("rx_spatial_streams", ht_caps['rx_ss'] - 1,
                                        tlv.ap_ht_caps_tree['ieee1905.ap_ht.max_rx_streams'])
                    self.assertEqualInt("tx_spatial_streams", ht_caps['tx_ss'] - 1,
                                        tlv.ap_ht_caps_tree['ieee1905.ap_ht.max_tx_streams'])
                    self.assertEqualInt("GI_20_MHz", ht_caps['gi_20_mhz'],
                                        tlv.ap_ht_caps_tree['ieee1905.ap_ht.short_gi_20mhz'])
                    self.assertEqualInt("GI_40_MHz", ht_caps['gi_40_mhz'],
                                        tlv.ap_ht_caps_tree['ieee1905.ap_ht.short_gi_40mhz'])
                    self.assertEqualInt("HT_40_Mhz", ht_caps['ht_40_mhz'],
                                        tlv.ap_ht_caps_tree['ieee1905.ap_ht.ht_support_40mhz'])
                if tlv.tlv_type == self.ieee1905['eTlvTypeMap']['TLV_AP_VHT_CAPABILITIES']:
                    radio = repeater.radios[tlv.ap_vht_radio_id]
                    vht_caps = self.get_nbapi_vht_capabilities(radio.path + ".Capabilities")
                    self.assertEqualInt("rx_spatial_streams", vht_caps['rx_ss'] - 1,
                                        tlv.ap_vht_caps_tree['ieee1905.ap_vht.max_rx_streams'])
                    self.assertEqualInt("tx_spatial_streams", vht_caps['tx_ss'] - 1,
                                        tlv.ap_vht_caps_tree['ieee1905.ap_vht.max_tx_streams'])
                    self.assertEqualInt("GI_80_MHz", vht_caps['gi_80_mhz'],
                                        tlv.ap_vht_caps_tree['ieee1905.ap_vht.short_gi_80mhz'])
                    self.assertEqualInt("GI_160_MHz", vht_caps['gi_160_mhz'],
                                        tlv.ap_vht_caps_tree['ieee1905.ap_ht.short_gi_160mhz'])
                    self.assertEqualInt("VHT_80_80_MHz", vht_caps['vht_80_80_mhz'],
                                        tlv.ap_vht_caps_tree['ieee1905.ap_ht.vht_80plus_mhz'])
                    self.assertEqualInt("VHT_160_MHz", vht_caps['vht_160_mhz'],
                                        tlv.ap_vht_caps_tree['ieee1905.ap_ht.vht_160mhz'])
                    self.assertEqualInt("SU_beamformer", vht_caps['su_beamformer'],
                                        tlv.ap_vht_caps_tree['ieee1905.ap_ht.su_beamformer'])
                    self.assertEqualInt("MU_beamformer", vht_caps['mu_beamformer'],
                                        tlv.ap_vht_caps_tree['ieee1905.ap_ht.mu_beamformer'])
                    self.assertEqualInt(
                        "VHT_Tx_MCS", vht_caps['vht_tx_mcs'], tlv.vht_supported_tx_mcs)
                    self.assertEqualInt(
                        "VHT_Rx_MCS", vht_caps['vht_rx_mcs'], tlv.vht_supported_rx_mcs)
