# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

import os

import boardfarm
from environment import ALEntityDocker, _get_bridge_interface
from .prplmesh_base import PrplMeshBase
from sniffer import Sniffer

rootdir = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../..'))


class PrplMeshCompose(PrplMeshBase):
    """Dockerized prplMesh device."""

    model = ("prplmesh_compose")
    agent_entity = None
    controller_entity = None

    def __init__(self, *args, **kwargs):
        self.args = args
        self.kwargs = kwargs

        config = kwargs.get("config", kwargs)

        # List of device's consoles test can interact with
        self.consoles = [self]

        # Getting unic ID to distinguish devices and network they belong to
        self.unique_id = os.getenv("RUN_ID")

        self.name = config.get("name", "prplmesh_compose")
        self.docker_name = "-".join((self.name, self.unique_id))
        print('config.get("name") {}'.format(config.get("name")))
        self.role = config.get("role", "agent")
        self.cleanup_cmd = config.get("cleanup_cmd", None)
        self.conn_cmd = config.get("conn_cmd", None)
        self.delay = config.get("delay", 7)
        self.docker_network = "prplMesh-net-{}".format(self.unique_id)

        if self.role == "controller":
            self.controller_entity = \
                ALEntityDocker(self.docker_name, device=self, is_controller=True, compose=True)
        else:
            self.agent_entity = ALEntityDocker(self.docker_name, device=self,
                                               is_controller=False, compose=True)

        self.wired_sniffer = Sniffer(_get_bridge_interface(self.unique_id),
                                     boardfarm.config.output_dir)

        self.check_status()

    def isalive(self):
        """Method required by boardfarm.

        States that device is operational and its consoles are accessible.

        """
        return True

    def prprlmesh_status_check(self):
        self.check_status()
        return True
