#!/bin/sh

sleep 2

namemac=$1
up_mac=${namemac//_/:}
low_mac=`echo $up_mac | tr '[A-Z]' '[a-z]'`
ip=`cat /proc/net/arp | grep $low_mac | tail -n 1 | awk -F ' ' '{print $1}' | tr -d "\n"`

uci -q set wldevlist.$namemac.ip=$ip
uci commit
