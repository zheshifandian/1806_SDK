#!/usr/bin/env python3
###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2022 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

"""This script can be used to configure a device (prplOS, RDK-B, etc)
over a serial conntection."""

import argparse
import sys

from pathlib import Path

from device.configuration import configure_device
from device.get_device import device_from_name


def main():
    parser = argparse.ArgumentParser(prog=sys.argv[0],
                                     description="""Configure a device over serial.""")
    parser.add_argument('-d', '--device',
                        help="""Device to configure. For a list of the supported devices, see the
 get_device module.""", required=True)
    parser.add_argument(
        '-t',
        '--target-name',
        help="Name of the target to upgrade.", required=True)

    parser.add_argument(
        '-c',
        '--configuration',
        help="The path to the configuration file.", required=True)

    args = parser.parse_args()

    dev = device_from_name(args.device, args.target_name, image="")
    configure_device(dev, Path(args.configuration))


if __name__ == '__main__':
    main()
