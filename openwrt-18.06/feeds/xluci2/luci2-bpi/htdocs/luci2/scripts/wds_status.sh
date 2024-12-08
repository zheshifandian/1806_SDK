#!/bin/sh

set_config(){
	device=`uci get wireless.wds.device`
	oc=`uci get wireless.$device.origin_channel`
	if [ -n "$oc" ]; then
		uci set wireless.$device.channel=$oc
	fi
	uci del wireless.wds
	ip=`uci get network.lan.oip`
	if [ -n "$ip" ]; then
		uci set network.lan.proto='static'
		uci set network.lan.ipaddr=$ip
		uci set network.lan.netmask='255.255.255.0'
		uci set network.lan.ip6assign='60'
	fi
	uci commit
	/etc/init.d/network restart
}

check_lan_ip(){
	sleep 5
	ip=`ubus call network.interface.lan status | grep "ipv4-address" -C 3 | grep '"address"' | awk -F '"' '{print $4}'`
	if [ "$ip" == "" ] ;then
		set_config
	else
		/etc/init.d/dnsmasq stop
	fi
}

sleep 5
if [ "$1" == "wlan0-1" ]
	status=`cat /sys/kernel/debug/ieee80211/phy0/siwifi/repeater_status`
else
	status=`cat /sys/kernel/debug/ieee80211/phy1/siwifi/repeater_status`
fi
	encryption=`uci get wireless.wps.encryption`

if [ "$encryption" == "none" ] && [ "$status" == "repeater assoc!" ] ; then
	check_lan_ip
elif [ "$status" == "repeater eapol!" ] ; then
	check_lan_ip
else
	wifi
	sleep 5
	if [ "$1" == "wlan0-1" ]
		status=`cat /sys/kernel/debug/ieee80211/phy0/siwifi/repeater_status`
	else
		status=`cat /sys/kernel/debug/ieee80211/phy1/siwifi/repeater_status`
	fi
	if [ "$encryption" == "none" ] && [ "$status" == "repeater assoc!" ] ; then
		check_lan_ip
	elif [ "$status" == "repeater eapol!" ] ; then
		check_lan_ip
	else
		set_config
	fi
fi
