#!/bin/sh

set -e

# IP for device upgrades, operational tests, Boardfarm data network, ...
uci set network.lan.ipaddr='192.168.1.130'

# VLAN interface to control the device separatly:
uci batch << 'EOF'
set network.UCC=interface
set network.UCC.ifname='eth0_1.200'
set network.UCC.proto='static'
set network.UCC.netmask='255.255.255.0'
set network.UCC.ipaddr='192.168.200.130'
EOF

# Wired backhaul interface:
uci set prplmesh.config.backhaul_wire_iface='eth0_1'

uci commit
/etc/init.d/network restart
