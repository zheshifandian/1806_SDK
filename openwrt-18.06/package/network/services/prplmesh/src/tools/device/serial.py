###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2022 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################
"""This module contains utilities for shells over a serial line."""
# Standard library
import os
import sys
import time

# Third party
import pexpect
import pexpect.fdpexpect
import pexpect.pxssh
import serial


class SerialDevice(pexpect.fdpexpect.fdspawn):
    """A base class representing a serial device using pexpect.

    It assumes the device will be accessible at /dev/<name> (use a
    udev rule to make sure it's the case).
    """

    send_delay = 10
    """The delay (in milliseconds) to wait before sending another character.
    This is used for cases where the serial console cannot handle a regular pexpect
    "sendline" because the characters are sent too fast.
    """

    def __init__(self, baudrate: int, name: str, prompt, expect_prompt_on_connect=True):
        self.baudrate = baudrate
        self.name = name
        self.prompt_expr = prompt
        self.expect_prompt_on_connect = expect_prompt_on_connect

        self.serial_path = "/dev/{}".format(self.name)
        self.serial = None
        self.shell = None

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, *exc_details):
        self.disconnect()

    def connect(self):
        """Create the serial connection and the pexpect shell.

        Raises
        ------
        ValueError
            If the shell is not alive after its creation.
        """
        if not os.path.exists(self.serial_path):
            raise ValueError(f"The serial device {self.serial_path} does not exist!\n"
                             + "Please make sure you have an appropriate udev rule for it.")
        if self.serial is not None:
            raise ValueError("Serial already connected!")
        self.serial = serial.Serial(self.serial_path, self.baudrate, xonxoff=True)
        self.serial.flushInput()
        super().__init__(self.serial, logfile=sys.stdout.buffer)

        # Unlike pxssh, fdspawn doesn't have a spawn method. For ease
        # of use, create one:
        def prompt():
            self.expect(self.prompt_expr)
        self.prompt = prompt

        if not self.isalive():
            raise ValueError("Unable to connect to the serial device!")
        self.sendline("")
        if self.expect_prompt_on_connect:
            self.prompt()

    def disconnect(self):
        """Close the serial connection."""
        if self.serial.is_open:
            self.serial.close()

    def sendline(self, s: str):
        """Imitates pexpect sendline(), but waits for a delay between each
        characters to give the device some time to process them.
        """
        if not self.serial:
            raise ValueError("There is no serial! Use connect() to create it first.")
        for char in s:
            self.send(char)
            time.sleep(self.send_delay / 1000.)
        super().sendline("")
        time.sleep(self.send_delay / 1000.)
