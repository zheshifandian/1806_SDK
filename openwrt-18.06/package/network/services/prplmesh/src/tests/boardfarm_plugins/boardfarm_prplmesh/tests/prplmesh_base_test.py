# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from typing import Any, Dict, Union, Callable, NoReturn

from boardfarm.exceptions import SkipTest
from boardfarm.tests import bft_base_test
from capi import tlv
from opts import debug
import connmap
import environment as env
import os
import sniffer
import subprocess
import time
import re
import yaml


class PrplMeshBaseTest(bft_base_test.BftBaseTest):
    """PrplMesh base test case, no actual testing.

    Contains common methods used by other(derived) prplmesh test cases.
    """

    ieee1905 = {}

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        if not self.ieee1905:
            self.get_tvls()

    def startMarker(self):
        """Calls method with the same name in base class and then prints current topology.

        To make sure all devices are always in a known state before the test starts, clears
        existing SSID configuration on all agents. If required, a particular test must configure
        the SSIDs for that test (as part of the test logic).

        This method is called right before the test.
        """
        try:
            super().startMarker()

            '''Clear existing SSID configuration on all agents.'''
            self.configure_ssids([])

            debug("Current network topology:")
            topology = self.get_topology()
            for value in topology.values():
                debug(value)
            for dev in self.dev.devices:
                # call checkpoint on any controller or agent:
                if getattr(dev.obj, "role", None):
                    dev.obj.get_active_entity().checkpoint()

        except Exception as e:
            debug("Failed to start test:\n{}".format(e))
            raise e

    def get_tvls(self):
        debug("Getting tlvs from yaml files")
        path = os.getcwd() + os.sep + os.pardir + '/framework/tlvf/yaml/tlvf'
        for folder in os.listdir(path):
            for filename in os.listdir(os.path.join(path, folder)):
                with open(os.path.join(path, folder, filename)) as safe_file:
                    mid = yaml.safe_load(safe_file)
                    namespace = filename[:-5]
                    if namespace in mid:
                        for k, v in mid[namespace].items():
                            mid[k] = v
                        del mid[namespace]
                    self.ieee1905[namespace] = mid

    def check_log(self, entity_or_radio: Union[env.ALEntity, env.Radio], regex: str,
                  start_line: int = 0, timeout: float = 0.6, fail_on_mismatch: bool = True) -> bool:
        result, line, match = entity_or_radio.wait_for_log(regex, start_line, timeout,
                                                           fail_on_mismatch=fail_on_mismatch)
        if fail_on_mismatch and not result:
            raise Exception
        return result, line, match

    def prplmesh_status_check(self, entity_or_radio: Union[env.ALEntity, env.Radio]) -> bool:
        """Check prplMesh status by executing status command to initd service.
        Return True if operational.
        """
        result = entity_or_radio.prprlmesh_status_check()
        if not result:
            raise Exception
        return result

    def check_cmdu(
        self, msg: str, match_function: Callable[[sniffer.Packet], bool]
    ) -> [sniffer.Packet]:
        """Verify that the wired_sniffer has captured a CMDU that satisfies match_function.

        Mark failure if no satisfying packet is found.

        Parameters
        ----------
        msg: str
            Message to show in case of failure. It is formatted in a context like
            "No CMDU <msg> found".

        match_function: Callable[[sniffer.Packet], bool]
            A function that returns True if it is the expected packet. It is called on every packet
            returned by get_packet_capture.

        Returns
        -------
        [sniffer.Packet]
            The matching packets.
        """
        debug("Checking for CMDU {}".format(msg))
        result = self.dev.DUT.wired_sniffer.get_cmdu_capture(match_function)
        assert result, "No CMDU {} found".format(msg)
        return result

    def check_cmdu_type(
        self, msg: str, msg_type: int, eth_src: str, eth_dst: str = None, mid: int = None
    ) -> [sniffer.Packet]:
        """Verify that the wired sniffer has captured a CMDU.

        Mark failure if the CMDU is not found.

        Parameters
        ----------
        msg: str
            Message to show in case of failure. It is formatted in a context like
            "No CMDU <msg> found".

        msg_type: int
            CMDU message type that is expected.

        eth_src: str
            MAC address of the sender that is expected.

        eth_dst: str
            MAC address of the destination that is expected. If omitted, the IEEE1905.1 multicast
            MAC address is used.

        mid: int
            Message Identifier that is expected. If omitted, the MID is not checked.

        Returns
        -------
        [sniffer.Packet]
            The matching packets.
        """
        debug("Checking for CMDU {} (0x{:04x}) from {} to {} mid {}"
              .format(msg, msg_type, eth_src,
                      eth_dst if eth_dst else "Multicast",
                      mid if mid else "Any"))
        result = self.dev.DUT.wired_sniffer.get_cmdu_capture_type(msg_type, eth_src, eth_dst, mid)
        assert result, "No CMDU {} found".format(msg)
        return result

    def check_cmdu_type_single(
        self, msg: str, msg_type: int, eth_src: str, eth_dst: str = None, mid: int = None
    ) -> sniffer.Packet:
        '''Like check_cmdu_type, but also check that only a single CMDU is found.'''
        debug("Checking for single CMDU {} (0x{:04x}) from {} to {} mid {}"
              .format(msg, msg_type, eth_src,
                      eth_dst if eth_dst else "Multicast",
                      mid if mid else "Any"))
        cmdus = self.check_cmdu_type(msg, msg_type, eth_src, eth_dst, mid)
        assert len(cmdus) == 1, \
            "Multiple CMDUs {} found:\n {}".format(msg, '\n'.join([str(cmdu) for cmdu in cmdus]))
        return cmdus[0]

    def check_no_cmdu_type(
        self, msg: str, msg_type: int, eth_src: str, eth_dst: str = None
    ) -> NoReturn:
        '''Like check_cmdu_type, but check that *no* machting CMDU is found.'''
        debug("Checking for no CMDU {} (0x{:04x}) from {}".format(msg, msg_type, eth_src))
        result = self.dev.DUT.wired_sniffer.get_cmdu_capture_type(msg_type, eth_src, eth_dst)
        if result:
            for packet in result:
                debug("  {}".format(packet))
            assert False, "Unexpected CMDU {}".format(msg)

    def check_cmdu_has_tlvs(
        self, packet: sniffer.Packet, tlv_type: int
    ) -> [sniffer.Tlv]:
        '''Check that the packet has at least one TLV of the given type.

        Mark failure if no TLV of that type is found.

        Parameters
        ----------
        packet: Union[sniffer.Packet]
            The packet to verify. If it is empty or it is not an IEEE1905
            packet, an AssertionError is raised.

        tlv_type: int
            The type of TLV to look for.

        Returns
        -------
        [sniffer.Tlv]
            List of TLVs of the requested type. An AssertionError is raised if
            no TLV is found.
        '''
        assert packet, "No packet found"
        assert packet.ieee1905, "Packet is not IEEE1905: {}".format(packet)
        tlvs = [tlv for tlv in packet.ieee1905_tlvs if tlv.tlv_type == tlv_type]
        if not tlvs:
            debug("  {}".format(packet))
            raise ValueError("No TLV of type 0x{:02x} found in packet".format(tlv_type))
        return tlvs

    def check_cmdu_has_tlv_single(
        self, packet: Union[sniffer.Packet, None], tlv_type: int
    ) -> sniffer.Tlv:
        '''Like check_cmdu_has_tlvs, but also check that only one TLV of that type is found.'''
        tlvs = self.check_cmdu_has_tlvs(packet, tlv_type)
        if len(tlvs) > 1:
            debug("  {}".format(packet))
            assert False, "More than one ({}) TLVs of type 0x{:02x} found".format(
                len(tlvs), tlv_type)
        return tlvs[0]

    def check_cmdu_has_tlvs_exact(
        self, packet: Union[sniffer.Packet, None], tlvs: [sniffer.Tlv]
    ) -> NoReturn:
        '''Check that the CMDU has exactly the TLVs given.'''
        assert packet, "Packet not found"
        assert packet.ieee1905, "Packet is not IEEE1905: {}".format(packet)

        packet_tlvs = list(packet.ieee1905_tlvs)
        for t in tlvs:
            if t in packet_tlvs:
                packet_tlvs.remove(t)
            else:
                assert False, "Packet misses tlv:\n {}".format(str(t))

        assert not packet_tlvs, "Packet has unexpected tlvs:\n {}".format(
            "\n ".join(map(str, packet_tlvs)))

    def check_cmdu_contains_tlvs(
        self, packet: Union[sniffer.Packet, None], tlvs: [sniffer.Tlv]
    ) -> NoReturn:
        '''Check that the CMDU contains the TLVs given.'''
        assert packet, "Packet not found"
        assert packet.ieee1905, "Packet is not IEEE1905: {}".format(packet)

        packet_tlvs = list(packet.ieee1905_tlvs)
        for t in tlvs:
            assert t in packet_tlvs, "Packet misses tlv:\n {}".format(str(t))

    def checkpoint(self) -> None:
        '''Checkpoint the current state.

        Any subsequent calls to functions that query cumulative state
        (e.g. log files, packet captures) will not match any of the state that was
        accumulated up till now, but only afterwards.
        '''
        for dev in self.dev.devices:
            # call checkpoint on any controller or agent:
            if getattr(dev.obj, "role", None):
                dev.obj.get_active_entity().checkpoint()
        self.dev.DUT.wired_sniffer.checkpoint()

    def fail(self, msg: str):
        '''Throw an exception message.'''
        FAIL = '\033[91m'
        END = '\033[0m'
        msg = FAIL + msg + END
        raise Exception(msg)

    def safe_check_obj_attribute(self, obj: object, attrib_name: str,
                                 expected_val: Any, fail_str: str) -> NoReturn:
        """Check if expected attrib exists first, fail test if it does not exist"""
        try:
            if getattr(obj, attrib_name) != expected_val:
                self.fail(fail_str)
        except AttributeError:
            self.fail("{} has no attribute {}".format(type(obj).__name__, attrib_name))

    def base_test_client_capability_query(self, sta: env.Station):
        try:
            agent = self.dev.DUT.agent_entity
            controller = self.dev.lan.controller_entity
        except AttributeError as ae:
            raise SkipTest(ae)

        mid = controller.ucc_socket.dev_send_1905(
            agent.mac,
            self.ieee1905['eMessageType']['CLIENT_CAPABILITY_QUERY_MESSAGE'], tlv(
                self.ieee1905['eTlvTypeMap']['TLV_CLIENT_INFO'],
                '{} {}'.format(agent.radios[0].mac, sta.mac)))

        time.sleep(1)

        query = self.check_cmdu_type_single(
            "client capability query",
            self.ieee1905['eMessageType']['CLIENT_CAPABILITY_QUERY_MESSAGE'],
            controller.mac, agent.mac, mid)

        query_tlv = self.check_cmdu_has_tlv_single(query,
                                                   self.ieee1905['eTlvTypeMap']['TLV_CLIENT_INFO'])
        self.safe_check_obj_attribute(query_tlv, 'client_info_mac_addr', sta.mac,
                                      "Wrong mac address in query")
        self.safe_check_obj_attribute(query_tlv, 'client_info_bssid',
                                      agent.radios[0].mac,
                                      "Wrong bssid in query")

        report = self.check_cmdu_type_single(
            "client capability report",
            self.ieee1905['eMessageType']['CLIENT_CAPABILITY_REPORT_MESSAGE'],
            agent.mac, controller.mac, mid)

        client_info_tlv = self.check_cmdu_has_tlv_single(
            report,
            self.ieee1905['eTlvTypeMap']['TLV_CLIENT_INFO'])
        self.safe_check_obj_attribute(client_info_tlv, 'client_info_mac_addr', sta.mac,
                                      "Wrong mac address in report")
        self.safe_check_obj_attribute(client_info_tlv, 'client_info_bssid',
                                      agent.radios[0].mac,
                                      "Wrong bssid in report")
        return report

    def get_device_by_name(self, device_name: str) -> env.ALEntity:
        try:
            return [_.obj for _ in self.dev.devices if _.obj.name == device_name][0]
        except IndexError as ae:
            raise SkipTest(ae)

    def check_topology_notification(self, eth_src: str, neighbors: list,
                                    sta: env.Station, event: env.StationEvent, bssid: str) -> bool:
        """Verify topology notification reliable multicast - given a source mac and
           a list of neighbors macs, check that exactly one relayed multicast CMDU
           was sent to the IEEE1905.1 multicast MAC address, and a single unicast
           CMDU with the relayed bit unset to each of the given neighbors destination MACs.
           Verify correctness of the association event TLV inside the topology notification.
           Mark failure if any of the above conditions isn't met.

        Parameters
        ----------

        eth_src: str
            source AL MAC (origin of the topology notification)

        neighbors: list
            destination AL MACs (destinations of the topology notification)

        sta: environment.Station
            station mac

        event: environment.StationEvent
            station event - CONNECTED / DISCONNECTED

        bssid: str
            bssid Multi-AP Agent BSSID

        Returns:
        bool
            True for valid topology notification, False otherwise
        """
        mcasts_all = self.check_cmdu_type("topology notification", 0x1, eth_src)

        def filter_mcast_notifications(mcast) -> bool:
            try:
                assoc_event_tlv = self.check_cmdu_has_tlv_single(
                    mcast,
                    self.ieee1905['eTlvTypeMap']['TLV_CLIENT_ASSOCIATION_EVENT'])
            except ValueError:
                # Skip notifications that don't have the association event tlv.
                return False
            return assoc_event_tlv.assoc_event_client_mac == sta.mac and \
                assoc_event_tlv.assoc_event_agent_bssid == bssid and \
                int(assoc_event_tlv.assoc_event_flags, 16) == event.value

        mcasts = list(filter(filter_mcast_notifications, mcasts_all))
        if not mcasts:
            self.fail(f"No matching topology notification!\n {mcasts}")
            return False

        if len(mcasts) > 1:
            self.fail(f"Multiple topology notification found!\n {mcasts}")
            return False

        mcast = mcasts[0]
        # relay indication should be set
        if not mcast.ieee1905_relay_indicator:
            self.fail("Multicast topology notification should be relayed")
            return False

        mid = mcast.ieee1905_mid
        for eth_dst in neighbors:
            ucast = self.check_cmdu_type_single("topology notification",
                                                self.ieee1905['eMessageType']
                                                ['TOPOLOGY_NOTIFICATION_MESSAGE'],
                                                eth_src, eth_dst, mid)
            if ucast.ieee1905_relay_indicator:
                self.fail("Unicast topology notification should not be relayed")
                return False

        return True

    def get_nbapi_ht_capabilities(self, ht_cap_path: str) -> Dict[str, int]:
        '''Get HT Capabilities of the object.

        Parameters
        ----------
        ht_cap_path: str
            Path to HT Capabilties object.
            Example:
            "Device.WiFi.DataElements.AssociationEvent.AssociationEventData.8"

        Returns
        -------
        Dict[str, int]
            A dictionary with HT Capabilities of NBAPI object.
        '''
        controller = self.dev.lan.controller_entity
        ht_cap_path += ".HTCapabilities"
        ht_caps = {}

        ht_caps['rx_ss'] = controller.nbapi_get_parameter(ht_cap_path, "rx_spatial_streams")
        ht_caps['tx_ss'] = controller.nbapi_get_parameter(ht_cap_path, "tx_spatial_streams")
        ht_caps['gi_20_mhz'] = controller.nbapi_get_parameter(ht_cap_path, "GI_20_MHz")
        ht_caps['gi_40_mhz'] = controller.nbapi_get_parameter(ht_cap_path, "GI_40_MHz")
        ht_caps['ht_40_mhz'] = controller.nbapi_get_parameter(ht_cap_path, "HT_40_Mhz")
        return ht_caps

    def get_nbapi_vht_capabilities(self, vht_cap_path: str) -> Dict[str, int]:
        '''Get VHT Capabilities of the object.

        Parameters
        ----------
        vht_cap_path: str
            Path to VHT Capabilties object.
            Example:
            "Device.WiFi.DataElements.AssociationEvent.AssociationEventData.9"

        Returns
        -------
        Dict[str, int]
            A dictionary with VHT Capabilities of NBAPI object.
        '''
        controller = self.dev.lan.controller_entity
        vht_cap_path += ".VHTCapabilities"
        vht_caps = {}

        vht_caps['rx_ss'] = controller.nbapi_get_parameter(vht_cap_path, "rx_spatial_streams")
        vht_caps['tx_ss'] = controller.nbapi_get_parameter(vht_cap_path, "tx_spatial_streams")
        vht_caps['gi_80_mhz'] = controller.nbapi_get_parameter(vht_cap_path, "GI_80_MHz")
        vht_caps['gi_160_mhz'] = controller.nbapi_get_parameter(vht_cap_path, "GI_160_MHz")
        vht_caps['vht_80_80_mhz'] = controller.nbapi_get_parameter(vht_cap_path, "VHT_80_80_MHz")
        vht_caps['vht_160_mhz'] = controller.nbapi_get_parameter(vht_cap_path, "VHT_160_MHz")
        vht_caps['su_beamformer'] = controller.nbapi_get_parameter(vht_cap_path, "SU_beamformer")
        vht_caps['mu_beamformer'] = controller.nbapi_get_parameter(vht_cap_path, "MU_beamformer")
        vht_caps['vht_tx_mcs'] = controller.nbapi_get_parameter(vht_cap_path, "VHT_Tx_MCS")
        vht_caps['vht_rx_mcs'] = controller.nbapi_get_parameter(vht_cap_path, "VHT_Rx_MCS")
        return vht_caps

    def get_topology(self) -> Dict[str, connmap.MapDevice]:
        '''Get the topology.

        Returns a list of devices, the rest of the topology is a tree under it.

        Uses the northbound API to get this information.
        '''
        controller = self.dev.lan.controller_entity

        data_model = controller.nbapi_get_data_model()
        map_devices = {}

        device_regex = r'^Device\.WiFi\.DataElements\.Network\.Device\.\d+\.$'

        devices = [obj_path for obj_path in data_model if re.search(
            device_regex, obj_path)]

        for device in devices:
            map_device = connmap.MapDevice(data_model[device]["ID"])
            map_device.path = device[:-1]  # Cut off the dot.
            map_devices[map_device.mac] = map_device

            dot = map_device.path.rfind('.')
            device_indx = map_device.path[dot + 1:]
            radio_regex = device_regex[:-6] + device_indx + r'\.Radio\.\d+\.$'

            radios = [obj_path for obj_path in data_model if re.search(
                radio_regex, obj_path)]

            for radio in radios:
                map_radio = map_device.add_radio(data_model[radio]["ID"])
                map_radio.path = radio[:-1]

                dot = map_radio.path.rfind('.')
                radio_indx = map_radio.path[dot + 1:]
                bss_regex = radio_regex[:-6] + radio_indx + r'\.BSS\.\d+\.$'

                vaps = [obj_path for obj_path in data_model if re.search(
                    bss_regex, obj_path)]

                for vap in vaps:
                    map_vap = map_radio.add_vap(data_model[vap]["BSSID"],
                                                data_model[vap]["SSID"])
                    map_vap.path = vap[:-1]

                    dot = map_vap.path.rfind('.')
                    vap_indx = map_vap.path[dot + 1:]
                    sta_regex = bss_regex[:-6] + vap_indx + r'\.STA\.\d+\.$'

                    clients = [obj_path for obj_path in data_model if re.search(
                        sta_regex, obj_path)]

                    for client in clients:
                        map_client = map_vap.add_client(data_model[client]["MACAddress"])
                        map_client.path = client[:-1]

                interface_regex = r'^Device\.WiFi\.DataElements\.Network' + \
                    rf'\.Device\.{device_indx}\.Interface\.\d+\.$'

                interfaces = [obj_path for obj_path in data_model if re.search(
                    interface_regex, obj_path)]

                for interface in interfaces:
                    map_interface = map_device.add_interface(data_model[interface]["MACAddress"])
                    map_interface.path = interface[:-1]

                    dot = map_interface.path.rfind('.')
                    interface_indx = map_interface.path[dot + 1:]
                    neighbor_regex = interface_regex[:-6] + interface_indx + r'\.Neighbor\.\d+\.$'

                    neighbors = [obj_path for obj_path in data_model if re.search(
                        neighbor_regex, obj_path)]

                    for neighbor in neighbors:
                        map_neighbor = map_interface.add_neighbor(data_model[neighbor]["ID"])
                        map_neighbor.path = neighbor[:-1]

        return map_devices

    def configure_ssids_clear(self):
        '''Clear the SSID configuration.

        Removes all Device.WiFi.DataElements.Network.AccessPoint instances in the northbound API.
        '''
        controller = self.dev.lan.controller_entity
        access_points = controller.nbapi_get_list_instances(
            'Device.WiFi.DataElements.Network.AccessPoint')
        for access_point_path in access_points:
            controller.nbapi_command(access_point_path, '_del', {})

    def wait_ubus_object(self, path: str, timeout: int = 10):
        deadline = time.monotonic() + timeout
        controller = self.dev.lan.controller_entity
        ubus_obj = None
        while True:
            try:
                ubus_obj = controller.nbapi_command(path, "_get")
            except subprocess.CalledProcessError as e:
                debug(e)
            finally:
                if ubus_obj:
                    return True
                if time.monotonic() < deadline:
                    time.sleep(.3)
                else:
                    return ubus_obj

    def wait_radios_enabled(self, timeout: int = 10):
        """Waits and checks in .3 intervals until the timeout
        period is reached if all the radios come out as enabled
        on a ubus call.
        Parameters
        ----------

        timeout: int
            maximum waiting period in seconds

        Returns:
        list(bool)
            All radios state on the moment of the exit condition
        """
        deadline = time.monotonic() + timeout
        controller = self.dev.lan.controller_entity
        while True:
            devices_status = []
            topology = self.get_topology()
            for device in topology.values():
                for radio in device.radios.values():
                    enabled = controller.nbapi_get_parameter(radio.path, "Enabled")
                    devices_status.append(enabled)
            if all(devices_status):
                return devices_status
            if time.monotonic() < deadline:
                time.sleep(.3)
            else:
                return devices_status

    def configure_passphrase(self, ssid: str = 'Dummy_ssid',
                             vap_passphrase: str = 'prplmesh_pass'):
        '''Configures a new password on the controller

        Parameters
        ----------
        ssid: str = Dummy_ssid
            The SSID to configure.

        vap_passphrase: str = prplmesh_pass
            The password to configure.
        Returns
        -------
        str
            New password.
        '''
        controller = self.dev.lan.controller_entity
        ap_security_path = self.configure_ssid(ssid, "Fronthaul") + ".Security"

        self.wait_ubus_object(ap_security_path)

        controller.nbapi_set_parameters(ap_security_path,
                                        {"ModeEnabled": "WPA2-Personal"})
        controller.nbapi_set_parameters(ap_security_path,
                                        {"KeyPassphrase": vap_passphrase})

        controller.nbapi_command("Device.WiFi.DataElements.Network", "AccessPointCommit")
        time.sleep(5)
        return vap_passphrase

    def configure_ssid(self, ssid: str, multi_ap_mode: str = "Fronthaul",
                       bands: Dict = None) -> str:
        '''Configure an SSID.

        Adds a Device.WiFi.DataElements.Network.AccessPoint instance and configures it with
        the given SSID, bands and multi AP mode.
        If parameter 'bands' was not passed the SSID will be enabled on all bands.
        If one of the band was not specified its value will be set to false.
        The value of 'multi_ap_mode' can be one of "Fronthaul","Backhaul","Fronthaul+Backhaul".
        By default, multi AP mode set as fronthaul-only in open mode.

        Parameters
        ----------
        ssid: str
            The SSID to configure.

        Returns
        -------
        str
            Path to the Device.WiFi.DataElements.Network.AccessPoint instance.
        '''
        if not bands:
            bands = {"Band5GH": True, "Band6G":  True, "Band5GL": True, "Band2_4G": True}

        controller = self.dev.lan.controller_entity
        params = {"parameters": {
            "MultiApMode": multi_ap_mode,
            "Band5GH": bands.get("Band5GH", False),
            "Band6G": bands.get("Band6G", False),
            "Band5GL": bands.get("Band5GL", False),
            "Band2_4G": bands.get("Band2_4G", False),
            "SSID": ssid,
        }}
        new_inst = controller.nbapi_command(
            "Device.WiFi.DataElements.Network.AccessPoint", "_add", params)
        return "Device.WiFi.DataElements.Network.AccessPoint." + new_inst["name"]

    def configure_ssids(self, ssids: [str], clear_old: bool = True):
        '''Configure SSIDs on all agents.

        Configure all radios on all agents with the given set of ssids. They
        are configured as fronthaul-only, in open mode.

        If clear_old is True, the existing configuration is cleared first. By
        setting it to false, it is possible to add more complicated custom
        configuration first and then call configure_ssids for the simple SSIDs.

        After configuration completes, check that all agents have been updated.

        Uses northbound API.

        Parameters
        ---------
        ssid: [str]
            List of SSIDs to configure. Each SSID will configure an additional
            VAP on the radios.

        clear_old: bool
            If True (default), the existing configuration is cleared before
            adding new SSIDs.
        '''
        if clear_old:
            self.configure_ssids_clear()
        for ssid in ssids:
            self.configure_ssid(ssid)

        self.dev.lan.controller_entity.nbapi_command(
            "Device.WiFi.DataElements.Network", "AccessPointCommit")
        # TODO check that renew was sent to all agents
        # TODO check that all agents have been configured with the SSIDs
        self.wait_radios_enabled()

    def assertEqual(self, path: str, name: str, expected: str):
        ''' Get specified with 'name' parameter of given in 'path' NBAPI object,
            compare this value with expected and raise an error, if they don't match.

        Parameters
        ---------
        path: str
            Path to NBAPI object which contains a parameter to check.
            Return parameter should be of type int.
        name: str
            Name of parameter to compare.
        expected: str
            Expected value (casted to int).
        '''
        controller = self.dev.lan.controller_entity
        actual = controller.nbapi_get_parameter(path, name)
        assert actual == int(expected), \
            f"Wrong value for {name}, actual: {actual}, expected: {expected}"

    def send_and_check_policy_config_metric_reporting(self, controller,
                                                      agent, include_sta_traffic_stats=True,
                                                      include_sta_link_metrics=True):
        '''Configure SSIDs on all agents.

        Send multi-ap policy config request with metric reporting policy to agent
        and verifies policy config request was acknowledged by agent

        Parameters
        ---------
        controller: entity
            Device that policy is being sent from
        agent: entity
            Device that policy is sent
        include_sta_traffic_stats: bool
            Sets AP Metric Request policy to include STA Traffic Stats
        include_sta_link_metrics: bool
            Sets AP Metric Request policy to include STA Link Metrics
        '''
        debug("Send multi-ap policy config request with metric reporting policy to agent")
        reporting_value = 0
        if include_sta_traffic_stats:
            reporting_value |= 0x80
        if include_sta_link_metrics:
            reporting_value |= 0x40
        radio_policies = ["{%s 0x00 0x00 0x01 0x%02x}" % (radio.mac, reporting_value)
                          for radio in agent.radios]
        metric_reporting_tlv = tlv(0x8a,
                                   "{0x00 0x%02x %s}" % (len(radio_policies),
                                                         " ".join(radio_policies)))
        mid = controller.dev_send_1905(
            agent.mac,
            self.ieee1905['eMessageType']['MULTI_AP_POLICY_CONFIG_REQUEST_MESSAGE'],
            metric_reporting_tlv)
        time.sleep(1)
        debug("Confirming multi-ap policy config request was acked by agent")
        self.check_cmdu_type_single("ACK", self.ieee1905['eMessageType']['ACK_MESSAGE'],
                                    agent.mac, controller.mac, mid)

    def device_reset_default(self):
        controller = self.dev.lan.controller_entity
        controller.cmd_reply("DEV_RESET_DEFAULT")
        self.checkpoint()

    def device_reset_then_set_config(self):
        '''Resets the controller

        This method is used to put contoller in clear state.
        Also with "DEV_RESET_DEFAULT" command, periodic link metrics
        requests are disabled.

        "DEV_SET_CONFIG" is sent afterwards without any specific settings.
        '''
        controller = self.dev.lan.controller_entity
        agent = self.dev.DUT.agent_entity
        self.device_reset_default()

        controller.cmd_reply(
            "DEV_SET_CONFIG,bss_info1,{} 8x".format(agent.mac))

    @classmethod
    def teardown_class(cls):
        """Teardown method, optional for boardfarm tests."""

        test = cls.test_obj

        for dev in test.dev:
            if dev.model in ['prplWRT_STA', 'STA_dummy'] and dev.associated_vap:
                dev.wifi_disconnect(dev.associated_vap)

        subprocess.call('pkill iperf3', shell=True)

        print("Sniffer - stop")
        test.dev.DUT.wired_sniffer.stop()
        # Send additional Ctrl+C to the device to terminate "tail -f"
        # Which is used to read log from device. Required only for tests on HW
        try:
            test.dev.DUT.agent_entity.device.send('\003')
        except AttributeError:
            # If AttributeError was raised - we are dealing with dummy devices.
            # We don't have to additionaly send Ctrl+C for dummy devices.
            pass
