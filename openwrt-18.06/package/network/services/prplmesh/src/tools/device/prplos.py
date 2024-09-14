###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2022 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

# Standard library
import os
import shutil
import subprocess
import sys
import time
from typing import List

# Third party
import pexpect
import pexpect.fdpexpect
import pexpect.pxssh
from device.generic import GenericDevice
from device.serial import SerialDevice


class GenericPrplOS(GenericDevice):
    """A generic PrplOSDevice.

    Since upgrading through uboot is generally target-specific, it
    only offers the sysupgrade option.
    """

    def read_artifacts_dir_version(self) -> List[str]:
        """Reads the local prplwrt-version.

        The version file is read from the artifacts (see `artifacts_dir`).

        Returns
        -------
        List[str]
            The content of the local prplwrt-version as a list of strings.
        """
        with open(os.path.join(self.artifacts_dir, "prplwrt-version")) as version_file:
            return version_file.read().splitlines()

    def read_remote_version(self) -> List[str]:
        """Read prplwrt-version on a remote device

        Returns
        -------
        List[str]
            The content of the prplwrt-version file on the device as a list of strings.
        """
        version = None
        with pexpect.pxssh.pxssh(logfile=sys.stdout.buffer) as shell:
            shell.login(self.name, self.username)
            shell.sendline("cat /etc/prplwrt-version")
            shell.expect("cat /etc/prplwrt-version")
            shell.prompt()
            # remove the first (empty) line:
            version = shell.before.decode().splitlines()[1:]
        return version

    def upgrade_bootloader(self):
        """Upgrade the device through u-boot.

        It requires a running tftp server listenning on the IP the
        target device will expect (see `ipaddr` in the uboot environment).
        """
        serial_path = "/dev/{}".format(self.name)
        if not os.path.exists(serial_path):
            raise ValueError("The serial device {} does not exist!\n".format(serial_path)
                             + "Please make sure you have an appropriate udev rule for it.")
        print("Copying image '{}' to '{}'".format(self.image, self.tftp_dir))
        shutil.copy(os.path.join(self.artifacts_dir, self.image), self.tftp_dir)
        print("Image copied to {}.".format(self.tftp_dir))
        with SerialDevice(self.baudrate, self.name, self.serial_prompt,
                          expect_prompt_on_connect=False) as shell:
            self.reboot(self.check_serial_type(),
                        stop_in_bootloader=True)

            try:
                self.upgrade_from_u_boot(shell)
            except pexpect.exceptions.TIMEOUT:
                print("The upgrade timed out, trying one more time after a reboot.")
                # Interrupt any running command:
                shell.send('\003')
                time.sleep(1)
                self.reboot(self.check_serial_type(),
                            stop_in_bootloader=True)
                try:
                    self.upgrade_from_u_boot(shell)
                except pexpect.exceptions.TIMEOUT as err:
                    print("The upgrade failed again, aborting.")
                    raise err
            print("The upgrade is done, rebooting.")
            self.reboot(self.check_serial_type())
            shell.expect("Please press Enter to activate this console", timeout=180)
            # activate the console:
            shell.sendline("")
            shell.expect(self.serial_prompt)
        # Wait at least for the CAC timer:
        time.sleep(self.initialization_time)

    def sysupgrade(self):
        serial_path = "/dev/{}".format(self.name)
        if not os.path.exists(serial_path):
            raise ValueError("The serial device {} does not exist!\n".format(serial_path)
                             + "Please make sure you have an appropriate udev rule for it.")
        print("Copying image '{}' to the target".format(self.image))
        try:
            subprocess.check_output(["scp",
                                     "{}/{}".format(self.artifacts_dir, self.image),
                                     "{}:/tmp/{}".format(self.name, self.image)])
        except subprocess.CalledProcessError as exc:
            print("Failed to copy the image to the target:\n{}".format(exc.output))
            raise exc
        with SerialDevice(self.baudrate, self.name, self.serial_prompt) as shell:
            # Turn off the wifi to make sure it doesn't prevent the upgrade:
            shell.sendline("wifi down")
            # Do the upgrade
            shell.sendline("sysupgrade -v /tmp/{}".format(self.image))
            # first give it 30 seconds, and fail early if the upgrade didn't start:
            shell.expect(r"Performing system upgrade\.\.\.", timeout=30)
            # now give it more time to apply the upgrade and reboot:
            shell.expect(r"Rebooting system\.\.\.", timeout=300)
            shell.expect("Please press Enter to activate this console", timeout=180)
            # activate the console:
            shell.sendline("")
            shell.expect(self.serial_prompt)
        time.sleep(self.initialization_time)

    def upgrade_from_u_boot(self, shell: pexpect.fdpexpect.fdspawn):
        """Upgrades the OS image from the bootloader once it's ready.
        Since the exact procedure to upgrade devices can vary from
        devices to devices, this method should be overridden in sub
        classes.
        Subclasses can assume the serial shell is ready to receive u-boot commands

        Parameters
        ----------
        shell
            The pexpect-based shell (in u-boot).
        """
        raise NotImplementedError("Not implemented for this device.")
