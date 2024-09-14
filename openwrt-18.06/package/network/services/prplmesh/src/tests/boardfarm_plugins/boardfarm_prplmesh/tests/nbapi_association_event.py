
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from capi import tlv
from opts import debug

import time
from datetime import datetime, timedelta
import dateutil.parser
import pytz
import re


class NbapiAssociationEvent(PrplMeshBaseTest):
    '''
    Test for NBAPI Device.WiFi.DataElements.Network.Device.Radio.AssociationEvent object.
    This object describes an event generated when a STA associates to a BSS.
    '''

    def runTest(self):
        try:
            controller = self.dev.lan.controller_entity
            agent = self.dev.DUT.agent_entity
            vap1 = agent.radios[1].vaps[0]
            sta = self.dev.wifi
        except AttributeError as ae:
            raise SkipTest(ae)

        self.dev.DUT.wired_sniffer.start(self.__class__.__name__ + "-" + self.dev.DUT.name)
        self.configure_ssids(["NbapiAssociationEvent"])
        time.sleep(3)

        sta.wifi_connect(vap1)
        debug("Send Associated STA Link Metrics Query message")
        mid = controller.ucc_socket.dev_send_1905(agent.mac,
                                                  self.ieee1905['eMessageType']
                                                  ['ASSOCIATED_STA_LINK_METRICS_QUERY_MESSAGE'],
                                                  tlv(self.ieee1905['eTlvTypeMap']
                                                      ['TLV_STAMAC_ADDRESS_TYPE'],
                                                      sta.mac))
        association_time = datetime.now()
        association_time = pytz.utc.localize(association_time)
        time.sleep(3)

        debug("STA sends a valid Association Request frame to MAUT")
        self.check_cmdu_type_single("Associated STA Link Metrics Response",
                                    self.ieee1905['eMessageType']
                                    ['ASSOCIATED_STA_LINK_METRICS_RESPONSE_MESSAGE'],
                                    agent.mac, controller.mac, mid)

        debug("Topology map after settings:")
        topology = self.get_topology()
        for device in topology.values():
            print(device)

        association_data_path = "Device.WiFi.DataElements."\
            + "AssociationEvent.AssociationEventData"
        association_data_list = controller.nbapi_get_list_instances(association_data_path)
        event_present = False
        for assoc_data in association_data_list:
            bssid = controller.nbapi_get_parameter(assoc_data, "BSSID")
            sta_mac = controller.nbapi_get_parameter(assoc_data, "MACAddress")
            status_code = controller.nbapi_get_parameter(assoc_data, "StatusCode")
            time_stamp = controller.nbapi_get_parameter(assoc_data, "TimeStamp")
            time_nbapi = dateutil.parser.isoparse(time_stamp)
            debug(
                f'AssociationEventData created for client {sta_mac} '
                f'connected to {bssid} at {time_stamp}')
            # Check only last added AssociationEvents
            if time_nbapi - timedelta(seconds=3) <= association_time and\
                    association_time <= time_nbapi + timedelta(seconds=3):
                event_present = True
                assert bssid == vap1.bssid, f"Wrong value for BSSID {bssid}, expect {vap1.bssid}"
                assert sta_mac == sta.mac, f"Wrong value for MACAddress {sta_mac}, expect {sta.mac}"
                assert status_code == 0, f"StatusCode should be 0 not {status_code}"

                time_format = r'\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}.\d+(Z|[-+]\d{2}:\d{2})'
                if re.match(time_format, time_stamp) is None:
                    self.fail(f'Fail. NBAPI time stamp has unncorrect format: {time_stamp}')
                self.get_nbapi_ht_capabilities(assoc_data)
                self.get_nbapi_vht_capabilities(assoc_data)
        assert 0 < event_present and event_present <= 2,\
            f"Wrong amount of AssociationEvents [{event_present}] registered for client: {sta.mac}"

        sta.wifi_disconnect(vap1)
        self.check_log(
            controller, f"client disconnected, client_mac={sta_mac}, bssid={bssid}", timeout=10)
        association_data_list_2 = controller.nbapi_get_list_instances(association_data_path)
        assert association_data_list_2 == association_data_list, \
            "AssociationEventData object disappears after disconnect"
