#!/bin/sh
# $1: updown
# $2: mac
# $3: is_wifi
# $4: ifname
# $5: port
# $6: vlan_id

logger -t device_listen: mac:$2 updown:$1 is_wifi:$3 ifname:$4 port:$5 vlan_id:$6

mac=`echo ${2//:/_} | tr '[a-z]' '[A-Z]'`

if [ $1 -eq 1 ]; then
	# ignore wan device
	if [ $3 -eq 0 ]; then
		wan_device=`uci get network.wan.device | awk -F '.' '{print$2}'`
		if [ "$wan_device" == "$6" ]; then
			logger -t device_listen: is wan mac:$2 updown:$1
			return;
		else
			wan_device=`uci get network.wan.device`
			if [ "$wan_device" == "$4" ]; then
				logger -t device_listen: is wan mac:$2 updown:$1
				return;
			fi
		fi
	fi
	# sleep for get ip
	sleep 4
	dev_mac=`echo ${mac//_/:}`
	result=`uci get devlist.$mac`
	if [ -z "$result" ];then
 		uci set devlist.$mac=device
		uci set devlist.$mac.mac=$dev_mac
	fi

	uci set devlist.$mac.is_wifi=$3
	uci set devlist.$mac.port=$5
 	uci set devlist.$mac.ifname=$4

	uci set devlist.$mac.online=1
	timestamp=`date "+%s"`
	uci set devlist.$mac.latest_time=$timestamp

	ip=`grep $2 /proc/net/arp  | awk '{print $1}'`
	if [ -n "$ip" ]; then
		uci set devlist.$mac.ip=$ip
	fi

	hostname=`grep $2 /tmp/dhcp.leases | awk '{print $4}'`
	if [ -z "$hostname" ]; then
		hostname="Anonymous"
	fi
	uci set devlist.$mac.hostname=$hostname
	uci set devlist.$mac.vlan_id=$6
	uci commit devlist

elif [ $1 -eq 0 ]; then
	uci set devlist.$mac.online=0
	uci commit devlist
fi