###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################


class MapClient:
    '''Represents a client (STA) in the connection map.'''

    def __init__(self, mac: str):
        self.mac = mac
        self.path = ""

    def __str__(self):
        return "      STA   {}".format(self.mac)


class MapVap:
    '''Represents a VAP in the connection map.'''

    def __init__(self, bssid: str, ssid: bytes):
        self.bssid = bssid
        self.ssid = ssid
        self.path = ""
        self.clients = {}

    def add_client(self, mac: str):
        client = MapClient(mac)
        self.clients[mac] = client
        return client

    def __str__(self):
        my_id = self.bssid + " " + self.ssid
        return "    BSS   " + '\n'.join([my_id] + [str(client) for client in self.clients.values()])


class MapRadio:
    '''Represents a radio in the connection map.'''

    def __init__(self, uid: str):
        self.uid = uid
        self.path = ""
        self.vaps = {}

    def add_vap(self, bssid: str, ssid: bytes):
        vap = MapVap(bssid, ssid)
        self.vaps[bssid] = vap
        return vap

    def __str__(self):
        return "  Radio " + '\n'.join([self.uid] + [str(vap) for vap in self.vaps.values()])


class MapNeighbor:
    '''Represents a Neighbor in the connection map.'''

    def __init__(self, neighbor_mac: str):
        self.mac = neighbor_mac
        self.path = ""

    def __str__(self):
        return "    Neighbor " + self.mac


class MapInterface:
    '''Represents a interface in the connection map.'''

    def __init__(self, mac: str):
        self.mac = mac
        self.path = ""
        self.neighbors = {}

    def add_neighbor(self, neighbor_mac: str):
        neighbor = MapNeighbor(neighbor_mac)
        self.neighbors[neighbor_mac] = neighbor
        return neighbor

    def __str__(self):
        return "  Interface " + "\n".join(
            [self.mac] + [str(neighbor) for neighbor in self.neighbors.values()]
        )


class MapDevice:
    '''Represents a device in the connection map.'''

    def __init__(self, mac: str):
        self.mac = mac
        self.path = ""
        self.radios = {}
        self.interfaces = {}

    def add_radio(self, uid: str):
        radio = MapRadio(uid)
        self.radios[uid] = radio
        return radio

    def add_interface(self, mac: str):
        interface = MapInterface(mac)
        self.interfaces[mac] = interface
        return interface

    def __str__(self):
        return "Agent " + "\n".join(
            [self.mac]
            + [str(radio) for radio in self.radios.values()]
            + [str(interface) for interface in self.interfaces.values()]
        )
