###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

import pexpect

from boardfarm.devices.debian_wifi import DebianWifi
from boardfarm.devices import connection_decider
from environment import VirtualAPHostapd, _device_reset_console, _iperf_throughput


class PrplMeshStation(DebianWifi):
    """Client of prplMesh enabled Access Point."""

    linesep = "\r"
    model = "prplWRT_STA"
    prompt = ['.*:~', '/.*#']

    def __init__(self, *args, **kwargs):
        """Init station and wlan iface."""
        self.args = args
        self.kwargs = kwargs

        config = kwargs.get("config", kwargs)
        self.connection_type = config.get("connection_type", None)
        self.station_ip_wifi = config.get("station_ip_wifi", None)

        ipaddr = config.get("station_ip", None)
        station_username = config.get("station_username", "root")
        station_pw = config.get("station_pw", None)

        if not ipaddr:
            self.connection = connection_decider.connection(device=self,
                                                            conn_type=self.connection_type,
                                                            **kwargs)
            self.connection.connect()

        self.consoles = [self]

        super().__init__(*args, **kwargs,
                         ipaddr=ipaddr,
                         username=station_username,
                         password=station_pw)

        self.iface_dut = self.iface_wifi = self.kwargs.get(
            'iface', 'wlan0')
        self.driver_name = config.get("driver", "nl80211,wext")
        self.mac = self.get_mac()
        self.associated_vap = None

        # kill all wpa_supplicant relevant to active interface
        self.wifi_disconnect(None)
        # Turn on and off wlan iface just in case
        self.disable_and_enable_wifi()

    def station_command(self, *command: str) -> bytes:
        """Execute `command` in device and return its output."""
        _device_reset_console(self)

        self.sendline(" ".join(command))
        self.expect(self.prompt, timeout=10)
        return self.before

    def wifi_connect(self, vap: VirtualAPHostapd,
                     vap_passphrase: str = 'prplmesh_pass') -> bool:
        """Connect to the Access Point. Return True if successful."""
        config_file_name = "boardfarm_tmp.conf"
        config_file_path = "/tmp/{}".format(config_file_name)
        # Create network configuration for SSID
        bssid = "bssid={}".format(vap.bssid)
        ssid = "ssid=\"{}\"".format(vap.get_ssid())
        key = "psk=\"{}\"".format(vap_passphrase)
        network_config = "network={{\n{}\n{}\n{}\n}}".format(bssid, ssid, key)
        # Clean up previous configuration
        self.sendline("rm -f \"{}\"".format(config_file_path))
        self.expect(self.prompt)
        self.sendline("echo \'{}\' > \"{}\"".format(network_config, config_file_path))
        self.expect(self.prompt)
        # Start wpa_supplicant with created configuration
        # Typical coommand on RPI: wpa_supplicant -B -c/tmp/temp.conf -iwlan0 -Dnl80211,wext
        self.sudo_sendline("wpa_supplicant -B -D {} -i {} -c {}".format(
            self.driver_name, self.iface_wifi, config_file_path))
        self.expect("Successfully initialized wpa_supplicant")
        self.associated_vap = vap
        return bool(self.match)

    def wifi_disconnect(self, vap: VirtualAPHostapd) -> bool:
        assert vap == self.associated_vap
        super().wifi_disconnect()
        self.associated_vap = None
        return True

    def wifi_connect_check(self, vap: VirtualAPHostapd) -> bool:
        """Connect to a SSID and verify WIFI connectivity"""
        for _ in range(5):
            self.wifi_connect(vap)
            self.expect(pexpect.TIMEOUT, timeout=10)
            verify_connect = self.wifi_connectivity_verify(vap)
            if verify_connect:
                break
            else:
                self.wifi_disconnect(None)
        return verify_connect

    def wifi_connectivity_verify(self, vap):
        """Verify that wifi is connected. Return bool"""
        self.sendline("iw %s link" % self.iface_wifi)
        matched = self.expect([f"Connected to {vap.bssid}",
                               "Not connected", pexpect.TIMEOUT])
        return matched == 0

    def get_mac(self) -> str:
        """Get MAC of STA iface"""

        self.sendline("iw {} info".format(self.iface_dut))
        # We are looking for MAC definition of STA
        # wdev 0x1
        # addr 96:4e:c9:cc:7a:2c
        # type managed
        self.expect("addr (?P<mac>..:..:..:..:..:..)\r\n\t(type|ssid)")
        return self.match.group('mac')

    def iperf_throughput(self, to_dut: bool, duration: int = 5, protocol: str = 'tcp',
                         bitrate: int = 0, omit: int = 2, num_streams: int = 5,
                         print_output: bool = False) -> float:
        server_hostname = self.station_ip_wifi
        self.station_command('iperf3', '--daemon', '-s', '-B', server_hostname, '-J', '-1')
        return _iperf_throughput(server_hostname, to_dut, duration,
                                 protocol, omit, bitrate,
                                 num_streams, print_output)
