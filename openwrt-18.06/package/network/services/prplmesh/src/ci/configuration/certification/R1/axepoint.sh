#!/bin/sh

set -e

# One of the LAN ports is used for control, and the WAN port for data:
uci batch << 'EOF'
set network.cert=interface
set network.cert.proto='static'
set network.cert.ifname='eth0_4'
set network.cert.ipaddr='192.168.250.180'
set network.cert.netmask='255.255.255.0'
set network.lan.ifname='eth0_1 eth0_2 eth0_3 eth1'
EOF

# Wired backhaul interface:
uci set prplmesh.config.backhaul_wire_iface='eth1'

uci commit
/etc/init.d/network restart
