###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

import time

from .prplmesh_base_test import PrplMeshBaseTest
from capi import tlv
from opts import debug


class ClientAssociationLinkMetrics(PrplMeshBaseTest):
    ''' This test verifies that a MAUT with an associated STA responds to
    an Associated STA Link Metrics Query message with an Associated STA Link Metrics
    Response message containing an Associated STA Link Metrics TLV for the associated STA.'''

    def runTest(self):
        # Locate test participants
        agent = self.dev.DUT.agent_entity
        controller = self.dev.lan.controller_entity
        sta = self.dev.wifi

        # Regression check
        # Don't connect nonexistent Station
        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)
        sta_mac = "11:11:33:44:55:66"
        debug("Send link metrics query for unconnected STA")
        controller.ucc_socket.dev_send_1905(agent.mac,
                                            self.ieee1905['eMessageType']
                                            ['ASSOCIATED_STA_LINK_METRICS_QUERY_MESSAGE'],
                                            tlv(self.ieee1905['eTlvTypeMap']
                                                ['TLV_STAMAC_ADDRESS_TYPE'],
                                                '{sta_mac}'.format(sta_mac=sta_mac)))
        self.check_log(agent,
                       "client with mac address {sta_mac} not found".format(sta_mac=sta_mac))
        time.sleep(1)

        debug('sta: {}'.format(sta.mac))
        sta.wifi_connect_check(agent.radios[0].vaps[0])

        time.sleep(1)

        mid = controller.ucc_socket.dev_send_1905(agent.mac, self.ieee1905['eMessageType']
                                                  ['ASSOCIATED_STA_LINK_METRICS_QUERY_MESSAGE'],
                                                  tlv(self.ieee1905['eTlvTypeMap']
                                                  ['TLV_STAMAC_ADDRESS_TYPE'],
                                                      '{sta_mac}'.format(sta_mac=sta.mac)))
        time.sleep(1)
        self.check_log(agent,
                       "Send AssociatedStaLinkMetrics to controller, mid = {}".format(mid),
                       timeout=20)
