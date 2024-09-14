#!/bin/sh

set -e

# We use WAN for the control interface:
uci batch << 'EOF'
set network.control=interface
set network.control.type='bridge'
set network.control.proto='static'
set network.control.netmask='255.255.255.0'
set network.control.ipaddr='192.168.250.170'
set network.control.ifname='eth2'
del network.wan
set network.lan.ipaddr=192.165.100.170
EOF

# Wired backhaul interface:
uci set prplmesh.config.backhaul_wire_iface='lan0'

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
