# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

import pexpect
from typing import Dict

from boardfarm.devices import linux
from environment import ALEntity


class CommandError(Exception):
    """Raised on failed execution"""
    pass


class PrplMeshBase(linux.LinuxDevice):
    """PrplMesh abstract device."""

    def _run_shell_cmd(self, cmd: str = "", args: list = None, timeout: int = 30,
                       env: Dict[str, str] = None):
        """Wrapper that executes command with specified args on host machine and logs output."""
        args = args or []

        res, exitstatus = pexpect.run(cmd, args=args, timeout=timeout, encoding="utf-8",
                                      withexitstatus=1, env=env)

        entry = " ".join((cmd, " ".join(args)))
        if exitstatus != 0:
            raise CommandError("Error executing {}:\n{}".format(entry, res))

        self.log_calls += entry
        self.log += "$ " + entry + "\r\n" + res

    def check_status(self):
        """Method required by boardfarm.

        It is used by boardfarm to indicate that spawned device instance is ready for test
        and also after test - to insure that device still operational.

        Currently always returns True.
        """
        # TODO: enable actual check

        # entity = self.get_active_entity()

        # entity.ucc_socket.cmd_reply("device_get_info")

        return True

    def close(self):
        """Method required by boardfarm.

        Purpose is to close connection to device's consoles.
        """
        self.copy_logs()

    def isalive(self):
        """Method required by boardfarm.

        States that device is operational and its consoles are accessible.
        """
        pass

    def touch(self):
        """Method required by boardfarm.

        Purpose is to keep consoles active, so they don't disconnect for long running activities.
        """
        pass

    def get_active_entity(self) -> ALEntity:
        """Returns the active ALEntityDocker instance based on the
        class role so the entity call can be abstracted"""

        if self.role == "controller":
            return self.controller_entity
        return self.agent_entity

    def copy_logs(self):
        """Copy logs from the device"""
        pass
