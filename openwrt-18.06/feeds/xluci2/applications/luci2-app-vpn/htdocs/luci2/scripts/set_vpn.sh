#!/bin/sh

enable=$1

if [ "x$enable" == "x1" ]; then
	uci set network.vpn='interface'
	uci set network.vpn.ifname='tun0'
	uci set network.vpn.proto='none'
	uci add_list firewall.@zone[2].network='vpn'
else
	uci delete network.vpn
	uci del_list firewall.@zone[2].network='vpn'
fi

uci commit
/etc/init.d/firewall reload &
/etc/init.d/network reload
