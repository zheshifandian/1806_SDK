###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################
from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from opts import debug
import time


class ClientCapabilityQuerySuccessful(PrplMeshBaseTest):
    """
        Devices used in test setup:
        STA1 - WIFI repeater
        AP1 - Agent1 [DUT]

        Dummy STA is connected to wlan0
        Client capability query is sent to STA
        AP1 is checked for a sent capability query
        The there should be an expected capability frame on the report
    """

    def runTest(self):
        # Locate test participants
        try:
            sta = self.dev.wifi
            agent = self.dev.DUT.agent_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        expected_association_frame = "31:14:14:00:00:0e:4d:75:6c:74:69:2d:41:50:2d:32:"\
                                     "34:47:2d:31:01:08:02:04:0b:0c:12:16:18:24:21:02:"\
                                     "00:14:30:14:01:00:00:0f:ac:04:01:00:00:0f:ac:04:"\
                                     "01:00:00:0f:ac:02:00:00:32:04:30:48:60:6c:3b:10:"\
                                     "51:51:53:54:73:74:75:76:77:78:7c:7d:7e:7f:80:82:"\
                                     "3b:16:0c:01:02:03:04:05:0c:16:17:18:19:1a:1b:1c:"\
                                     "1d:1e:1f:20:21:80:81:82:46:05:70:00:00:00:00:46:"\
                                     "05:71:50:50:00:04:7f:0a:04:00:0a:82:21:40:00:40:"\
                                     "80:00:dd:07:00:50:f2:02:00:01:00:2d:1a:2d:11:03:"\
                                     "ff:ff:00:00:00:00:00:00:00:00:00:00:00:00:00:00:"\
                                     "00:00:18:e6:e1:09:00:bf:0c:b0:79:d1:33:fa:ff:0c:"\
                                     "03:fa:ff:0c:03:c7:01:10:dd:07:50:6f:9a:16:03:01:"\
                                     "03"
        # connect a station
        debug("Connect dummy STA to wlan0")
        sta.wifi_connect_check(agent.radios[0].vaps[0])

        time.sleep(1)

        # then check capability query is successful with connected station
        try:
            report = self.base_test_client_capability_query(sta)

            cap_report_tlv = self.check_cmdu_has_tlvs(report,
                                                      self.ieee1905['eTlvTypeMap']
                                                      ['TLV_CLIENT_CAPABILITY_REPORT'])[0]
            self.safe_check_obj_attribute(cap_report_tlv, 'client_capability_result', '0x00000000',
                                          "Capability report result is not successful")
            try:
                if cap_report_tlv.client_capability_frame != expected_association_frame:
                    self.fail("Capability report does not contain expected frame")
                    debug(f"Frame received\n{cap_report_tlv.client_capability_frame}")
                    debug(f"Frame expected\n{expected_association_frame}")
            except AttributeError:
                self.fail("Report does not contain capability frame")
        finally:  # cleanup
            agent.radios[0].vaps[0].disassociate(sta)
