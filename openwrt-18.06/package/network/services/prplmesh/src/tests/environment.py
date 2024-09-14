###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

# Standard imports:
import json
import os
import platform
import re
import shlex
import subprocess
import time
from collections import namedtuple
from enum import Enum
from subprocess import PIPE, Popen
from typing import Dict, Any, List


# Third-party imports:
import pexpect
import yaml

# Local imports:
import sniffer
from capi import UCCSocket
from connmap import MapDevice
from opts import opts, debug, err

MemoryStat = namedtuple('MemoryStat', 'total_memory free_memory buffers cached used_memory')
CpuStat = namedtuple('CpuStat', 'cpu_usage cpu_avg')


class ALEntity:
    '''Abstract representation of a MultiAP device (1905.1 AL Entity).

    Derived classes provide concrete implementations for a specific device (e.g. docker).

    This provides basic information about the entity, e.g. its AL MAC address. How this information
    is retrieved is implementation-specific.

    It also provides an abstract interface to interact with the entity, e.g. for sending CAPI
    commands.

    If a device runs both the agent and the controller, two ALEntities should be created for it,
    with the same MAC address. This is not how it is modeled in 1905.1, but it corresponds to how
    it is implemented in prplMesh and it allows us to have e.g. a separate UCCSocket to controller
    and agent.
    '''

    def __init__(self, mac: str, ucc_socket: UCCSocket, installdir: str,
                 is_controller: bool = False):
        self.mac = mac
        self.ucc_socket = ucc_socket
        self.installdir = installdir
        self.is_controller = is_controller
        self.radios = []
        self.logfilenames = []  # List[str]
        self.checkpoints = {}  # Dict[str, int]

        # Convenience functions that propagate to ucc_socket
        self.cmd_reply = self.ucc_socket.cmd_reply
        self.dev_get_parameter = self.ucc_socket.dev_get_parameter
        self.dev_send_1905 = self.ucc_socket.dev_send_1905
        self.start_wps_registration = self.ucc_socket.start_wps_registration

    @staticmethod
    def get_checkpoints(checkpoints: Dict[str, int], start_line: int):
        """If a start_line was provided, use it for all
        checkpoints. Otherwise, keep the checkpoints as-is"""
        if not start_line:
            return checkpoints
        return {checkpoint: start_line for checkpoint in checkpoints}

    def command(self, *command: str) -> str:
        '''Run `command` on the device and return its output as bytes.

        Example: command('ip', 'addr') to get IP addresses of all interfaces.
        '''
        raise NotImplementedError("command is not implemented in abstract class ALEntity")

    def prplmesh_command(self, command: str, *args: str) -> str:
        '''Run `command` with "args" on the device and return its output as bytes.

        "command" is relative to the installation directory of prplmesh, e.g. "bin/beerocks_cli".
        '''
        return self.command(os.path.join(self.installdir, command), *args)

    def wait_for_log(self, regex: str, start_line: int, timeout: float,
                     fail_on_mismatch: bool = True) -> bool:
        '''Poll the entity's logfile until it contains "regex" or times out.'''
        raise NotImplementedError("wait_for_log is not implemented in abstract class ALEntity")

    def checkpoint(self):
        '''Checkpoint the log files for both the entity and its radios.

        Any subsequent calls to check_logs will only return log lines after now.
        '''
        for logfilename in self.logfilenames:
            self.checkpoints[logfilename] = int(self.command("wc", "-l", f"{logfilename}")
                                                .split(" ")[0])
        for radio in self.radios:
            radio.checkpoint()

    # Northbound API access functions

    def nbapi_command(self, path: str, command: str, args: Dict = None) -> Dict:
        '''Run a northbound API command.

        Run northbound API "command" on the object specified with "path" with arguments "args".
        '''
        raise NotImplementedError("nbapi_command is not implemented in abstract class ALEntity")

    def nbapi_command_not_fail(self, path: str, command: str, args: Dict = None) -> Dict:
        '''Run a northbound API command.

        Run northbound API "command" on the object specified with "path" with arguments "args".
        '''
        raise NotImplementedError(
            "nbapi_command_not_fail is not implemented in abstract class ALEntity")

    def nbapi_get(self, path: str, args: Dict = None) -> Dict:
        '''Run a northbound API 'get' command.

        Run northbound API "get" on the object specified with "path" with arguments "args". Parse
        the return value and remove the outer dict (with is always a single-entry dict with 'path.'
        as the key).
        '''
        ret = self.nbapi_command(path, "_get", args)
        if not ret:
            return {}
        assert len(ret) == 1, "NBAPI 'get' should return a single object"
        return ret[path + "."]

    def nbapi_list(self, path: str, args: Dict = None) -> Dict:
        '''Run a northbound API 'list' command.

        Run northbound API "list" on the object specified with "path" with arguments "args".
        '''

        return self.nbapi_command(path, '_list', args)

    def nbapi_get_parameter(self, path: str, parameter: str) -> Any:
        '''Get a parameter from nbapi.

        Gets the northbound API "parameter" in the object specified with "path". Returns the value,
        converted to a Python object.

        Equivalent to nbapi_get_object(path)[parameter] but slightly more efficient.
        '''
        values = self.nbapi_get(path, {"parameters": [parameter]})
        return values and values[parameter]

    def nbapi_set_parameters(self, path: str, parameters: Dict) -> Any:
        '''Set a parameter for nbapi object.

        Sets value for "parameters" of northbound API object specified with "path".
        '''
        ret = self.nbapi_command(path, "_set", {"parameters": parameters})
        return ret

    def nbapi_get_data_model(self):
        '''Get entire data model tree.'''

        data_model = self.nbapi_command("Device.WiFi.DataElements.Network", "_get", {"depth": "10"})
        return data_model

    def nbapi_get_list_instances(self, path: str) -> List[str]:
        '''Get all instances of a template object from nbapi.

        Gets the northbound API objects instantiated from the template object "path". Returns a
        list of strings - path to specific object.
        '''
        instances = self.nbapi_list(path)['instances']
        return [f"{path}.{instance['index']}" for instance in instances]

    def get_memory_usage(self):
        cmd_output = self.command(
            'sh',
            '-c',
            "awk '/MemTotal/ || /MemFree/ || /Buffers/ || /^Cached/ {print $2}' /proc/meminfo")
        tot_m, free_m, buff, cached = map(int, cmd_output.split())

        return MemoryStat(tot_m, free_m, buff, cached, tot_m - free_m - buff - cached)

    def get_cpu_usage(self):
        '''Get percentage sum of %CPU column in top command

        Runs one iteration of the top command and locates the index of
        the %CPU column. Then proceeds to sum all the values on the index
        '''
        cmd_output = self.command('top', 'b', '-n', '1')
        cpu_column = False
        cpu_usage = 0.0
        for line in cmd_output.split('\n'):
            if not cpu_column and re.findall(r'%CPU', line):
                cpu_column = line.split().index('%CPU')
                continue
            if cpu_column and line:
                cpu_usage += float(line.split()[cpu_column].replace('%', ''))
        cmd_output = self.command('cat', '/proc/loadavg')
        cpu_avg = float(cmd_output.split()[0])
        return CpuStat(cpu_usage/100, cpu_avg)


ChannelInfo = namedtuple("ChannelInfo", "channel bandwidth center_channel")


class Radio:
    '''Abstract representation of a radio on a MultiAP agent.

    This provides basic information about the radio, e.g. its mac address, and functionality for
    checking its status.
    '''

    def __init__(self, agent: ALEntity, mac: str):
        self.agent = agent
        agent.radios.append(self)
        self.mac = mac
        self.vaps = []
        self.logfilenames = []  # List[str]
        self.checkpoints = {}  # Dict[str, int]

    def checkpoint(self):
        for logfilename in self.logfilenames:
            self.checkpoints[logfilename] = \
                int(self.agent.command("wc", "-l", f"{logfilename}")
                    .split(" ")[0])

    def wait_for_log(self, regex: str, start_line: int, timeout: float,
                     fail_on_mismatch: bool = True) -> bool:
        '''Poll the radio's logfile until it contains "regex" or times out.'''
        raise NotImplementedError("wait_for_log is not implemented in abstract class Radio")

    def get_current_channel(self) -> ChannelInfo:
        '''Get the current channel information.

        Returns
        -------
        ChannelInfo
            3-tuple with the channel information: channel index, bandwidth as an integer, and
            center channel for VHT.
        '''
        raise NotImplementedError("get_current_channel is not implemented in abstract class Radio")

    def get_power_limit(self) -> int:
        '''Get the current tx_power information.'''
        raise NotImplementedError("get_power_limit is not implemented in abstract class Radio")

    def disable(self) -> int:
        '''Disable the radio.'''
        raise NotImplementedError("Not implemented in abstract class.")

    def enable(self) -> int:
        '''Enable the radio and wait for it to be ready.'''
        raise NotImplementedError("Not implemented in abstract class.")

    def update_vap_list(self):
        ''' Initialize / update VAP list '''
        pass

    def get_vap(self, ssid: str):
        for vap in self.vaps:
            if vap.get_ssid() == ssid:
                return vap
        return None


class Station:
    '''Placeholder for a wireless (fronthaul) station.

    Unlike the other classes, this is not an abstract class. Instead, it is a placeholder that
    represents a station. Handling the station is actually done through the VirtualAP concrete
    implementation.
    '''

    def __init__(self, mac: str):
        self.mac = mac

    __mac_base = 0

    @staticmethod
    def create():
        '''Generate a Station placeholder with a random MAC address.'''
        mac = '51:a1:10:20:{:02x}:{:02x}'.format(int(Station.__mac_base / 256),
                                                 Station.__mac_base % 256)
        Station.__mac_base += 1
        if Station.__mac_base > 256*256:
            Station.__mac_base = 0
        return Station(mac)


class StationEvent(Enum):
    '''An enum representing the possible station events
    based on client association event TLV assoc_event_flags'''
    CONNECT = 0x00000080
    DISCONNECT = 0x00000000


class ChannelTlvs(Enum):
    CHANNEL_6 = (
        "0x14 "
        "{0x51 {0x0C {0x01 0x02 0x03 0x04 0x05 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D} 0x00}} "
        "{0x52 {0x00 0x00}} "
        "{0x53 {0x08 {0x01 0x02 0x03 0x04 0x05 0x07 0x08 0x09} 0x00}} "
        "{0x54 {0x08 {0x05 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D} 0x00}} "
        "{0x73 {0x00 0x00}} "
        "{0x74 {0x00 0x00}} "
        "{0x75 {0x00 0x00}} "
        "{0x76 {0x00 0x00}} "
        "{0x77 {0x00 0x00}} "
        "{0x78 {0x00 0x00}} "
        "{0x79 {0x00 0x00}} "
        "{0x7A {0x00 0x00}} "
        "{0x7B {0x00 0x00}} "
        "{0x7C {0x00 0x00}} "
        "{0x7D {0x00 0x00}} "
        "{0x7E {0x00 0x00}} "
        "{0x7F {0x00 0x00}} "
        "{0x80 {0x00 0x00}} "
        "{0x81 {0x00 0x00}} "
        "{0x82 {0x00 0x00}} "
    )

    CHANNEL_36 = (
        "0x14 "
        "{0x51 {0x00 0x00}} "
        "{0x52 {0x00 0x00}} "
        "{0x53 {0x00 0x00}} "
        "{0x54 {0x00 0x00}} "
        "{0x73 0x03 {0x28 0x2C 0x30} 0x00} "
        "{0x74 0x01 {0x2C} 0x00} "
        "{0x75 {0x00 0x00}} "
        "{0x76 {0x00 0x00}} "
        "{0x77 {0x00 0x00}} "
        "{0x78 {0x00 0x00}} "
        "{0x79 {0x00 0x00}} "
        "{0x7A {0x00 0x00}} "
        "{0x7B {0x00 0x00}} "
        "{0x7C {0x00 0x00}} "
        "{0x7D {0x00 0x00}} "
        "{0x7E {0x00 0x00}} "
        "{0x7F {0x00 0x00}} "
        "{0x80 0x05 {0x3A 0x6A 0x7A 0x8A 0x9B} 0x00} "
        "{0x81 {0x00 0x00}} "
        "{0x82 {0x00 0x00}}"
    )


class VirtualAP:
    '''Abstract representation of a VAP on a MultiAP Radio.'''

    def __init__(self, radio: Radio, bssid: str):
        self.radio = radio
        radio.vaps.append(self)
        self.bssid = bssid

    def associate(self, sta: Station) -> bool:
        '''Associate "sta" with this VAP.'''
        raise NotImplementedError("associate is not implemented in abstract class VirtualAP")

    def disassociate(self, sta: Station) -> bool:
        '''Disassociate "sta" from this VAP.'''
        raise NotImplementedError("disassociate is not implemented in abstract class VirtualAP")


# The following variables are initialized as None, and have to be set when a concrete test
# environment is started.
wired_sniffer = None
controller = None
agents = []


def beerocks_cli_command(command: str) -> str:
    '''Execute `command` beerocks_cli command on the controller and return its output.'''
    debug("Send CLI command " + command)
    res = controller.prplmesh_command("bin/beerocks_cli", "-c", command)
    debug("  Response: " + res.strip())
    return res


# Helper function used by the implementations based on ubus
def nbapi_ubus_command(entity: ALEntity, path: str, command: str, args: Dict = None) -> Dict:
    command = ['ubus', 'call', path, command]
    if args:
        command.append(json.dumps(args))
    result = entity.command(*command)
    if result:
        return json.loads(result)
    else:
        return result


# The same as nbapi_ubus_command, except it does not fail when error occurs
def nbapi_ubus_command_not_fail(entity: ALEntity, path: str, command: str,
                                args: Dict = None) -> Dict:
    command = ['ubus', 'call', path, command]
    result = b''
    if args:
        command.append(json.dumps(args))
    try:
        result = entity.command(*command)
    except subprocess.CalledProcessError as error:
        debug("ubus call command fail")
        debug(error)
    if result:
        return json.loads(result)
    else:
        return result


# Concrete implementation with docker

rootdir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
installdir = os.path.join(rootdir, 'build', 'install')
on_wsl = "microsoft" in platform.uname()[3].lower()


# Temporary workaround
# Since we have multiple log files that correspond to a radio, multiple programs are passed
# as argument. In the log messages, we only use the first one.
# This should be reverted again as part of Unified Agent.
def _docker_wait_for_log(container: str, checkpoints: Dict[str, int], regex: str, timeout: float,
                         fail_on_mismatch: bool = True) -> bool:
    for logfilename in checkpoints.keys():
        print(' --- logfilename: {}'.format(logfilename))
    deadline = time.monotonic() + timeout
    try:
        while True:
            for logfilename in checkpoints.keys():
                start_line = checkpoints[logfilename]
                with open(logfilename, 'rb') as logfile:
                    for (i, v) in enumerate(logfile.readlines(), 1):
                        if i <= start_line:
                            continue
                        search = re.search(regex.encode('utf-8'), v)
                        if search:
                            debug("Found '{}'\n\tin {} line {}".format(regex, logfilename, i))
                            return (True, i, search.groups())
            if time.monotonic() < deadline:
                time.sleep(.3)
            else:
                if fail_on_mismatch:
                    err("Can't find '{}'\n\tin log of {} on {} after {}s".format(regex,
                                                                                 str(checkpoints),
                                                                                 container,
                                                                                 timeout))
                else:
                    debug("Can't find '{}'\n\tin log of {} on {},"
                          "but failure allowed".format(regex, str(checkpoints), container))

                return (False, 0, None)
    except OSError:
        err("Can't read log of one of {} on {}".format(str(checkpoints), container))
        return (False, 0, None)


def _device_clear_input_buffer(device, timeout=0.5):
    '''
    Clear input buffer

    Parameters
    ----------
    device: PrplMeshPrplWRT
        An agent or controller device class.

    timeout: float
        Number of seconds to wait for a new input to arrive.

        If timeout is zero it discards input buffer and exits.

        If timeout is greater than 0 it disards input until there is a `timeout`-long period
            of time without new input.
    '''

    attempts = 20

    try:
        # Limit number of attempts in case of a periodic input arriving more often
        # than `timeout` seconds
        while attempts > 0:
            device.read_nonblocking(size=128*1024, timeout=timeout)
            attempts -= 1
    except pexpect.TIMEOUT:
        pass


def _device_reset_console(device):
    ''' Reset console input.

    Interrupt any running command and wait for an input prompt.
    '''

    _device_clear_input_buffer(device)

    # Interrupt any running command
    device.send('\003')

    # Expect the prompt and the end of the line, to make sure we match
    # the last one. Doing this will make sure we don't keep old data
    # in the buffer.
    device.expect(device.prompt)

    _device_clear_input_buffer(device)


# Temporary workaround
# Since we have multiple log files that correspond to a radio, multiple log files are passed
# as argument. In the log messages, we only use the first one.
# This should be reverted again as part of Unified Agent.
def _device_wait_for_log(device: None, checkpoints: Dict[str, int], regex: str,
                         timeout: float, fail_on_mismatch: bool = True):
    """Waits for log matching regex expression to show up."""

    debug("--- Looking for {}".format(regex))
    debug("    in {}".format(" ".join(checkpoints.keys())))
    debug("    starting at lines {}".format(checkpoints))
    debug("    timeout: {} second(s)".format(timeout))

    # Current approach reads remote files in a loop.
    # It may cause some delays that should be invisible to the user,
    # so increase timeout.
    timeout += 2

    deadline = time.monotonic() + timeout

    while time.monotonic() < deadline:
        for logfilename in checkpoints.keys():
            start_line = checkpoints[logfilename]
            command = ['tail', '-n', f'+{start_line}', logfilename]
            output = subprocess.check_output(['ssh', device.control_ip] + command)

            for (i, v) in enumerate(output.split(b"\n")):
                search = re.search(regex.encode('utf-8'), v)
                if search:
                    debug("--- Found at line {}:".format(i))
                    debug(v)
                    return (True, i + start_line, search.groups())

        time.sleep(.3)

    if fail_on_mismatch:
        err("--- Cannot find {} in {}".format(regex, " ".join(str(check) for check in checkpoints)))
    else:
        debug("--- Not found")

    return (False, 0, None)


class ALEntityDocker(ALEntity):
    '''Docker implementation of ALEntity.

    The entity is defined from the name of the container, the rest is derived from that.
    '''
    # NOTE: name arg can be also extracted from the device class itself, but test_flows.py
    # don't have it. We can remove this arg as soon, as we drop test_flows.py

    def __init__(self, name: str, device: None = None, is_controller: bool = False,
                 compose: bool = False):

        self.name = name
        ucc_interface_name = 'eth0'
        if device:
            self.device = device

        # First, get the UCC port from the config file
        if is_controller:
            config_file_name = 'beerocks_controller.conf'
        else:
            config_file_name = 'beerocks_agent.conf'
        with open(os.path.join(installdir, 'config', config_file_name)) as config_file:
            ucc_port = \
                re.search(r'ucc_listener_port=(?P<port>[0-9]+)',
                          config_file.read()).group('port')

        # On WSL, connect to the locally exposed container port
        if on_wsl or compose:
            published_port_output = subprocess.check_output(
                ["docker", "port", name, ucc_port]).decode('utf-8').split(":")
            device_ip = published_port_output[0]
            ucc_port = int(published_port_output[1])
        else:
            device_ip_output = self.command(
                'ip', '-f', 'inet', 'addr', 'show', ucc_interface_name)
            device_ip = re.search(
                r'inet (?P<ip>[0-9.]+)', device_ip_output).group('ip')

        ucc_socket = UCCSocket(device_ip, ucc_port)
        mac = ucc_socket.dev_get_parameter('ALid')

        super().__init__(mac, ucc_socket, installdir, is_controller)
        program = "controller" if is_controller else "backhaul"
        self.logfilenames = [self.logfilename(program)]

        # We always have two radios, wlan0 and wlan2
        RadioDocker(self, "wlan0")
        RadioDocker(self, "wlan2")

        self.refresh_vaps()

    def logfilename(self, program):
        logfilename = os.path.join(rootdir, 'logs', self.name, 'beerocks_{}.log'.format(program))

        # WSL doesn't support symlinks on NTFS, so resolve the symlink manually
        if on_wsl:
            logfilename = os.path.join(
                rootdir, 'logs', self.name,
                subprocess.check_output(["tail", "-2", logfilename]).decode('utf-8').
                rstrip(' \t\r\n\0'))
        return logfilename

    def iperf_throughput(self, to_dut: bool, duration: int = 5, protocol: str = 'tcp',
                         bitrate: int = 0, omit: int = 2, num_streams: int = 5,
                         print_output: bool = False) -> float:
        server_hostname = self.get_iface_ip()
        self.command('iperf3', '--daemon', '-s', '-B', server_hostname, '-J', '-1')
        return _iperf_throughput(server_hostname, to_dut, duration,
                                 protocol, omit, bitrate,
                                 num_streams, print_output)

    def command(self, *command: str) -> str:
        '''Execute `command` in docker container and return its output.'''

        command_str = " ".join(command)
        debug(f"--- Executing command: {command_str}")
        return subprocess.check_output(("docker", "exec", self.name) + command).decode()

    def wait_for_log(self, regex: str, start_line: int, timeout: float,
                     fail_on_mismatch: bool = True) -> bool:
        '''Poll the entity's logfile until it contains "regex" or times out.'''
        checkpoints = ALEntity.get_checkpoints(self.checkpoints, start_line)
        return _docker_wait_for_log(self.name, checkpoints, regex, timeout,
                                    fail_on_mismatch=fail_on_mismatch)

    def nbapi_command(self, path: str, command: str, args: Dict = None) -> Dict:
        return nbapi_ubus_command(self, path, command, args)

    def nbapi_command_not_fail(self, path: str, command: str, args: Dict = None) -> Dict:
        return nbapi_ubus_command_not_fail(self, path, command, args)

    def prprlmesh_status_check(self):
        return self.device.prprlmesh_status_check()

    def beerocks_cli_command(self, command) -> str:
        '''Execute `command` beerocks_cli command on the controller and return its output.
        Will return None if called from an object that is not a controller.
        '''
        if self.is_controller:
            debug("Send CLI command " + command)
            res = self.prplmesh_command("bin/beerocks_cli", "-c", command)
            debug("  Response: " + res.strip())
            return res
        return None

    def get_conn_map(self) -> Dict[str, MapDevice]:
        '''Get the connection map from the controller.'''

        '''Regular expression to match a MAC address in a bytes string.'''
        RE_MAC = r"(?P<mac>([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2})"

        conn_map = {}
        for line in self.beerocks_cli_command("bml_conn_map").split('\n'):
            # TODO we need to parse indentation to get the exact topology.
            # For the time being, just parse the repeaters.
            bridge = re.search(r' {8}IRE_BRIDGE: .* mac: ' + RE_MAC, line)
            radio = re.match(r' {16}RADIO: .* mac: ' + RE_MAC, line)
            vap = re.match(r' {20}fVAP.* bssid: ' + RE_MAC + r', ssid: (?P<ssid>.*)$', line)
            client = re.match(r' {24}CLIENT: mac: ' + RE_MAC, line)
            if bridge:
                cur_agent = MapDevice(bridge.group('mac'))
                conn_map[cur_agent.mac] = cur_agent
            elif radio:
                cur_radio = cur_agent.add_radio(radio.group('mac'))
            elif vap:
                cur_vap = cur_radio.add_vap(vap.group('mac'), vap.group('ssid'))
            elif client:
                cur_vap.add_client(client.group('mac'))
        return conn_map

    def refresh_vaps(self):
        for radio in self.radios:
            radio.vaps = []
            vap_file = yaml.safe_load(radio.read_tmp_file("vap"))
            for vap in vap_file:
                VirtualAPDocker(radio, vap['bssid'])

    def get_iface_ip(self):
        '''Returns the IP of the data interface'''
        prplmesh_net = _docker_inspect_network_json(os.getenv("RUN_ID"))

        container_info = [v for v in prplmesh_net['Containers'].values()
                          if v['Name'] == self.name][0]
        return container_info['IPv4Address'].split('/')[0]


class RadioDocker(Radio):
    '''Docker implementation of a radio.'''

    def __init__(self, agent: ALEntityDocker, iface_name: str):
        self.iface_name = iface_name
        ip_output = agent.command("ip", "-o",  "link", "list", "dev", self.iface_name)
        mac = re.search(r"link/ether (([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2})",
                        ip_output).group(1)
        super().__init__(agent, mac)
        self.logfilenames = [
            self.agent.logfilename(f"agent"),
            self.agent.logfilename(f"ap_manager_{iface_name}"),
        ]

    def wait_for_log(self, regex: str, start_line: int, timeout: float,
                     fail_on_mismatch: bool = True) -> bool:
        '''Poll the radio's logfile until it contains "regex" or times out.'''
        checkpoints = ALEntity.get_checkpoints(self.checkpoints, start_line)
        return _docker_wait_for_log(self.agent.name, checkpoints, regex, timeout,
                                    fail_on_mismatch=fail_on_mismatch)

    def send_bwl_event(self, event: str) -> None:
        # The file is only available within the docker container so we need to use an echo command.
        # Use '|| true' to make sure it doesn't fail on timeout.
        command = "echo \"{}\" | timeout 1s tee /tmp/beerocks/{}/EVENT* >/dev/null || true"\
            .format(event, self.iface_name)
        self.agent.command('sh', '-c', command)

    def read_tmp_file(self, filename: str) -> bytes:
        '''Read the file given by `filename` from the radio's status directory'''
        # The file is only available within the docker container so we need to use cat.
        path = "/tmp/beerocks/{}/{}".format(self.iface_name, filename)
        return self.agent.command("cat", path)

    def get_current_channel(self) -> ChannelInfo:
        channel_info = yaml.safe_load(self.read_tmp_file("channel"))
        return ChannelInfo(channel_info["channel"], channel_info["bw"],
                           channel_info["center_channel"])

    def get_power_limit(self) -> int:
        power_info = yaml.safe_load(self.read_tmp_file("tx_power"))
        return power_info["tx_power"]

    def disable(self):
        self.send_bwl_event("EVENT AP-DISABLED {}".format(self.iface_name))

    def enable(self):
        self.send_bwl_event("EVENT AP-ENABLED {}".format(self.iface_name))


class BssType(Enum):
    Disabled = (0, 0)
    Fronthaul = (1, 0)
    Backhaul = (0, 1)
    Hybrid = (1, 1)


class VirtualAPDocker(VirtualAP):
    '''Docker implementation of a VAP.'''

    def __init__(self, radio: RadioDocker, bssid: str):
        super().__init__(radio, bssid)

    def get_ssid(self) -> str:
        '''Get current SSID of attached radio. Return string.'''
        vaps_info = yaml.safe_load(self.radio.read_tmp_file("vap"))
        vap_info = [vap for vap in vaps_info if vap['bssid'] == self.bssid]
        if vap_info:
            return 'N/A' if not vap_info[0]['ssid'] else vap_info[0]['ssid']
        return None

    def associate(self, sta: Station) -> bool:
        '''Associate "sta" with this VAP.'''
        self.radio.send_bwl_event("EVENT AP-STA-CONNECTED {}".format(sta.mac))

    def disassociate(self, sta: Station) -> bool:
        '''Disassociate "sta" from this VAP.'''
        self.radio.send_bwl_event("EVENT AP-STA-DISCONNECTED {}".format(sta.mac))

    def get_bss_type(self) -> int:
        '''
        0 = disabled (default)
        1 = AP supports backhaul BSS
        2 = AP supports fronthaul BSS
        3 = AP supports both backhaul BSS and fronthaul BSS
        '''
        vaps_info = yaml.safe_load(self.radio.read_tmp_file("vap"))
        vap_info = [(vap['fronthaul'], vap['backhaul'])
                    for vap in vaps_info if vap['bssid'] == self.bssid]
        if vap_info:
            return self.bss_from_bits(*vap_info[0])
        return None

    @staticmethod
    def bss_from_bits(fronthaul: bool, backhaul: bool):
        return {
            (False, False): BssType.Disabled,
            (False, True): BssType.Backhaul,
            (True, False): BssType.Fronthaul,
            (True, True): BssType.Hybrid
        }.get((fronthaul, backhaul), BssType.Disabled)


def _docker_inspect_network_json(unique_id: str):
    """Use docker network inspect to get the docker bridge interface.

            Parameters
            ----------
            unique_id: str
                index ID of the current run

            Raises
            ------
            CalledProcessError
                If exit code was non-zero

            Returns
            ------
            inspect: dict
                dict containing the inspected docker network
    """
    docker_network = 'prplMesh-net-{}'.format(unique_id)
    docker_network_inspect_cmd = ('docker', 'network', 'inspect', docker_network)
    inspect_result = subprocess.run(docker_network_inspect_cmd, stdout=subprocess.PIPE)
    if inspect_result.returncode != 0:
        # Assume network doesn't exist yet. Create it.
        # This is normally done by test_gw_repeater.sh, but we need it earlier to be able to
        # start tcpdump
        # Raise an exception if it fails (check=True).
        subprocess.run(('docker', 'network', 'create', '--label', 'prplmesh',
                        '--label', 'prplmesh-id={}'.format(unique_id), docker_network), check=True,
                       stdout=subprocess.DEVNULL)

        # Inspect again, now raise if it fails (check=True).
        inspect_result = subprocess.run(docker_network_inspect_cmd, check=True,
                                        stdout=subprocess.PIPE)

    inspect = json.loads(inspect_result.stdout)
    return inspect[0]


def _iperf_throughput(server_hostname: str, to_dut: bool, duration: int = 5,
                      protocol: str = 'tcp', omit: int = 2, bitrate: int = 0,
                      num_streams: int = 5, print_output: bool = False) -> float:
    '''Connects boardfarm as client to previously started server
     on the server_hostname address
        Parameters
        ----------
        server_hostname: str
            iperf server IP

        to_dut: bool
            True - Download
            False - Upload

        duration: int = 5
            Time in seconds

        protocol: str tcp
            Protocol used

        omit: int = 2
            Seconds to be removed from a test result

        bitrate: int = 0
            bitrate

        num_streams: int = 5
            Parallel streams

        Returns
        ------
            2 decimals rounded value of the throughput average
    '''

    to_dut_flag = {True: '', False: '-R'}
    cmd = (f"iperf3 -c {server_hostname} -t {str(duration)} -b {str(bitrate)}"
           f"-P {num_streams}{to_dut_flag.get(to_dut)} -O {omit} -J --get-server-output")

    debug('Running {} iperf {}'.format(protocol,
                                       {True: "download",
                                        False: "upload"}.get(to_dut)))
    debug('Connecting to {}'.format(server_hostname))

    output = Popen(cmd, stdout=PIPE, stderr=PIPE, shell=True)
    output = output.communicate()[0]
    throughput_average = extract_avg(output.decode())

    return throughput_average


def extract_avg(iperf_output):
    try:
        _json = json.loads(re.sub('(-nan|nan)', '"nan"', iperf_output))
    except TypeError:
        raise TypeError("The output of the Iperf server was empty.")

    try:
        intervals = _json['server_output_json']['intervals']
        throughput_intervals = [x['sum']['bits_per_second']
                                for x in intervals if x['sum']['omitted'] is False]
    except KeyError as exc:
        raise KeyError('The iperf json output does not have the expected keys.') from exc

    try:
        # Find the sample average and convert bits to Mbits
        throughput_average = sum(throughput_intervals) / len(throughput_intervals) / 10**6
    except ZeroDivisionError:
        print('Throughput average: 0 Mbit/s')
        return 0

    # Round two decimal place
    throughput_average = round(throughput_average, 2)

    return throughput_average


def _get_bridge_interface(unique_id: str):
    prplmesh_net = _docker_inspect_network_json(unique_id)

    # podman adds a 'plugins' indirection that docker doesn't have.
    if 'plugins' in prplmesh_net:
        bridge = prplmesh_net['plugins'][0]['bridge']
    else:
        # docker doesn't report the interface name of the bridge. So format it based on the ID.
        bridge_id = prplmesh_net['Id']
        bridge = 'br-' + bridge_id[:12]

    return bridge


def launch_environment_docker(unique_id: str, skip_init: bool = False, tag: str = ""):
    global wired_sniffer
    iface = _get_bridge_interface(unique_id)
    wired_sniffer = sniffer.Sniffer(iface, opts.tcpdump_dir)

    gateway = 'gateway-' + unique_id
    repeater1 = 'repeater1-' + unique_id
    repeater2 = 'repeater2-' + unique_id

    if not skip_init:
        command = [os.path.join(rootdir, "tests", "test_gw_repeater.sh"), "-f", "-u", unique_id,
                   "-g", gateway, "-r", repeater1, "-r", repeater2, "-d", "7"]
        if tag:
            command += ["-t", tag]
        wired_sniffer.start('init')
        try:
            subprocess.check_call(command)
        finally:
            wired_sniffer.stop()

    global controller, agents
    controller = ALEntityDocker(name=gateway, is_controller=True)
    agents = (ALEntityDocker(name=repeater1), ALEntityDocker(name=repeater2))

    debug('controller: {}'.format(controller.mac))
    debug('agent1: {}'.format(agents[0].mac))
    debug('agent1 wlan0: {}'.format(agents[0].radios[0].mac))
    debug('agent1 wlan2: {}'.format(agents[0].radios[1].mac))
    debug('agent2: {}'.format(agents[1].mac))
    debug('agent2 wlan0: {}'.format(agents[1].radios[0].mac))
    debug('agent2 wlan2: {}'.format(agents[1].radios[1].mac))


class ALEntityPrplWrt(ALEntity):
    """Abstraction of ALEntity in real device."""

    def __init__(self, device: None, is_controller: bool = False):
        self.device = device
        self.name = device.name

        if is_controller:
            self.config_file_name = '/opt/prplmesh/config/beerocks_controller.conf'
        else:
            self.config_file_name = '/opt/prplmesh/config/beerocks_agent.conf'

        ucc_port_raw = self.command("grep", "ucc_listener_port", self.config_file_name)
        ucc_port = int(re.search(r'ucc_listener_port=(?P<port>[0-9]+)',
                                 ucc_port_raw).group('port'))
        log_folder_raw = self.command(
            "grep", "log_files_path", self.config_file_name)
        self.log_folder = re.search(r'log_files_path=(?P<log_path>[a-zA-Z0-9_\/]+)',
                                    log_folder_raw).group('log_path')
        ucc_socket = UCCSocket(str(self.device.control_ip), int(ucc_port), timeout=60)
        mac = ucc_socket.dev_get_parameter('ALid')

        super().__init__(mac, ucc_socket, installdir, is_controller)

        program = "controller" if is_controller else "backhaul"

        self.logfilenames = ["{}/beerocks_{}.log".format(self.log_folder, program)]

        radios = nbapi_ubus_command(self, 'network.wireless', 'status')
        for radio_name, radio in radios.items():
            for intf in radio['interfaces']:
                if intf['config']['mode'] == 'ap':
                    assert 'ifname' in intf, f'ifname not found in {radio_name}'
                    status_output = self.command('hostapd_cli', '-i', intf['ifname'], 'status')
                    bss = [line for line in status_output.splitlines()
                           if line.startswith('bss[0]=')]
                    assert bss, f'BSS not found in {radio_name}'
                    assert len(bss) == 1, f'More than one main BSS found in {radio_name}'
                    main_intf = bss[0].split('=')[1]
                    RadioHostapd(self, main_intf)
                    break
        assert len(self.radios), f'No radios found on {self.name}'

    def command(self, *command: str) -> str:
        """Execute `command` in device and return its output."""

        command_str = shlex.join(command)
        return subprocess.check_output(["ssh", self.device.control_ip, command_str]).decode()

    def wait_for_log(self, regex: str, start_line: int, timeout: float,
                     fail_on_mismatch: bool = True) -> bool:
        """Poll the entity's logfile until it contains "regex" or times out."""
        checkpoints = ALEntity.get_checkpoints(self.checkpoints, start_line)
        return _device_wait_for_log(self.device, checkpoints, regex, timeout, fail_on_mismatch)

    def nbapi_command(self, path: str, command: str, args: Dict = None) -> Dict:
        return nbapi_ubus_command(self, path, command, args)

    def nbapi_command_not_fail(self, path: str, command: str, args: Dict = None) -> Dict:
        return nbapi_ubus_command_not_fail(self, path, command, args)

    def prprlmesh_status_check(self):
        return self.device.prprlmesh_status_check()


class ALEntityRDKB(ALEntity):
    """Abstraction of ALEntity in real device."""

    def __init__(self, device: None, is_controller: bool = False):
        self.device = device
        self.name = device.name

        if is_controller:
            self.config_file_name = '/opt/prplmesh/config/beerocks_controller.conf'
        else:
            self.config_file_name = '/opt/prplmesh/config/beerocks_agent.conf'

        ucc_port_raw = self.command("grep", "ucc_listener_port", self.config_file_name)
        ucc_port = int(re.search(r'ucc_listener_port=(?P<port>[0-9]+)',
                                 ucc_port_raw).group('port'))
        log_folder_raw = self.command(
            "grep", "log_files_path", self.config_file_name)
        self.log_folder = re.search(r'log_files_path=(?P<log_path>[a-zA-Z0-9_\/]+)',
                                    log_folder_raw).group('log_path')
        ucc_socket = UCCSocket(str(self.device.control_ip), int(ucc_port))
        mac = ucc_socket.dev_get_parameter('ALid')

        super().__init__(mac, ucc_socket, installdir, is_controller)

        program = "controller" if is_controller else "agent"
        self.logfilenames = ["{}/beerocks_{}.log".format(self.log_folder, program)]

        # We always have two radios, wifi0 and wifi1
        RadioHostapd(self, "wifi0")
        RadioHostapd(self, "wifi1")

    def command(self, *command: str) -> str:
        """Execute `command` in device and return its output."""

        command_str = shlex.join(command)
        return subprocess.check_output(["ssh", self.device.control_ip, command_str]).decode()

    def wait_for_log(self, regex: str, start_line: int, timeout: float,
                     fail_on_mismatch: bool = True) -> bool:
        """Poll the entity's logfile until it contains "regex" or times out."""
        checkpoints = ALEntity.get_checkpoints(self.checkpoints, start_line)
        return _device_wait_for_log(self.device, checkpoints, regex, timeout, fail_on_mismatch)

    def nbapi_command(self, path: str, command: str, args: Dict = None) -> Dict:
        return nbapi_ubus_command(self, path, command, args)

    def nbapi_command_not_fail(self, path: str, command: str, args: Dict = None) -> Dict:
        return nbapi_ubus_command_not_fail(self, path, command, args)

    def prprlmesh_status_check(self):
        return self.device.prprlmesh_status_check()


class RadioHostapd(Radio):
    """Abstraction of real Radio in prplWRT device."""

    def __init__(self, agent: ALEntityPrplWrt, iface_name: str):
        self.iface_name = iface_name
        self.agent = agent

        ip_raw = self.agent.command("/sbin/ip", "link", "list", "dev", self.iface_name)

        mac = re.search(r"link/ether (([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2})",
                        ip_raw).group(1)
        self.log_folder = agent.log_folder
        super().__init__(agent, mac)

        self.logfilenames = [
            "{}/beerocks_agent.log".format(self.log_folder),
            "{}/beerocks_backhaul.log".format(self.log_folder),
            "{}/beerocks_ap_manager_{}.log".format(self.log_folder, self.iface_name)
        ]

        self.update_vap_list()

    def update_vap_list(self):
        iface_name = self.iface_name

        self.vaps = []

        output = self.agent.command('sh', '-c', f'/sbin/ip a | grep "{iface_name}"')
        output = re.findall(r"wlan[0-9]+[^\s]+", output)
        result = ""
        for i in output:
            output = str(i)[:-1]
            result += f"{output}\n"

        output = result
        vap_candidates = output.split()

        debug("vap candidates : " + " * ".join(vap_candidates))

        for vap_iface in vap_candidates:
            iwinfo_output = self.agent.command('/usr/sbin/iw', 'dev', vap_iface, 'info')

            if re.search('dummy_ssid', iwinfo_output):
                # On MaxLinear devices (e.g. Axepoint) wlan0 and wlan2 are dummy interfaces.
                # They are not VAPs.
                # These interfaces have SSIDs "dummy_ssid_0" and "dummy_ssid_2".
                debug(f"Skip {vap_iface} since it has dummy SSID")
                continue

            if not re.search('type AP', iwinfo_output):
                # Skip backhaul/station interfaces.
                debug(f"Skip {vap_iface} since it is a station interface")
                continue

            debug(f"Add {vap_iface} to the list of VAPs")

            vap_mac = self.get_mac(vap_iface)
            VirtualAPHostapd(self, vap_mac)

        if len(self.vaps) == 1:
            # On Axepoint-based devices wlan2 has no SSID until it obtains real VAPs
            # wlan2 is not a VAP itself, remove it.
            #
            # TODO: PPM-1312: find a better way to detect this.
            self.vaps = []

        vaps = [vap.iface for vap in self.vaps]
        debug("Radio {} has following VAPs: {}".format(iface_name, " ".join(vaps)))

    def wait_for_log(self, regex: str, start_line: int, timeout: float,
                     fail_on_mismatch: bool = True):
        ''' Poll the Radio's logfile until it match regular expression '''
        checkpoints = ALEntity.get_checkpoints(self.checkpoints, start_line)
        return _device_wait_for_log(self.agent.device, checkpoints, regex, timeout,
                                    fail_on_mismatch)

    def get_mac(self, iface: str) -> str:
        """Return mac of specified iface"""
        regex = "link/ether (?P<mac>([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2})"

        output = self.agent.command("ip", "link", "show", f"{iface}")
        match = re.search(regex, output)
        return match.group('mac')

    def get_current_channel(self) -> ChannelInfo:
        regex = r"channel (?P<channel>[0-9]+) [^\r\n]*width[^\r\n]* (?P<width>[0-9]+) " + \
            r"MHz[^\r\n]*center1[^\r\n]* (?P<center>[0-9]+) MHz"

        output = self.agent.command("iw", "dev", f"{self.iface_name}", "info")
        match = re.search(regex, output)
        return ChannelInfo(int(match.group('channel')), int(match.group('width')),
                           int(match.group('center')))

    def get_power_limit(self) -> int:
        regex = r"txpower (?P<power_limit>[0-9]*)(\.0+)? dBm"

        output = self.agent.command("iw", "dev", f"{self.iface_name}", "info")
        match = re.search(regex, output)
        return int(match.group('power_limit'))

    def disable(self):
        self.agent.command("hostapd_cli", "-i", self.iface_name, "disable")

    def enable(self):
        self.agent.command("hostapd_cli", "-i", self.iface_name, "enable")


class VirtualAPHostapd(VirtualAP):
    """Abstraction of a VAP in prplWRT device."""

    def __init__(self, radio: RadioHostapd, bssid: str):
        super().__init__(radio, bssid)
        self.iface = self.get_iface(self.bssid)

    def get_ssid(self) -> str:
        """Get current SSID of attached radio. Return string."""
        regex = "\nssid=(?P<ssid>.*)\n"
        output = self.radio.agent.command(
            "hostapd_cli", "-i", self.iface, "get_config")
        # We are looking for SSID definition
        # ssid=Multi-AP-24G-1
        match = re.search(regex, output)
        return 'N/A' if match is None else match.group('ssid')

    def get_psk(self) -> str:
        """Get SSIDs personal key set during last autoconfiguration. Return string"""
        ssid = self.get_ssid()
        command = (f'grep "Autoconfiguration for ssid: {ssid}"'
                   ' "{self.radio.log_folder}/beerocks_agent.log"'
                   ' | tail -n 1')

        # We looking for key, which was set during last autoconfiguration. E.g of such string:
        # network_key: maprocks2 fronthaul:
        regex = "network_key: (?P<psk>.*) fronthaul"

        output = self.radio.agent.command('sh', '-c', command)
        match = re.search(regex, output)
        return match.group('psk')

    def get_iface(self, bssid: str) -> str:
        output = self.radio.agent.command(
            'sh',
            '-c',
            f'/sbin/ip link list | grep -B1 "{bssid}"')
        assert output, f'No interface found with the bssid {bssid}'
        return output.split(':')[1].strip()

    def associate(self, sta: Station) -> bool:
        ''' Associate "sta" with this VAP '''
        # TODO: complete this stub
        return True

    def disassociate(self, sta: Station) -> bool:
        ''' Disassociate "sta" from this VAP.'''
        # TODO: complete this stub
        return True

    def get_bss_type(self) -> int:
        regex = r"mesh_mode\=\w+ \((?P<bss_type>\d?)\)"

        output = self.radio.agent.command(
            "hostapd_cli", "-i", f"{self.iface}", "get_mesh_mode", f"{self.iface}")
        match = re.search(regex, output)

        return self.bss_from_bits_intel(match.group('bss_type'))

    @staticmethod
    def bss_from_bits(bss_type: str):
        return {
            '0': BssType.Disabled,
            '1': BssType.Backhaul,
            '2': BssType.Fronthaul,
            '3': BssType.Hybrid
        }.get(bss_type, BssType.Disabled)

    @staticmethod
    def bss_from_bits_intel(bss_type: str):
        return {
            '0': BssType.Fronthaul,
            '1': BssType.Backhaul,
            '2': BssType.Hybrid
        }.get(bss_type, BssType.Disabled)
