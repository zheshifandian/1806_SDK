###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2022 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################
"""This module facilitates configuring devices over serial."""

# Standard library
import hashlib
import time
from pathlib import Path

# Third party
import pexpect
from device.generic import GenericDevice
from device.serial import SerialDevice


def configure_device(device: GenericDevice, configuration_file: Path):
    """Configure a device over a serial line.

    It is assumed that the device is accessible over serial at
    /dev/device_name.
    Note that a udev rule can be used to create a symlink pointing to
    the right serial device.

    To configure the device, the configuration file is written to the
    device over the serial line. As serial lines can be unreliable, an
    md5 hash of the file is computed to try to make sure the
    configuration that we apply corresponds to what we expect. Note
    that this means that the md5sum utility should be present on the
    device.

    Parameters
    ----------
    device
        The device to configure.
    configuration_file
        The shell file to use to configure the device (relative to the
        repository's top-level directory).
    """
    configuration_file = device.rootdir / configuration_file
    if not configuration_file.is_file():
        raise ValueError(f"Missing configuration file {configuration_file}!")

    print(f"Applying configuration {str(configuration_file)} on device {device.name}.")
    conf_file_location = "/tmp/config_file.sh"
    md5 = hashlib.md5()
    with SerialDevice(device.baudrate, device.name, device.serial_prompt,
                      expect_prompt_on_connect=False) as shell:
        # Workaround for PCF-600: kill leftover network restarts, and
        # remove NetDev neighbors.
        shell.sendline("")
        time.sleep(1)
        shell.send("\003")
        shell.expect(device.serial_prompt)
        shell.sendline("pgrep -f 'ubus -t 30 wait_for network.interface' | xargs kill -s KILL")
        shell.sendline("/etc/init.d/netdev-plugin  restart")
        shell.expect(device.serial_prompt)
        shell.sendline("")
        shell.expect(device.serial_prompt)
        shell.sendline("sysctl -w kernel.printk='0 4 1 7'")
        shell.expect(device.serial_prompt)
        shell.sendline(f"rm -rf {conf_file_location} && touch {conf_file_location}")
        shell.expect(device.serial_prompt)
        shell.sendline(f"cat << 'END_OF_TRANSMISSION' > {conf_file_location}")
        shell.expect("> ")
        with open(str(configuration_file)) as config_file:
            # Write the configuration file line by line on the device:
            for line in config_file.readlines():
                shell.send(line)
                shell.expect("> ")
                md5.update(line.encode())
        shell.sendline("END_OF_TRANSMISSION")
        shell.expect(device.serial_prompt)
        shell.sendline(f"md5sum {conf_file_location}")
        shell.expect(f"md5sum {conf_file_location}")
        shell.expect(device.serial_prompt)
        md5_device = shell.before.decode().splitlines()[1]
        if md5_device != f"{md5.hexdigest()}  {conf_file_location}":
            raise ValueError("The md5 of the configuration file doesn't match! Aborting.")

        print(md5.hexdigest())
        shell.sendline(f"sh {conf_file_location}")
        shell.expect(device.serial_prompt)
        shell.sendline("echo conf_file_exit_code=$?")
        try:
            shell.expect("conf_file_exit_code=0", timeout=1)
        except pexpect.TIMEOUT as err:
            raise ValueError("Failed to apply the configuration on the device!") from err
