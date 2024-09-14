#!/bin/sh

set -e

# IP for device upgrades, operational tests, Boardfarm data network, ...
# Note that this device uses the WAN interface (as on some Omnias the
# others don't work in the bootloader):
uci batch << 'EOF'
set network.wan.proto='static'
set network.wan.netmask='255.255.255.0'
set network.wan.ipaddr='192.168.1.100'
set network.lan.ipaddr='192.168.0.100'
EOF

# Wired backhaul interface:
uci set prplmesh.config.backhaul_wire_iface='eth2'

# Create a hole for SSH on WAN:
uci add firewall rule
uci batch << 'EOF'
set firewall.@rule[-1].name='SSH'
set firewall.@rule[-1].src='wan'
set firewall.@rule[-1].dest_port='22'
set firewall.@rule[-1].target='ACCEPT'
set firewall.@rule[-1].proto='tcp'
set firewall.@rule[-1].enabled='yes'
EOF
uci commit firewall
/etc/init.d/firewall restart

uci commit
/etc/init.d/network restart
