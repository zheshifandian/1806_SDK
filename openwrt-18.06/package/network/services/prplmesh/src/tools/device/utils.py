###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2022 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

# Standard library
from enum import Enum

# Third party
import pexpect
import pexpect.fdpexpect
import pexpect.pxssh


class ShellType(Enum):
    """ ShellType enum contains 3 types of the possible shell on the device:

        UBOOT, PRPLOS, RDKB
    """
    UBOOT = 1
    PRPLOS = 2
    RDKB = 3
    LINUX_UNKNOWN = 4


def check_uboot_var(shell, variable: str, expectation: str):
    """ Check content of the UBoot variable.

    Parameters
    ----------
    shell : pexpect.fdpexpect.fdspawn
        Shell file descriptor.

    variable : str
        The number of seconds to wait between attempts.

    expectation: str
        Expected content of the variable.

    Raises
    -------
    ValueError
        If the getting content of the variable failed.
    """

    UBOOT_PROMPT = "=>"
    # Standard UBoot prompt.

    shell.sendline(f"printenv {variable}")
    shell.expect([expectation, pexpect.TIMEOUT])
    shell.expect([UBOOT_PROMPT, pexpect.TIMEOUT])
    if shell.match == pexpect.TIMEOUT:
        raise ValueError(f"Failed to get {variable} variable.")


def serial_cmd_err(shell, cmd_prompt: str, command: str):
    """ Execute command via serial port and check error code.

    Parameters
    ----------
    shell : int
        Shell file descriptor.

    cmd_prompt :
        Serial prompt.

    command : str
        Command which should be executed.

    Raises
    -------
    ValueError
        If command prompt not found or failed to execute command.
    """
    shell.sendline("")
    shell.expect([cmd_prompt, pexpect.TIMEOUT])
    if shell.match == pexpect.TIMEOUT:
        raise ValueError("Failed to get serial prompt!")

    shell.sendline(f"{command};echo err_code $?")
    shell.expect(["err_code 0", pexpect.TIMEOUT])
    if shell.match == pexpect.TIMEOUT:
        raise ValueError(f"Failed to execute {command}!")
