###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from opts import debug


class ClientCapabilityQueryFailsWithNoSta(PrplMeshBaseTest):
    """
        Devices used in test setup:
        STA1 - WIFI repeater
        AP1 - Agent1 [DUT]
        GW - Controller

        Client capability query is sent to unconnected STA
        AP1 is checked for a sent capability query
        The action should return an error message
    """

    def runTest(self):
        # Locate test participants
        try:
            sta = self.dev.wifi
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        report = self.base_test_client_capability_query(sta)

        debug("Send client capability query for unconnected STA")

        cap_report_tlv = self.check_cmdu_has_tlv_single(report,
                                                        self.ieee1905['eTlvTypeMap']
                                                        ['TLV_CLIENT_CAPABILITY_REPORT'])
        self.safe_check_obj_attribute(cap_report_tlv, 'client_capability_result', '0x00000001',
                                      "Capability query was successful for disconnected STA")

        error_tlv = self.check_cmdu_has_tlv_single(
            report, self.ieee1905['eTlvTypeMap']['TLV_ERROR_CODE'])
        self.safe_check_obj_attribute(error_tlv, 'error_code_reason', '0x00000002',
                                      "Wrong error reason code")
        self.safe_check_obj_attribute(error_tlv, 'error_code_mac_addr', sta.mac,
                                      "Wrong mac address in error code")
