###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2022 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

# Standard library

# Third party
import pexpect
import pexpect.fdpexpect
import pexpect.pxssh
from device.prplos import GenericPrplOS


class Axepoint(GenericPrplOS):
    """An axepoint or any similar device (e.g. NEC WX3000HP).

    At the moment, the device can only be updated through uboot using a tftp server.
    """

    bootloader_prompt = "GRX500 #"
    """The u-boot prompt on the target."""

    def upgrade_from_u_boot(self, shell: pexpect.fdpexpect.fdspawn):
        """Upgrade from u-boot and remove the overlay.

        Parameters
        ----------
        shell: pexpect.fdpexpect.fdspawn
            The serial console to send commands to.
            It's assumed that the console is already stopped in u-boot.
        """
        # set the image name and save it:
        shell.sendline("setenv fullimage {} ; saveenv".format(self.image))
        shell.expect(self.bootloader_prompt)
        # do the actual upgrade:
        shell.sendline("run update_fullimage")
        shell.expect("Creating dynamic volume .* of size", timeout=120)
        shell.expect(r"(?i)Writing to nand\.\.\. (ok|done)", timeout=60)
        shell.expect(self.bootloader_prompt, timeout=600)
        shell.sendline("true")

    def sysupgrade(self):
        # sysupgrade is not supported at the moment, so explicitely override it.
        raise NotImplementedError("sysupgrade is not implemented for this device.")
