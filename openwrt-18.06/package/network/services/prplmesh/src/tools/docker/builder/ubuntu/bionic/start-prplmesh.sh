#!/bin/bash
###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

run() {
    echo "$*"
    "$@" || exit $?
}


# Use the ip address that was allocated by the daemon to this
# container. The IP of the second docker network (eth1) is used,
# because the first one is by default the one used for exposed ports,
# and we rely on exposed ports for UCC communication.
bridge_ip="$(ip addr show dev eth1 | awk '/^ *inet / {print $2}')"

run ip link add          br-lan   type bridge
run ip link add          eth0_1   type dummy
run ip link add          eth0_2   type dummy
run ip link add          eth0_3   type dummy
run ip link add          eth0_4   type dummy
run ip link add          wlan0    type dummy
run ip link add          wlan2    type dummy

# When an interface is added to the bridge, the bridge inherits its MAC address.
# It shouldn't be the same as any other interface because that messes up the topology in the
# controller, however. Therefore, save the MAC address an re-apply it later.
bridge_mac="$(ip link show dev br-lan | awk '/^ *link\/ether / {print $2}')"

run ip link set      dev eth1     master br-lan
run ip link set      dev eth0_1   master br-lan
run ip link set      dev eth0_2   master br-lan
run ip link set      dev eth0_3   master br-lan
run ip link set      dev eth0_4   master br-lan
run ip link set      dev wlan0    master br-lan
run ip link set      dev wlan2    master br-lan
run ip address flush dev eth1
run ip link set      dev eth0_1   up
run ip link set      dev eth0_2   up
run ip link set      dev eth0_3   up
run ip link set      dev eth0_4   up
run ip link set      dev wlan0    up
run ip link set      dev wlan2    up

run ip link set      dev br-lan   addr "$bridge_mac"
run ip address add   dev br-lan "$bridge_ip"
run ip link set      dev br-lan   up

cd "${INSTALL_DIR}" || exit 1

type="$1"; shift

if [ "$type" = "start-agent" ]; then
    start_arg=(--mode a)
else
    start_arg=(--mode ca)
fi

# After new change for ubus, socket creates in /var/run/ubus
mkdir /var/run/ubus
ubusd &

"${INSTALL_DIR}/scripts/prplmesh_utils.sh" start "${start_arg[@]}" "$@"

tail -f /dev/null # hack so the script will not exit forcing the container to stop
