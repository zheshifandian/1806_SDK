###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################
import time

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from opts import debug


class ClientAssociationDummy(PrplMeshBaseTest):
    """Checks if allow/disallow requests are being implemented when instructed

        Devices used in test setup:
        STA1 - WIFI repeater
        AP1 - Agent1 [DUT]
        GW - Controller

        Dummy STA is connected to wlan0
        GW controller is instructed through beerocks CLI to client allow repeater 1 radio 1
        Repeater 1 radio 1 is checked to see if AP1 got allow request
        Connection map is checked for repeater 1 wlan0
        GW controller is instructed through beerocks CLI to client disallow repeater 1 radio 0
        Repeater 1 radio 0 is checked to see if AP1 got disallow request
        Dummy STA is connected to wlan2
        Connection map is checked for repeater 1 wlan2
        GW controller is instructed through beerocks CLI to client allow repeater 1 radio 1
        Repeater 1 radio 1 is checked to see if AP1 got allow request
        Connection map is checked for repeater 1 wlan2
        Connection map is checked for repeater 1 wlan0
    """

    def runTest(self):
        # Locate test participants
        try:
            agent = self.dev.DUT.agent_entity
            controller = self.dev.lan.controller_entity
            sta = self.dev.wifi
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        debug("Connect dummy STA to wlan0 with SSID ClientAssocDummy")
        self.configure_ssids(['ClientAssocDummy'])
        sta.wifi_connect_check(agent.radios[0].vaps[0])

        time.sleep(1)

        # Beerocks CLI client_allow repeater 1 wlan2
        debug("Send client association control request to the chosen BSSID (UNBLOCK)")
        print('client_allow {} {}'.format(sta.mac, agent.radios[1].mac))
        controller.beerocks_cli_command('client_allow {} {}'.format(sta.mac, agent.radios[1].mac))

        time.sleep(1)

        # Check logs repeater 1 wlan2 got client allow request
        debug("Confirming Client Association Control Request message was received (UNBLOCK)")
        self.check_log(agent.radios[1],
                       r"Got client allow request for {}".format(sta.mac), timeout=20)

        # Check in connection map for repeater 1 wlan0
        conn_map = controller.get_conn_map()

        map_radio = conn_map[agent.mac].radios[agent.radios[0].mac]
        map_vap = map_radio.vaps[agent.radios[0].vaps[0].bssid]
        if sta.mac not in map_vap.clients:
            self.fail("client {} not in conn_map, clients: {}".format(sta.mac, map_vap.clients))

        # Beerocks CLI client_disallow repeater 1 wlan0
        debug("Send client association control request to all other (BLOCK) ")
        controller.beerocks_cli_command('client_disallow {} {}'.format(sta.mac,
                                                                       agent.radios[0].mac))
        time.sleep(1)

        # Check logs repeater 1 wlan0 got client disallow request
        debug("Confirming Client Association Control Request message was received (BLOCK)")
        self.check_log(agent.radios[0],
                       r"Got client disallow request for {}".format(sta.mac), timeout=20)

        # TODO client blocking not implemented in dummy bwl

        # Associate with other radio
        sta.wifi_disconnect(agent.radios[0].vaps[0])
        sta.wifi_connect(agent.radios[1].vaps[0])

        time.sleep(1)

        # Check in connection map for repeater 1 wlan2
        conn_map = controller.get_conn_map()
        map_agent = conn_map[agent.mac]

        map_radio1 = map_agent.radios[agent.radios[1].mac]
        map_vap1 = map_radio1.vaps[agent.radios[1].vaps[0].bssid]

        if sta.mac not in map_vap1.clients:
            self.fail("client {} not in conn_map, clients: {}".format(sta.mac, map_vap1.clients))

        # repeater 1 wlan0
        map_radio0 = map_agent.radios[agent.radios[0].mac]
        map_vap0 = map_radio0.vaps[agent.radios[0].vaps[0].bssid]
        if sta.mac in map_vap0.clients:
            self.fail("client {} still in conn_map, clients: {}".format(sta.mac, map_vap0.clients))

        # Beerocks CLI client_allow repeater 1 wlan0
        debug("Send client association control request to the chosen BSSID (UNBLOCK)")
        print('client_allow {} {}'.format(sta.mac, agent.radios[0].mac))
        controller.beerocks_cli_command('client_allow {} {}'.format(sta.mac, agent.radios[0].mac))

        time.sleep(1)

        # Check logs repeater 1 wlan0 got client allow request
        debug("Confirming Client Association Control Request message was received (UNBLOCK)")
        self.check_log(agent.radios[0],
                       r"Got client allow request for {}".format(sta.mac), timeout=20)

        # Associate with other radio implies disassociate from first
        sta.wifi_connect_check(agent.radios[0].vaps[0])

        time.sleep(1)

        # Check in connection map for repeater 1 wlan2
        conn_map = controller.get_conn_map()
        map_agent = conn_map[agent.mac]

        map_radio1 = map_agent.radios[agent.radios[1].mac]
        map_vap1 = map_radio1.vaps[agent.radios[1].vaps[0].bssid]
        if sta.mac in map_vap1.clients:
            self.fail("client {} still in conn_map, clients: {}".format(sta.mac, map_vap1.clients))

        # repeater 1 wlan0
        map_radio0 = map_agent.radios[agent.radios[0].mac]
        map_vap0 = map_radio0.vaps[agent.radios[0].vaps[0].bssid]
        if sta.mac not in map_vap0.clients:
            self.fail("client {} not in conn_map, clients: {}".format(sta.mac, map_vap0.clients))

        sta.wifi_disconnect(agent.radios[0].vaps[0])
