# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from capi import tlv
from opts import debug
import time


class BeaconReportQuery(PrplMeshBaseTest):

    def runTest(self):
        # Locate test participants
        try:
            sta = self.dev.wifi
            agent = self.dev.DUT.agent_entity
            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        debug("Connect dummy STA (" + sta.mac + ") to wlan0")
        agent.radios[0].vaps[0].associate(sta)

        # send beacon query request
        # (please take a look at https://github.com/prplfoundation/prplMesh/issues/1272)
        debug("Sending beacon report query to repeater:")
        request = '{mac} '.format(mac=sta.mac)
        request += '0x73 0xFF 0xFFFFFFFFFFFF 0x02 0x00 0x01 0x02 0x73 0x24 0x30 0x00'

        debug(request)
        mid = controller.dev_send_1905(agent.mac, self.ieee1905['eMessageType']
                                       ['BEACON_METRICS_QUERY_MESSAGE'],
                                       tlv(self.ieee1905['eTlvTypeMap']
                                           ['TLV_BEACON_METRICS_QUERY'],
                                           "{" + request + "}"))

        time.sleep(3)
        self.check_cmdu_type("ACK",
                             self.ieee1905['eMessageType']['ACK_MESSAGE'],
                             agent.mac, controller.mac, mid)

        # this line is printed in the monitor log - however currently there is no way to test it -
        # self.check_log(env.agents[0].radios[0].???,
        #                r"inserting 1 RRM_EVENT_BEACON_REP_RXED event(s) to the pending list")
        agent.radios[0].vaps[0].disassociate(sta)
