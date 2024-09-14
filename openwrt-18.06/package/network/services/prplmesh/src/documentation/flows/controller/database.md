<!--
SPDX-License-Identifier: BSD-2-Clause-Patent
Copyright (c) 2021 the prplMesh contributors
This code is subject to the terms of the BSD+Patent license.
See LICENSE file for more details.
-->

# Introduction

The controller database keeps track of all the information available in the controller.

# Topology

![UML](../../images/plantuml/controller.database.png)

The network topology is represented by a collection of Devices and Stations.

Each Device corresponds to a Multi-AP agent in the network (including the controller itself, which must be running an agent as well).
Each Device has a number of Radios.
Each Radio has a number of configured BSSes, as reported in the AP Operational BSS TLV.
Radio UID is unique within the device, but not necessarily globally unique.
Station MAC, BSSID and AL-MAC are globally unique, however it is possible that one of the BSSIDs is identical to the Device's AL-MAC.

Stations are added to the database when they are discovered.
This can be because the station associates to a BSS, or because it's the backhaul station of a Radio.

A device also has Interfaces.
An Interface could be a station interface, or a BSS, or even a radio, depending on how the agent reports it.
There is no explicit relation, however. Only implicit through the MAC address.
