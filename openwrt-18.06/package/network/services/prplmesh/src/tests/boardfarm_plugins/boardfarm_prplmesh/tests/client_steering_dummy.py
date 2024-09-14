# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from opts import debug
import environment as env
import time


class ClientSteeringDummy(PrplMeshBaseTest):
    """
        This test reproduces and tests client steering
        Also, it checks MultiAPSteeringSummaryStats NBAPI object

        Devices used in test setup:
        STA1 - WIFI repeater
        AP1 - Agent1 [DUT]
        AP2 - Agent2 [LAN2]

        GW - Controller

        Dummy STA is connected to AP1 wlan0
        A steer request is sent to AP1 radio 1
        Dummy STA is disconnected from wlan0
        Dummy STA is connected to AP1 wlan2
        Original values of MultiAPSteeringSummaryStats parameters are saved
        All APs radios should get a disallow request, except for AP1 radio 1
        Client Steering Request message is checked of AP1 radio 0
        BTM Report message is checked on GW controller
        ACK message is checked on AP1 radio 0
        GW controller should have a steering a disconnected message refering to STA1
        After 25 seconds all disallowed APs radios should have an allow message
        Value of BTMSuccesses should be incremented by one
    """

    def runTest(self):
        # Locate test participants
        try:
            sta = self.dev.wifi
            agent1 = self.dev.DUT.agent_entity
            agent2 = self.dev.lan2.agent_entity

            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        sniffer = self.dev.DUT.wired_sniffer
        sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)

        self.configure_ssids(['ClientSteeringDummy'])

        self.checkpoint()

        debug("Connect dummy STA to wlan0")
        sta.wifi_connect(agent1.radios[0].vaps[0])

        time.sleep(1)

        debug("Check dummy STA connected to repeater1 radio")
        self.check_topology_notification(agent1.mac,
                                         [controller.mac, agent2.mac],
                                         sta, env.StationEvent.CONNECT,
                                         agent1.radios[0].vaps[0].bssid)

        # Save MultiAPSteeringSummaryStats values before client steering
        steer_summ_stats = controller.nbapi_get(
            "Device.WiFi.DataElements.Network.MultiAPSteeringSummaryStats")

        self.checkpoint()

        debug("Send steer request ")
        controller.nbapi_command("Device.WiFi.DataElements.Network", "ClientSteering",
                                 {"station_mac": sta.mac,
                                  "target_bssid": agent1.radios[1].mac})

        time.sleep(1)
        debug("Disconnect dummy STA from wlan0")
        sta.wifi_disconnect(agent1.radios[0].vaps[0])

        time.sleep(1)
        self.check_topology_notification(agent1.mac,
                                         [controller.mac, agent2.mac],
                                         sta, env.StationEvent.DISCONNECT,
                                         agent1.radios[0].vaps[0].bssid)

        debug("Connect dummy STA to wlan2")
        sta.wifi_connect(agent1.radios[1].vaps[0])

        time.sleep(1)
        self.check_topology_notification(agent1.mac,
                                         [controller.mac, agent2.mac],
                                         sta, env.StationEvent.CONNECT,
                                         agent1.radios[1].vaps[0].bssid)

        debug("Confirming Client Association Control Request message was received (UNBLOCK)")
        self.check_log(agent1.radios[1], r"Got client allow request")

        debug("Confirming Client Association Control Request message was received (BLOCK)")
        self.check_log(agent1.radios[0], r"Got client disallow request")

        debug("Confirming Client Association Control Request message was received (BLOCK)")
        self.check_log(agent2.radios[0], r"Got client disallow request")

        debug("Confirming Client Association Control Request message was received (BLOCK)")
        self.check_log(agent2.radios[1], r"Got client disallow request")

        debug("Confirming Client Steering Request message was received - mandate")
        self.check_log(agent1.radios[0], r"Got steer request")

        debug("Confirming BTM Report message was received")
        self.check_log(controller, r"CLIENT_STEERING_BTM_REPORT_MESSAGE")

        debug("Confirming ACK message was received")
        self.check_log(agent1.radios[0], r"ACK_MESSAGE")

        debug("Confirm steering success by client connected")
        self.check_log(controller, r"steering successful for sta {}".format(sta.mac))
        self.check_log(controller,
                       r"sta {} disconnected due to steering request".format(sta.mac))

        # Make sure that all blocked agents send UNBLOCK messages at the end of
        # disallow period (default 25 sec)
        time.sleep(25)

        debug("Confirming Client Association Control Request message was received (UNBLOCK)")
        self.check_log(agent1.radios[0], r"Got client allow request")

        debug("Confirming Client Association Control Request message was received (UNBLOCK)")
        self.check_log(agent2.radios[0], r"Got client allow request")

        debug("Confirming Client Association Control Request message was received (UNBLOCK)")
        self.check_log(agent2.radios[1], r"Got client allow request")

        sta.wifi_disconnect(agent1.radios[1].vaps[0])

        # Check MultiAPSteeringSummaryStats values after client steering
        final_steer_summ_stats = controller.nbapi_get(
            "Device.WiFi.DataElements.Network.MultiAPSteeringSummaryStats")
        for param in final_steer_summ_stats:

            debug(f"Checking parameter '{param}', original value is '{steer_summ_stats[param]}'")

            if param == 'BTMSuccesses':
                assert steer_summ_stats[param] + 1 == final_steer_summ_stats[param], \
                    f"Value of '{param}' should be '{steer_summ_stats[param] + 1}'" \
                    f" not '{final_steer_summ_stats[param]}'"
                continue

            """
            TODO: Check other values of MultiAPSteeringSummaryStats parameters
            Other params values are currently set incorrectly, so there is no way to check them yet.
            PPM-1761

            assert steer_summ_stats[param] == fin_steer_summ_stats[param], \
                f"Value of '{param}' should be '{steer_summ_stats[param]}'" \
                f" not '{fin_steer_summ_stats[param]}'"
            """
