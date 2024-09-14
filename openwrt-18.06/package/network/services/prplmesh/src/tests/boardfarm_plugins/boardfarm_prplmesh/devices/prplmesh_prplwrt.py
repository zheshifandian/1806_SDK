###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

import boardfarm
import datetime
import json
import os
import pexpect
import signal
import subprocess
import sys
import time

from .prplmesh_base import PrplMeshBase
from boardfarm.exceptions import CodeError
from boardfarm.devices import connection_decider
from boardfarm.devices.openwrt_router import OpenWrtRouter
from environment import ALEntityPrplWrt, _get_bridge_interface
from ipaddress import IPv4Network, IPv4Address
from sniffer import Sniffer


class PrplMeshPrplWRT(OpenWrtRouter, PrplMeshBase):
    """prplWRT burned device with prplMesh installed."""

    model = "prplWRT"
    prompt = [r'root@[^\s]+:[^\s]+# ']
    wan_iface = "eth1"
    uboot_eth = "eth0_1"
    linesep = "\r"
    agent_entity = None
    controller_entity = None
    beerocks_logs_location = '/tmp/beerocks/logs'

    def __init__(self, *args, **kwargs):
        """Initialize device."""
        self.args = args
        self.kwargs = kwargs
        config = kwargs.get("config", kwargs)
        self.unique_id = os.getenv("RUN_ID")
        if not self.unique_id:
            raise CodeError("RUN_ID not set")

        self.docker_network = config.get("docker_network",
                                         "prplMesh-net-{}".format(self.unique_id))
        self.role = config.get("role", "agent")
        self.connection_type = config.get("connection_type", None)
        self.conn_cmd = config.get("conn_cmd", None)
        self.control_ip = config.get("control_ip", None)
        self.host_ip_to_device = config.get("host_ip_to_device", None)
        self.username = config.get("username", "root")
        self.host_iface_to_device = config.get("iface_to_device")
        if not self.host_iface_to_device:
            raise CodeError("Interface to the device not specified. \
            Please provide the interface on the host that connects to the prplWrt device.")

        self.name = "-".join((config.get("name", "nec-wx3000hp-1"), self.unique_id))
        try:
            self.delay = int(config.get("delay", 70))
        except ValueError as err:
            raise CodeError("Invalid delay specified: {}".format(str(err)))

        # If no WAN IP is set in config file retrieve IP from docker network set in config
        # X.X.X.245 IP will be selected from docker network
        if not self.control_ip:
            self.connection = connection_decider.connection(device=self,
                                                            conn_type="local_serial",
                                                            **kwargs)
            self.connection.connect()
            self.consoles = [self]
            self.logfile_read = sys.stdout
            self.wan_network = self.get_docker_subnet()
            self.control_ip = self.wan_network[+245]
            self.set_iface_ip("br-lan", self.control_ip, self.wan_network.prefixlen)
            self.close()
            self.kill(signal.SIGTERM)
            # Removal of PID is required by pexpect in order to spawn a new process
            # serial connection should be terminated by 2 commands above
            self.pid = None

        self.wired_sniffer = Sniffer(_get_bridge_interface(self.unique_id),
                                     boardfarm.config.output_dir)
        # Disable public key checking.
        # Boards will be reflashed from time to time and it will change their ssh identity.
        conn_cmd = "ssh -o PubkeyAuthentication=no" \
                   " -o StrictHostKeyChecking=no" \
                   " {}@{}".format(self.username, self.control_ip)

        self.connection = connection_decider.connection(device=self,
                                                        conn_type="ssh",
                                                        conn_cmd=conn_cmd)
        self.connection.connect()
        # Append active connection to the general array for logging
        self.consoles = [self]
        # Point what to log as data read from child process of pexpect
        # Result: boardfarm will log communication in separate file
        self.logfile_read = sys.stdout

        # Sync DUT date with the device running boardfarm
        self.set_device_date()

        # We need to add the interface to the actual device to the
        # docker bridge the docker controller is in, to allow them to
        # communicate:
        bridge_interface = _get_bridge_interface(self.unique_id)

        self.add_host_iface_to_bridge(self.host_iface_to_device,
                                      bridge_interface)

        # The IPv4 on the actual device interface needs to be added to
        # the bridged interface
        if self.host_ip_to_device:
            ip, prefixlen = self.host_ip_to_device.split('/')
            self.set_boardfarm_iface_ip(bridge_interface, ip, prefixlen)

        # Remove the logs to make sure we only get the ones from the
        # next prplMesh start:
        self.prplmesh_remove_logs()

        # prplMesh has to be started before creating the ALEntity,
        # since the latter requires the ucc listener to be running.
        self.prplmesh_start_mode(self.role)

        if self.role == "controller":
            self.controller_entity = ALEntityPrplWrt(self, is_controller=True)
        else:
            self.agent_entity = ALEntityPrplWrt(self, is_controller=False)

    def _prplMesh_exec(self, mode: str):
        """Send line to prplmesh initd script."""
        self.sendline("/etc/init.d/prplmesh {}".format(mode))

    def _prplmesh_status_poll(self, timeout: int = 120) -> bool:
        """Poll prplMesh status for timeout time.

        Main agent and wlan0, wlan2 radios should be operational.
        Return True if status is operational and timeout not reached.
        """
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.get_prplMesh_status():
                break
            time.sleep(5)
        else:
            return False
        return True

    def get_prplMesh_status(self) -> bool:
        """ Check prplMesh status. Return True if operational."""
        self.sendline("/etc/init.d/prplmesh status")
        self.expect(
            ["OK Main agent.+"
             "OK {}.+"
             "OK {}".format(
                 self.agent_entity.radios[0].iface_name,
                 self.agent_entity.radios[1].iface_name), pexpect.TIMEOUT],
            timeout=5)
        return self.match is not pexpect.TIMEOUT

    def isalive(self):
        """Check if device is alive.

        States that device is operational and its consoles are accessible.
        """
        return self.check_status()

    def touch(self):
        """Keep consoles alive.

        Purpose is to keep consoles active, so they don't disconnect for long running activities.
        """
        pass

    def get_docker_subnet(self) -> IPv4Network:
        """Get subnet used by docker network."""
        docker_network_inspect_cmd = ('docker', 'network', 'inspect', self.docker_network)
        inspect_raw = subprocess.run(docker_network_inspect_cmd, stdout=subprocess.PIPE)
        if inspect_raw.returncode != 0:
            # Assume network doesn't exist yet. Create it.
            # Raise an exception if it fails (check=True).
            subprocess.run(('docker', 'network', 'create', self.docker_network), check=True,
                           stdout=subprocess.DEVNULL)
            # Inspect again, now raise if it fails (check=True).
            inspect_raw = subprocess.run(docker_network_inspect_cmd, check=True,
                                         stdout=subprocess.PIPE)
        inspect_json = json.loads(inspect_raw.stdout)

        return IPv4Network(inspect_json[0]["IPAM"]["Config"][0]["Subnet"])

    def set_device_date(self):
        """Set device internal date."""
        cmd = f"date -s '@{int(datetime.datetime.now().timestamp())}'"

        self.sendline(cmd)
        self.expect(self.prompt, timeout=10)

    def add_host_iface_to_bridge(self, iface: str, bridge: str):
        """Add specified local interface to the specified bridge.

        Note that this applies to the _local_ device (i.e. the device
        boardfarm runs in), not to the prplwrt device. This can be used
        to add the interface to the prplWrt device to a docker bridge
        for example.

        Raises
        ------
        ExceptionPexpect
            If the operation failed.

        """
        ip_args = ("link set {} master {}".format(iface, bridge))
        self._run_shell_cmd("ip", ip_args.split(" "))

    def set_iface_ip(self, iface: str, ip: IPv4Address, prefixlen: int) -> bool:
        """Set interface IPv4 address."""
        cmd = f"ip a add {ip}/{prefixlen} dev {iface}"

        self.command(cmd)
        self.expect(self.prompt, timeout=10)

    def set_boardfarm_iface_ip(self, iface: str,
                               ip: IPv4Address, prefixlen: int) -> bool:
        """Set boardfarm interface IPv4 address."""
        ip_args = (["a", "add", f"{ip}/{prefixlen}", "dev", iface])
        self._run_shell_cmd("ip", ip_args)

    def prplmesh_start_mode(self, mode: str = "agent"):
        """Start prplMesh in certification_mode and wait for it to initialize.

        Parameters
        ----------
        mode: str
            (optional) The mode in which to start prplMesh.
            Has to be either 'agent' or 'controller'.
            Defaults to 'agent'.

        Raises
        ------
        ExceptionPexpect
            If the operation failed.

        ValueError
            If the mode is neither 'controller' nor 'agent'.
        """
        if mode not in ["controller", "agent"]:
            raise ValueError("Unknown prplMesh mode: {}".format(mode))

        print("Starting prplmesh as {}".format(mode))
        self._prplMesh_exec("certification_mode {}".format(mode))
        self.expect(self.prompt, timeout=120)
        if self.delay:
            print("Waiting {} seconds for prplMesh to initialize".format(self.delay))
            time.sleep(self.delay)

    def prprlmesh_status_check(self) -> bool:
        """Check prplMesh status by executing status command to initd service.
        Return True if operational.
        """
        return self._prplmesh_status_poll()

    def copy_logs_over_ssh(self, logdir, dirs_to_copy, commands_to_run):
        """Specialized version of `copy_logs`. Copies logs via ssh/scp.
        """
        print(f'Copying logs from {self.control_ip} to {logdir}')

        os.makedirs(logdir, exist_ok=True)

        for src, dst in dirs_to_copy:
            cmd = ['scp', '-r', f'root@{self.control_ip}:{src}', f'{logdir}/{dst}']
            subprocess.run(cmd)

        for command, output_filename in commands_to_run:
            with open(f'{logdir}/{output_filename}', 'w') as outfile:
                subprocess.run(['ssh', f'root@{self.control_ip}', command], stdout=outfile,
                               stderr=outfile)

    def copy_hostapd_configuration_over_ssh(self, logdir):
        def remote_command_output(cmd):
            return subprocess.check_output(['ssh', f'root@{self.control_ip}', cmd]).decode().split()

        # Get list of hostapd files in /var/run/
        output = remote_command_output('echo /var/run/hostapd-*.conf')
        files_to_copy = [(file, '.') for file in output]

        # Get list of network interfaces
        network_interfaces = remote_command_output('iwinfo | grep ESSID | cut -d " " -f 1')

        commands_to_run = []
        for iface in network_interfaces:
            commands_to_run += [
                (f'hostapd_cli -i {iface} status', f'{iface}_status'),
                (f'hostapd_cli -i {iface} get_config', f'{iface}_get_config'),
            ]

        self.copy_logs_over_ssh(logdir, files_to_copy, commands_to_run)

    def copy_logs(self):
        """Copy logs from the device"""

        logdir = f'../logs/device-{self.control_ip}'

        dirs_to_copy = [
            # beerock logs
            (self.beerocks_logs_location, 'beerock_logs'),
        ]

        commands_to_run = [
            # UCI settings
            ('uci show', 'uci.log'),
            # syslog logs
            ('logread', 'syslog.log'),
            # network status
            ('ifconfig', 'ifconfig.log'),
            ('brctl show', 'bridges.log'),
        ]

        self.copy_logs_over_ssh(logdir, dirs_to_copy, commands_to_run)
        self.copy_hostapd_configuration_over_ssh(f'{logdir}/hostapd')

    def prplmesh_remove_logs(self):
        command = ["rm", "-rf", "{}/*".format(self.beerocks_logs_location)]
        self.sendline(" ".join(command))
        self.expect(self.prompt, timeout=10)
