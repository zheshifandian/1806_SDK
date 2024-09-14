###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2022 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

# Standard library
import re
import subprocess
import time
import traceback

# Third party
import pexpect
import pexpect.fdpexpect
import pexpect.pxssh
from device.prplos import GenericPrplOS


class TurrisPrplOS(GenericPrplOS):
    """A Turris Omnia running prplOS."""

    bootloader_prompt = "=> "
    """The u-boot prompt on the target."""

    rescue_prompt = "~ # "

    def _upgrade_from_u_boot_write_emmc(self, shell: pexpect.fdpexpect.fdspawn):
        """Do the upgrade in the bootloader by loading the image over tftp,
        and write it to the emmc.

        Note that this might not work on some recent Turris Omnias,
        where u-boot is not able to write to the emmc. For those, see
        `_upgrade_from_rescue_shell`.
        """
        uncompressed_image = self.image.split(".gz")[0]
        shell.sendline("setenv bootargs earlyprintk console=ttyS0,115200")
        shell.expect(self.bootloader_prompt)
        time.sleep(1)
        shell.sendline("setenv set_blkcnt 'setexpr blkcnt ${filesize} + 0x7ffff && setexpr blkcnt ${blkcnt} / 0x80000 && setexpr blkcnt ${blkcnt} * 0x400'") # noqa E50
        shell.expect(self.bootloader_prompt)
        shell.sendline("tftpboot ${kernel_addr_r} " + f"{uncompressed_image}")
        shell.expect("Loading: ")
        shell.expect("done")
        shell.expect(self.bootloader_prompt)
        # Write to the emmc:
        shell.sendline("run set_blkcnt && mmc dev 0 0 && mmc erase 0 ${blkcnt} && mmc write ${kernel_addr_r} 0 ${blkcnt}") # noqa E501
        res = shell.expect(["0 blocks written: ERROR", r"\d+ blocks written: OK"], timeout=60)
        if res == 0:
            raise ValueError("Failed to write to the emmc: 0 blocks written!")
        # Still check that a non-zero number of blocks were written:
        try:
            blkcnt_line = re.search(r"(\d+) blocks written: OK", shell.after.decode())
            blkcnt = blkcnt_line.group(1)
            if int(blkcnt) <= 0:
                raise ValueError("Failed to write to the emmc")
        except ValueError as err:
            raise ValueError("Failed to write to the emmc") from err

    def _upgrade_from_rescue_shell(self, shell: pexpect.fdpexpect.fdspawn):
        """Use the device's "rescue shell to load the image over SSH, and
        write it back to NAND flash.
        """
        shell.sendline("")
        shell.expect(self.bootloader_prompt)
        shell.sendline("setenv omnia_reset 6")
        shell.expect(self.bootloader_prompt)

        bootcmd = """setenv bootcmd 'setenv bootargs "earlyprintk console=ttyS0,115200 omniarescue=$omnia_reset rescue_mode=$omnia_reset"; sf probe; sf read 0x1000000 0x100000 0x700000; lzmadec 0x1000000 0x1700000; if gpio input gpio@71_4; then bootm 0x1700000#sfp; else bootm 0x1700000; fi;bootz 0x1000000'""" # noqa E501
        shell.sendline(bootcmd)
        shell.expect(self.bootloader_prompt)
        shell.sendline("run bootcmd")
        shell.expect("Initializing the system", timeout=120)
        shell.expect(self.rescue_prompt)

        # We need to get the IP address of the device for the upgrade.
        # Since we rely on an SSH configuration to be available, get it from there.
        ssh_config = subprocess.check_output(("ssh", "-G", self.name)).decode()
        hostname = re.search(r"\nhostname ([^\n]+)", ssh_config)
        try:
            ipaddr = hostname.group(1)
        except (AttributeError, IndexError) as err:
            raise ValueError("The IP address of the device could not be found.\n" +
                             "Make sure you have the IP of the device in your SSH " +
                             "configuration and try again") from err

        # We cannot know the subnet but we don't care, it will only be
        # used by the device for the upgrade:
        shell.sendline(f"ip addr add {ipaddr}/1 dev eth2")
        shell.expect(self.rescue_prompt)

        shell.sendline("ip link set eth2 up")
        shell.expect(self.rescue_prompt)
        # Start dropbear from the rescue shell:
        shell.sendline("rm -f /etc/dropbear/dropbear_rsa_host_key && dropbear -R")
        shell.expect(self.rescue_prompt)

        # Give the ethernet interface some time to initialize:
        time.sleep(10)

        # Try to copy the uncompressed image to the device:
        uncompressed_image = self.image.split(".gz")[0]
        try:
            subprocess.check_output(["scp", self.tftp_dir + "/" +
                                     uncompressed_image,
                                     f"{self.name}:/tmp/"])
        except subprocess.CalledProcessError as exc:
            print("Failed to copy the image to the target:\n{}".format(exc.output))
            raise exc

        # Finally, write the image to the flash:
        shell.sendline(f'dd if="/tmp/{uncompressed_image}" ' +
                       'of=/dev/mmcblk0 bs=4096 conv=fsync')
        shell.expect(self.rescue_prompt)

    def upgrade_from_u_boot(self, shell: pexpect.fdpexpect.fdspawn):
        """Upgrade from u-boot, discarding the overlay.

        A first attempt is made to load the image in u-boot using
        tftp, then write it back to the emmc.  For some devices (some
        recent Turris Omnias), writing to the emmc from u-boot doesn't
        work.
        To make sure we can always upgrade the device anyway, in this
        case we fall back to using the rescue shell of the device
        instead. The rescue shell is entered just like if the user
        pressed the reset button until 7 leds are on.  From there, the
        prplOS image is copied over SSH and then written to flash
        using 'dd'.

        Parameters
        ----------
        shell: pexpect.fdpexpect.fdspawn
            The serial console to send commands to.
            It's assumed that the console is already stopped in u-boot.
        """

        # Before anything else, we need to decompress the image.
        # gunzip will always have a non-zero exit-code, because there
        # is trailing data at the end of the archive. Instead of
        # checking gunzip's exit code, just check that there is an
        # extracted file.
        subprocess.run(["gunzip", "-f", f"{self.tftp_dir}/{self.image}"], check=False)
        try:
            subprocess.check_output(["test", "-f", f"{self.tftp_dir}/"
                                     + self.image.split(".gz")[0]])
        except subprocess.CalledProcessError as err:
            raise ValueError("An error occured while decompressing the image!") from err

        try:
            self._upgrade_from_u_boot_write_emmc(shell)
        except ValueError:
            print("Unable to flash the device through u-boot:")
            print(traceback.format_exc())
            print("Trying through rescue shell.")
            self._upgrade_from_rescue_shell(shell)
