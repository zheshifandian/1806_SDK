#!/bin/sh

set_wireless() {
	local iface=$1
	local ssid=$2

	uci set wireless.$iface.network=wwan
	uci set wireless.$iface.mode=sta
	uci set wireless.$iface.ssid=$ssid
	uci set wireless.$iface.encryption=none
	uci commit
}


iface2G=`uci show wireless | grep "ifname='wlan0'" | awk -F '.' '{print $2}'`
iface5G=`uci show wireless | grep "ifname='wlan1'" | awk -F '.' '{print $2}'`

while [ 1 ]; do
	random=`cat /proc/sys/kernel/random/uuid | cksum | cut -f1 -d" "`
	if [ $(($random%2)) -eq 0 ]; then
		if=wlan0
		iface=$iface2G
	else
		if=wlan1
		iface=$iface5G
	fi
	scaned_ssid=
	while [ -z "$scaned_ssid" ];do
		scaned_ssid=`iw $if scan | grep SSID | grep zq | awk '{print $2}'`
	done
	ssid=`random_select $scaned_ssid`
	set_wireless $iface $ssid

	wifi reload
	timeout=0
	connect=`iw dev $if link | grep Connected`
	while [ -z "$connect" ] && [ $timeout -lt 10 ]; do
		connect=`iw dev $if link | grep Connected`
		sleep 3
		timeout=$(($timeout+3))
	done
	#[ $timeout -ge 30 ] && exit 2
done
