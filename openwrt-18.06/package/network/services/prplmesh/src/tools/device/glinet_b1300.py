###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2022 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

# Standard library
import time

# Third party
import pexpect
import pexpect.fdpexpect
import pexpect.pxssh
from device.prplos import GenericPrplOS


class GlinetB1300(GenericPrplOS):
    """A Turris Omnia running prplOS.

    A tftp server must be running to serve images, and both 'serverip'
    and 'ipaddr' should already be set in the bootloader.
    """

    bootloader_prompt = r"\(IPQ40xx\) # "
    """The u-boot prompt on the target."""

    boot_stop_expression = 'Hit "gl" key to stop booting:'
    """The expression signaling the device can be stopped in its bootloader."""

    boot_stop_sequence = "gl"
    """The sequence to use to stop the device in its bootloader."""

    def upgrade_from_u_boot(self, shell: pexpect.fdpexpect.fdspawn):
        """Upgrade from u-boot and remove the overlay.

        Parameters
        ----------
        shell: pexpect.fdpexpect.fdspawn
            The serial console to send commands to.
            It's assumed that the console is already stopped in u-boot.
        """
        shell.sendline("")
        shell.expect(self.bootloader_prompt)
        # Give the ethernet interfaces some time to initialize:
        time.sleep(5)
        shell.sendline(f"tftpboot 0x84000000 {self.image}")
        shell.sendline("")
        shell.expect("Loading: ")
        shell.expect("done")
        shell.expect(self.bootloader_prompt)
        shell.sendline("sf probe && sf erase 0x180000 0x1e80000 && sf write 0x84000000 0x180000 $filesize") # noqa E501
        # Writing the image takes time, make sure the timeout is big enough:
        shell.expect(self.bootloader_prompt, timeout=300)
