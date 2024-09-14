###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020-2022 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################
"""Utility module to get the correct device."""

# Third party
import device.axepoint
import device.generic
import device.glinet_b1300
import device.prplos
import device.turris_prplos
import device.turris_rdk_b


def device_from_name(name: str, target_name: str, image: str
                     ) -> device.generic.GenericDevice:
    """Construct a device based on its name and type.

    Parameters
    ----------
    name: str
        The name of the device
    target_name: str
        The name of the target.
    image: str
        The name of the image.
    """
    if name in ["axepoint", "nec-wx3000hp"]:
        dev = device.axepoint.Axepoint(name, target_name, image)
    elif name == "turris-omnia-rdk":
        dev = device.turris_rdk_b.TurrisRdkb(name, target_name, image)
    elif name == "turris-omnia":
        dev = device.turris_prplos.TurrisPrplOS(name, target_name, image)
    elif name == "glinet-b1300":
        dev = device.glinet_b1300.GlinetB1300(name, target_name, image)
    else:
        # if no device matched, try the generic prplOS (sysupgrade)
        print("No specific device matched, using GenericPrplOS.")
        dev = device.prplos.GenericPrplOS(name, target_name, image)
    return dev
