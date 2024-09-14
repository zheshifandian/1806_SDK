###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2022 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

# Standard library
import difflib
import os
import re
import time
from typing import List

# Third party
import pexpect
import pexpect.fdpexpect
import pexpect.pxssh
from device.serial import SerialDevice
from device.utils import ShellType


class GenericDevice():
    """Base class that all the devices class should extend.

    This class relies on access to the artifacts of a build job to
    determine when an upgrade is needed (see `artifacts_dir`).

    It's also expected that:

    - The device is reachable over SSH without a
      password.
    - A serial line for the device is accessible at
      '/dev/name'. Note that a udev rule can be used to create symlinks
      for serial devices.
    """

    serial_prompt = r'root@[^\s]+:[^\s]+# '
    """The OS prompt for serial connections."""

    baudrate = 115200
    """The baudrate of the serial connection to the device."""

    initialization_time = 60
    """The time (in seconds) the device needs to initialize when it boots
    for the first time after flashing a new image."""

    bootloader_prompt = "=> "
    """The prompt of the bootloader."""

    boot_stop_expression = "Hit any key to stop autoboot"
    """The expression signaling the device can be stopped in its bootloader."""

    boot_stop_sequence = "\n"
    """The sequence to use to stop the device in its bootloader."""

    tftp_dir = "/srv/tftp"
    """The root directory of the tftp server. OS images will be copied there."""

    def __init__(self, device: str, name: str, image: str, username: str = "root"):
        """

        Parameters
        -----------
        device: str
            The name of the platform (example: nec-wx3000hp).
        name: str
            The name of the device (it should ne reachable through ssh without a password).
        image: str
            The name of the image that can be used to upgrade the device.
        username: str, optional
            The username to use when connecting to the device over SSH.
        """
        self.device = device
        self.name = name
        self.image = image
        self.username = username

        self.rootdir = os.path.join(os.path.dirname(os.path.realpath(__file__)), "../..")
        self.artifacts_dir = os.path.join(self.rootdir, "build/{}".format(self.device))
        """The directory where artifacts are stored. It's expected to contain
        a version file, and the image file."""

    def reach(self, attempts: int = 5, wait: int = 5) -> bool:
        """Check if the device is reachable via SSH (and optionally try multiple times).

        Parameters
        ----------
        attempts: int, optional
            How many times we try before concluding that the device is unreachable.
        wait: int, optional
            The number of seconds to wait between attempts.

        Returns
        -------
        bool
            True if the device is reachable via SSH, false otherwise.
        """
        for _ in range(attempts):
            try:
                with pexpect.pxssh.pxssh(options={"StrictHostKeyChecking": "no",
                                                  "UserKnownHostsFile": "/dev/null"}) as shell:
                    shell.login(self.name, self.username, login_timeout=5)
                    return True
            except (pexpect.pxssh.ExceptionPxssh, pexpect.exceptions.EOF):
                print("Waiting for the device to be reachable")
                time.sleep(wait)
        return False

    def needs_upgrade(self):
        """Check if a device needs to be updated

        The check is done by comparing prplwrt-version on the target
        with a local one (see `read_artifacts_dir_version).
        """
        artifacts_dir_version = self.read_artifacts_dir_version()
        remote_version = self.read_remote_version()
        diff = difflib.unified_diff(artifacts_dir_version, remote_version,
                                    fromfile='artifacts', tofile='device')
        diff_str = '\n'.join(diff)
        if diff_str:
            print(diff_str)
        return bool(diff_str)

    def reboot(self, serial_type: ShellType, stop_in_bootloader: bool = False):
        """Reboot the device.

        Note that this method handles both the cases where the device
        is currently booted into a Linux OS, or stopped in its
        bootloader.

        Parameters
        -----------
        serial_type: ShellType
            Type of the serial connection as enum ShellType(uboot, rdkb, prplOS)
        stop_in_bootloader: bool
            Whether to stop the device when it enters its bootloader or not.
        """
        with SerialDevice(self.baudrate, self.name,
                          self.serial_prompt, expect_prompt_on_connect=False) as shell:
            print("Reset board.")

            if serial_type == ShellType.UBOOT:
                shell.sendline("reset")
            elif serial_type in [ShellType.PRPLOS, ShellType.RDKB, ShellType.LINUX_UNKNOWN]:
                shell.sendline("reboot ; sleep 15 && echo force rebooting && reboot -f")
            if stop_in_bootloader:
                print("Device will be stopped in its bootloader.")
                shell.expect(self.boot_stop_expression, timeout=180)
                shell.sendline(self.boot_stop_sequence)
                shell.expect(self.bootloader_prompt)
                print("Device stopped in bootloader.")

    def check_serial_type(self) -> ShellType:
        """ Checks type of the serial terminal.

        Returns
        -------
        ShellType
            Enum for rdkb, prplOS or uboot shell.

        Raises
        -------
        ValueError
            If connecting to the serial device failed or the serial
            type could not be determined.
        """

        OSTYPE_RE = r"NAME=[^\s]*"

        with SerialDevice(self.baudrate, self.name, self.serial_prompt,
                          expect_prompt_on_connect=False) as shell:
            shell.send('\003')
            shell.expect([self.bootloader_prompt, pexpect.TIMEOUT], timeout=1)
            if shell.match is not pexpect.TIMEOUT:
                return ShellType.UBOOT

            # Silence kernel messages:
            shell.sendline("sysctl -w kernel.printk='0 4 1 7'")
            shell.expect([self.serial_prompt, pexpect.TIMEOUT], timeout=1)
            shell.sendline("")
            shell.sendline("cat /etc/os-release")

            os_name = ""

            shell.expect([OSTYPE_RE, pexpect.TIMEOUT], timeout=2)
            os_name = str(re.search(OSTYPE_RE, shell.buffer.decode("utf-8")))
            if os_name:
                if re.findall(r"OpenWrt", os_name):
                    return ShellType.PRPLOS
                if re.findall(r"RDK", os_name):
                    return ShellType.RDKB
            shell.sendline("")
            shell.sendline("dmesg --help")
            try:
                shell.expect("Usage: dmesg")
                return ShellType.LINUX_UNKNOWN
            except pexpect.TIMEOUT as err:
                raise ValueError("Unknown serial type!") from err

    def read_artifacts_dir_version(self) -> List[str]:
        """Reads the local version file.

        The version file is read from the artifacts (see `artifacts_dir`).

        Returns
        -------
        List[str]
            The content of the local version file as a list of strings.
        """
        raise NotImplementedError("Reading the local version is not implemented for this device.")

    def read_remote_version(self) -> List[str]:
        """Read the version file on a remote device.

        Returns
        -------
        List[str]
            The content of the version file on the device as a list of strings.
        """
        raise NotImplementedError("Reading the remote version is not implemented for this device.")

    def upgrade_bootloader(self):
        """Upgrade the device through uboot."""
        raise NotImplementedError("u-boot upgrades is not implemented for this device.")
