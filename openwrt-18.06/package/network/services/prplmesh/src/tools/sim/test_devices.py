###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

'''Unit tests for devices.'''

import devices
import pytest


# *** Fixtures ***

@pytest.fixture
def a_network():
    network = devices.Network()
    network.add_devices_with_links(6, devices.Metric(100))
    return network


def add_bridged_link(device1, device2):
    link = device1.links[device2][0]
    device1.bridged_links.add(link)
    device2.bridged_links.add(link)


@pytest.fixture
def chain_network(a_network):
    prev_device = a_network.devices[0]
    for device in a_network.devices[1:]:
        add_bridged_link(prev_device, device)
        prev_device = device
    return a_network


@pytest.fixture
def star_network(a_network):
    gateway = a_network.devices[0]
    for device in a_network.devices[1:]:
        add_bridged_link(gateway, device)
    return a_network


@pytest.fixture
def tree_network(a_network):
    # Algorithmically constructing a tree is not so trivial, so just do it explicitly
    (d0, d1, d2, d3, d4, d5) = a_network.devices
    add_bridged_link(d0, d1)
    add_bridged_link(d0, d2)
    add_bridged_link(d1, d3)
    add_bridged_link(d1, d4)
    add_bridged_link(d2, d5)
    return a_network


# *** Device.neighbours() ***

def assert_expected_neighbors(network, device_idx, expected_idxes):
    '''Helper function for neighbor tests that checks that device neighbors are as expected.'''
    device = network.devices[device_idx]
    expected = {network.devices[expected_idx] for expected_idx in expected_idxes}
    assert set(device.neighbors()) == expected


def test_neighbors_chain(chain_network):
    '''Test that the neighbors() function returns as expected in a chain network.'''
    assert_expected_neighbors(chain_network, 0, [1])
    assert_expected_neighbors(chain_network, -1, [-2])
    for idx in range(1, len(chain_network.devices) - 1):
        assert_expected_neighbors(chain_network, idx, [idx - 1, idx + 1])


def test_neighbors_star(star_network):
    '''Test that the neighbors() function returns as expected in a star network.'''
    assert_expected_neighbors(star_network, 0, range(1, len(star_network.devices)))
    for idx in range(1, len(star_network.devices)):
        assert_expected_neighbors(star_network, idx, [0])


def test_neighbors_tree(tree_network):
    '''Test that the neighbors() function returns as expected in a tree network.'''
    assert_expected_neighbors(tree_network, 0, [1, 2])
    assert_expected_neighbors(tree_network, 1, [0, 3, 4])
    assert_expected_neighbors(tree_network, 2, [0, 5])


# *** Network.check_connectivity() ***

def assert_backhaul_tree_connected(network):
    backhaul_tree = network.calculate_backhaul_tree()
    assert set(backhaul_tree.keys()) == set(network.devices), "Network not fully connected"
    assert len(backhaul_tree[network.devices[0]]) == 0
    return backhaul_tree


def assert_backhaul_path(network, backhaul_tree, device_idxes):
    '''Helper function that checks for a specific backhaul path.

    device_idxes is a list of device indexes starting at a leaf and ending at the gateway.
    '''
    prev_device = network.devices[device_idxes[0]]
    backhaul_path = backhaul_tree[prev_device]
    for link, device_idx in zip(backhaul_path, device_idxes[1:]):
        device = network.devices[device_idx]
        assert link.devices == set((prev_device, device))
        prev_device = device


def test_connectivity_chain(chain_network):
    backhaul_tree = assert_backhaul_tree_connected(chain_network)
    assert_backhaul_path(chain_network, backhaul_tree,
                         range(len(chain_network.devices) - 1, -1, -1))


def test_connectivity_star(star_network):
    backhaul_tree = assert_backhaul_tree_connected(star_network)
    for device in star_network.devices[1:]:
        assert_backhaul_path(star_network, backhaul_tree, (device.idx, 0))


def test_connectivity_tree(tree_network):
    backhaul_tree = assert_backhaul_tree_connected(tree_network)
    assert_backhaul_path(tree_network, backhaul_tree, (3, 1, 0))
    assert_backhaul_path(tree_network, backhaul_tree, (4, 1, 0))
    assert_backhaul_path(tree_network, backhaul_tree, (5, 2, 0))


def test_connectivity_chain_broken(chain_network):
    broken_link = chain_network.devices[3].links[chain_network.devices[2]][0]
    chain_network.devices[3].bridged_links.remove(broken_link)
    chain_network.devices[2].bridged_links.remove(broken_link)
    backhaul_tree = chain_network.calculate_backhaul_tree()
    assert_backhaul_path(chain_network, backhaul_tree, (2, 1, 0))
    for idx in range(3, len(chain_network.devices)):
        assert chain_network.devices[idx] not in backhaul_tree


def test_connectivity_chain_loop(chain_network):
    device_1 = chain_network.devices[1]
    device_4 = chain_network.devices[4]
    add_bridged_link(device_1, device_4)
    with pytest.raises(chain_network.LoopDetected) as loop_detected:
        chain_network.calculate_backhaul_tree()
    devices_in_loop = set()
    for link in loop_detected.value.links:
        devices_in_loop.update(link.devices)
    assert {device.idx for device in devices_in_loop} == set(range(4, 0, -1))
