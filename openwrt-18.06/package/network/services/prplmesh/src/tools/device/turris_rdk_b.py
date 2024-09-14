###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2022 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

# Standard library
import os
import re
import subprocess
import time
from pathlib import Path

# Third party
import pexpect
import pexpect.fdpexpect
import pexpect.pxssh
from device.generic import GenericDevice
from device.serial import SerialDevice
from device.utils import check_uboot_var, serial_cmd_err, ShellType


class TurrisRdkb(GenericDevice):
    """Represents a RDKB device.

    Offers methods to check if a device needs to be upgraded and to do the actual upgrade.

    It needs to have access to the artifacts of a build job to determine when an upgrade
    is needed (see `artifacts_dir`).
    """

    TURRIS_DTB = "armada-385-turris-omnia.dtb"
    """ Device Tree Blob (Flat Device Tree) for Turris Omnia.
    The DTB is a database that represents the hardware components on a given board. """

    KERNEL_PARTITION = "mmcblk0p3"
    """ eMMC partiotion used for RDKB kernel (zImage) and DTB file."""

    ROOTFS_PARTITION = "mmcblk0p5"
    """ eMMC partition used for RDKB rootfs."""

    serial_prompt = r'root@[^\s]+:[^\s]+# '
    """ Regular expression for root prompt."""

    UBOOT_PROMPT = "=>"
    """ Standard UBoot prompt."""

    kernel = "zImage"
    """ The name of the kernel binary that can be used to upgrade kernel on device."""

    def __init__(self, device: str, name: str, rdkbfs: str, username: str = "root"):
        """

        Parameters
        -----------
        device: str
            The name of the platform (example: turris-omnia).
        name: str
            The name of the device (it should be reachable through ssh without a password).
        rdkbfs: str
            The name of the rdkbfs tarball that can be used to upgrade the device.
        username: str, optional
            The username to use when connecting to the device over SSH.
        """
        self.device = device
        self.name = name
        self.rdkbfs = rdkbfs
        self.username = username

        self.rootdir = os.path.join(os.path.dirname(os.path.realpath(__file__)), "../..")
        self.artifacts_dir = os.path.join(self.rootdir, "build/{}".format(self.device))
        """The directory where artifacts are stored. It's expected to contain the
        image, kernel, dtb files."""

    def is_prplos_ready(self) -> bool:
        """ Checks if prplOS propmpt ready to use after reboot"""

        with SerialDevice(self.baudrate, self.name, self.serial_prompt,
                          expect_prompt_on_connect=False) as shell:
            shell.expect(["Please press Enter to activate this console.", pexpect.TIMEOUT],
                         timeout=180)
            if shell.match == pexpect.TIMEOUT:
                return False

            shell.sendline("")
            # Give the shell a few seconds to be ready:
            time.sleep(30)
            return True

    def check_images_on_board(self):
        """Check images on the Turris Omnia.

            If images was not copied return False, otherwise True.
        """

        with SerialDevice(self.baudrate, self.name, self.serial_prompt) as shell:

            def find_file(file_name: str):
                shell.sendline("")
                shell.expect(self.serial_prompt)
                shell.sendline(f"find /tmp -maxdepth 1 -name {file_name}")
                shell.expect([f"/tmp/{file_name}", pexpect.TIMEOUT])
                if shell.match == pexpect.TIMEOUT:
                    raise ValueError(f"File {file_name} not found.")

            shell.sendline("")
            find_file(self.rdkbfs)
            find_file(self.kernel)
            find_file(self.TURRIS_DTB)

    def load_rdkb_firmware(self):
        """Copy RDKB rootfs and kernel to the Turris Omnia.
        """

        print(f"Copying '{self.rdkbfs}'\n'{self.kernel}'\n'{self.TURRIS_DTB}'\n to the target")
        try:
            subprocess.check_output(["scp", "-o StrictHostKeyChecking=no",
                                     f"{self.artifacts_dir}/{self.rdkbfs}",
                                     f"{self.username}@{self.name}:/tmp/{self.rdkbfs}"])
            subprocess.check_output(["scp", "-o StrictHostKeyChecking=no",
                                     f"{self.artifacts_dir}/{self.kernel}",
                                     f"{self.username}@{self.name}:/tmp/{self.kernel}"])
            subprocess.check_output(["scp", "-o StrictHostKeyChecking=no",
                                     f"{self.artifacts_dir}/{self.TURRIS_DTB}",
                                     f"{self.username}@{self.name}:/tmp/{self.TURRIS_DTB}"])
        except subprocess.CalledProcessError as exc:
            print(f"Failed to copy the image to the target:\n{exc.output}")
            raise exc

    def burn_rdkb_on_board(self):
        """Burn RDKB image on Turris Omnia.
        """
        # We need to make sure we're not running currently running
        # RDK-B! Otherwise, the rest of this method will try to "rm-rf"
        # the whole rootfs on the running system.
        serial_type = self.check_serial_type()
        if serial_type == ShellType.RDKB:
            raise ValueError("The board is currently running RDK-B! Aborting.")
        # If we're currently in u-boot, don't even try the upgrade
        # either, as there is no way it could succeed.
        if serial_type == ShellType.UBOOT:
            raise ValueError("The board is not currently in u-boot, aborting.")

        self.check_images_on_board()

        with SerialDevice(self.baudrate, self.name, self.serial_prompt) as shell:

            def mount_mmc(partition: str):
                serial_cmd_err(shell, self.serial_prompt, f"mount /dev/{partition} /mnt")
                shell.sendline("mount")
                shell.expect(f"/dev/{partition} on /mnt")

            def umount_mmc():
                serial_cmd_err(shell, self.serial_prompt, "umount /mnt")
                shell.sendline("du -sh /mnt")
                shell.expect("0")

            def copy_to_mmc(src: str, dst: str):
                try:
                    serial_cmd_err(shell, self.serial_prompt, f"cp -v {src} {dst}")
                except ValueError:
                    umount_mmc()
                    raise

            def check_partition(partition: str):
                shell.sendline("")
                shell.expect(self.serial_prompt)
                shell.sendline(f"find /dev/ -maxdepth 1 -name {partition}")
                shell.expect([f"/dev/{partition}", pexpect.TIMEOUT])
                if shell.match == pexpect.TIMEOUT:
                    raise ValueError(
                        f"Partition {partition} not found, need to create it.")

            shell.sendline("")
            print("Start to burn RDKB on Turris Omnia")

            check_partition(self.KERNEL_PARTITION)
            check_partition(self.ROOTFS_PARTITION)

            shell.sendline("")

            mount_mmc(self.KERNEL_PARTITION)

            print("Install kernel.")
            copy_to_mmc(f"/tmp/{self.kernel}", "/mnt/zImage")

            shell.sendline("")

            print("Install Device Tree Blob")
            copy_to_mmc(f"/tmp/{self.TURRIS_DTB}", f"/mnt/{self.TURRIS_DTB}")

            shell.sendline("")

            umount_mmc()

            shell.sendline("")

            print("Install RDKB rootfs.")
            mount_mmc(self.ROOTFS_PARTITION)

            shell.sendline("")

            serial_cmd_err(shell, self.serial_prompt, "rm -rf /mnt/*")
            shell.sendline("du -sh /mnt")
            shell.expect("0")

            serial_cmd_err(shell, self.serial_prompt, f"tar -xzf /tmp/{self.rdkbfs} -C /mnt/")

            shell.sendline("")

            umount_mmc()

    def load_rdkb(self):
        """ Launch RDKB on Turris Omnia.

            Raises
            -----------
            ValueError
                If serial does not exist or unable to connect.
        """

        self.reboot(self.check_serial_type())

        common_bridge_ip = "192.168.200.140"
        common_net_mask = "24"
        rdkb_bridge = "lan4.200"

        with SerialDevice(self.baudrate, self.name,
                          self.serial_prompt, expect_prompt_on_connect=False) as shell:
            shell.expect("Hit any key to stop autoboot")
            shell.sendline("")
            shell.expect(self.UBOOT_PROMPT)

            check_uboot_var(shell, "yocto_bootargs", "yocto_bootargs=earlyprintk")
            check_uboot_var(shell, "mmcboot", "mmcboot=run")
            check_uboot_var(shell, "yocto_mmcload", "yocto_mmcload=setenv")

            shell.sendline("run mmcboot")
            shell.expect(["TurrisOmnia-GW login", pexpect.TIMEOUT])
            time.sleep(30)

            # Add vlan. Will be used for SSH connection
            shell.sendline(f"ip link add link lan4 name {rdkb_bridge} type vlan id 200")
            shell.sendline("RES=$?")
            shell.sendline("echo PRPL=$RES")
            shell.expect("PRPL=0")
            shell.sendline(f"ip a a {common_bridge_ip}/{common_net_mask} dev {rdkb_bridge}")
            shell.expect([f"ip a a {common_bridge_ip}/{common_net_mask} dev {rdkb_bridge}"])

    def sysupgrade(self):
        """Upgrade RDKB image on Turris Omnia and launch it.
        """

        # Currently RDK-B can only be upgraded when the device has booted into prplOS:
        serial_type = self.check_serial_type()
        if serial_type != ShellType.PRPLOS:
            print("The device is not running prplOS, rebooting.")
            self.reboot(serial_type)
            if not self.is_prplos_ready():
                raise ValueError("Failed to get ready prplOS serial.")

        self.load_rdkb_firmware()

        self.burn_rdkb_on_board()

        self.load_rdkb()

    def read_rdkb_rootfs_version(self) -> str:
        """ Read new image version.

            Returns
            -----------
                String with image build date otherwise empty string.
        """

        image_name_re = r"rdkb[^\s]+[\d*]+[^\s]\.rootfs\.tar\.gz"
        """ Find correct RDKB rootfs file."""

        image_date_re = r"0*[1-9]\d{4,}"
        """ Retrieve RDKB rootfs build date."""

        image = ""
        for artifact in Path(self.artifacts_dir).iterdir():
            image = re.findall(image_name_re, str(artifact))
            if image:
                break

        date_list = re.findall(image_date_re, str(image))

        date = ""
        for i in date_list:
            date = str(i)

        return date

    def read_remote_rdkb_version(self) -> str:
        """ Read current RDKB rootfs version.

            Returns
            -----------
            str
                RDKB rootfs version.

            Raises
            -----------
            ValueError
                If version of rootfs not found.
        """

        RDKB_VERSION_PATH = "/mnt/etc/version"
        IMAGE_DATE_RE = r"0*[1-9]\d{4,}"
        """ Retrieve RDKB rootfs build date."""

        with pexpect.pxssh.pxssh(options={"StrictHostKeyChecking": "no",
                                          "UserKnownHostsFile": "/dev/null"}) as shell:
            shell.login(self.name, self.username)

            shell.sendline(f"mount /dev/{self.ROOTFS_PARTITION} /mnt")
            shell.prompt()
            shell.sendline("mount")
            shell.expect(f"/dev/{self.ROOTFS_PARTITION} on /mnt")

            shell.sendline(f"cat {RDKB_VERSION_PATH}")
            shell.expect([f"cat {RDKB_VERSION_PATH}", pexpect.TIMEOUT])
            shell.prompt()
            version = re.findall(IMAGE_DATE_RE, shell.before.decode())
            if not version:
                # We need to add dummy version for handling case when we flash new board or board
                # where RDKB rootfs was brocken and version file does not exist.
                version = "1"

            shell.sendline("umount /mnt")
            shell.expect("umount /mnt")
            shell.prompt()

            for i in version:
                version = str(i)

            return version

    def needs_upgrade(self) -> bool:
        """ Check if we need to upgrade the board or not.

            Returns
            -----------
            bool
                True if upgrade required otherwise False.

            Raises
            -----------
            ValueError
                If failed to get serial connection or prplOS is not ready for use
        """

        serial_type = self.check_serial_type()
        if serial_type == ShellType.UBOOT:
            self.reboot(serial_type)
            if not self.is_prplos_ready():
                raise ValueError("Failed to get ready prplOS serial.")

        current_version = int(self.read_remote_rdkb_version())
        new_version = int(self.read_rdkb_rootfs_version())

        print(f"Current RDKB version is: {current_version} \nNew RDKB version is: {new_version}")

        will_upgrade = new_version != current_version
        if will_upgrade and serial_type == ShellType.RDKB:
            self.reboot(self.check_serial_type())
            if not self.is_prplos_ready():
                raise ValueError("Failed to get ready prplOS serial.")

        return will_upgrade
