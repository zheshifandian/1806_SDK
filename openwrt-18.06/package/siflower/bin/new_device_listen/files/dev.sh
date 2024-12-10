#!/bin/sh

g_devIp=""
g_lanMac=""


is_router_mac() {
	mac=$1

	result=`echo $mac | grep "10:16:88"`
	if [ -n "$result" ]; then
		return 1
	fi
	return 0
}

mac_to_ip() {
	mac=$1
	timeout=0

	g_devIp=`arp-scan -l -g | grep $mac | awk '{print $2}'`
	while [ -z "$g_devIp" -a $timeout -lt 4 ]
	do
		sleep 2
		timeout=$(($timeout + 2))
		g_devIp=`arp-scan -l -g | grep $mac | awk '{print $2}'`
	done
}

staMac_to_lanMac() {
	timeout=0
	staMac=$1
	prefix=${staMac%:*}

	# lanMac=eth0=br-lan, 2.4G-start=(eth0+2), 5G-start=(eth0+6)
	last=`echo $staMac|cut -d : -f6`
	last=`printf %u 0x$last`
	if [ $last -lt 9 ]; then
		. /lib/functions/system.sh
		mac=$(macaddr_sub "$staMac" 9)
		prefix_extenal=${mac%:*}
	fi

	dhcp_info=`cat /tmp/dhcp.leases| grep $prefix`
	while [ -z "$dhcp_info" -a $timeout -lt 20 ]
	do
		if [ -n "$prefix_extenal" ]; then
			dhcp_info=`cat /tmp/dhcp.leases| grep $prefix_extenal`
			if [ -n "$dhcp_info" ]; then
				break
			fi
		fi
		sleep 2
		timeout=$(($timeout + 2))
		dhcp_info=`cat /tmp/dhcp.leases| grep $prefix`
	done
	g_lanMac=`echo $dhcp_info |awk '{print $2}'`
	g_devIp=`echo $dhcp_info |awk '{print $3}'`
}

set_all_ethernet_dev_offline() {
	tmp=`uci -d / show devlist | grep "online='1'" | awk -F . '{print $2}'`
	for devMac in $tmp
	do
		uci set devlist.$devMac.online=0

		# remove monitor device traffic statistics
		mac=`echo ${devMac//_/:} | tr '[A-Z]' '[a-z]'`
		tscript del $mac
	done

	tmp=`uci -d / show meshlist | grep "is_wireless='0'" | awk -F . '{print $2}'`
	for devMac in $tmp
	do
		online=`uci get meshlist.$devMac.online`
		if [ $online -eq 1 ]; then
			uci set meshlist.$devMac.online=0

			# remove monitor device traffic statistics
			mac=`echo ${devMac//_/:} | tr '[A-Z]' '[a-z]'`
			tscript del $mac
		fi
	done
}

search_online_eth_dev() {
	> /tmp/mesh

	tmp=`uci -d / show devlist | grep "online='1'" | awk -F . '{print $2}'`
	if [ -n "$tmp" ]; then
		echo "$tmp" >> /tmp/mesh
	fi

	tmp=`uci -d / show meshlist | grep "is_wireless='0'" | awk -F . '{print $2}'`
	for devMac in $tmp
	do
		online=`uci get meshlist.$devMac.online`
		if [ $online -eq 1 ]; then
			type=`uci get meshlist.$devMac.type`
			if [ "$type" != "master" ]; then
				echo "$devMac" >> /tmp/mesh
			fi
		fi
	done
}

search_online_wireless_cnt() {
	count=0

	tmp=`uci -d / show wldevlist | grep "online='1'" | wc -l`
	count=$(($count + $tmp))

	tmp=`uci -d / show meshlist | grep "is_wireless='1'" | awk -F . '{print $2}'`
	for devMac in $tmp
	do
		online=`uci get meshlist.$devMac.online`
		if [ $online -eq 1 ]; then
			count=$(($count + 1))
		fi
	done
	return $count
}

search_is_router_online() {
	arpInfo=$1
	count=1

	for tmp in $arpInfo
	do
		if [ $((count % 2)) -ne 0 ]; then
			# odd is ip
			devIp=$tmp
		else
			# even is mac
			mac=$tmp
			devMac=`echo ${mac//:/_} | tr '[a-z]' '[A-Z]'`
			is_router_mac $mac
			if [ $? -eq 1 ]; then
				is_wireless=`uci get meshlist.$devMac.is_wireless`
				result=`grep "$devMac" /tmp/mesh`
				if [ -z "$result" -a "$is_wireless" != "1" ]; then
					return 1
				fi
			fi
		fi
		count=$((count + 1))
	done
	return 0
}

wait_wireless_offline() {
	mac=$1
	timeout=0

	while [ $timeout -lt 10 ]
	do
		online=`uci get meshlist.$mac.online`
		if [ $online -eq 0 ]; then
			uci set meshlist.$mac.online=1
			uci set meshlist.$mac.is_wireless=0
			break
		fi
		sleep 2
		timeout=$(($timeout + 2))
	done
}

is_wireless_to_ethernet() {
	port=$1
	updown=$2
	rtMac=$3
	restart=$4

	slave_info=`uci -d / show meshlist | grep "rtMac='$rtMac'" | awk -F . '{print $2}'`
	for mac in $slave_info
	do
		is_wireless=`uci get meshlist.$mac.is_wireless`
		online=`uci get meshlist.$mac.online`
		if [ "$is_wireless" == "1" -a $online -eq 1 -a $updown -eq 1 ]; then
			if [ "$restart" == "" ]; then
				/sbin/dev.sh 0 $port $updown $rtMac 1 &
				exit
			else
				wait_wireless_offline $mac
			fi
		fi
	done
	uci commit meshlist
}

sync_stat_to_slave_dev() {
	config=$1
	rt_mac=$2
	updown=$3
	arpInfo=$4

	slave_info=`uci -d / show $config | grep "rtMac='$rt_mac'" | awk -F . '{print $2}'`
	for mac in $slave_info
	do
		macaddr=`echo ${mac//_/:} | tr '[A-Z]' '[a-z]'`
		result=`echo $arpInfo |grep $macaddr`
		if [ -n "$result" -a $updown -eq 1 ]; then
			uci set $config.$mac.online=1
		elif [ -z "$result" -a $updown -eq 0 ]; then
			uci set $config.$mac.online=0
		fi
	done

	uci commit $config
}

rtMac_stat_sync() {
	rt_mac=$1
	updown=$2
	is_wireless=$3
	arpInfo=$4
	loop=0
	old_loop=0

	while true
	do
		if [ $loop -gt 0 ]; then
			rt_mac=`eval echo "\$mac_$old_loop"`
		fi

		slave_info=`uci -d / show meshlist | grep "rtMac='$rt_mac'" | awk -F . '{print $2}'`
		for mac in $slave_info
		do
			macaddr=`echo ${mac//_/:} | tr '[A-Z]' '[a-z]'`
			result=`echo $arpInfo | grep $macaddr`
			if [ -n "$result" -a $updown -eq 1 ]; then
				uci set meshlist.$mac.online=1
			elif [ -z "$result" -a $updown -eq 0 ]; then
				uci set meshlist.$mac.online=0
			fi
			sync_stat_to_slave_dev devlist $macaddr $updown "$arpInfo"
			sync_stat_to_slave_dev wldevlist $macaddr $updown "$arpInfo"

			loop=$(($loop + 1))
			eval mac_$loop=$macaddr
		done

		if [ -z "$slave_info" ]; then
			sync_stat_to_slave_dev devlist $rt_mac $updown "$arpInfo"
			sync_stat_to_slave_dev wldevlist $rt_mac $updown "$arpInfo"
		fi

		if [ $loop -eq $old_loop ]; then
			break;
		fi
		old_loop=$(($old_loop + 1))
	done
}

update_meshlist() {
	mac=$1
	ip=$2
	updown=$3
	rtMac=$4
	is_wireless=$5
	arpInfo=$6
	devMac=`echo ${mac//:/_} | tr '[a-z]' '[A-Z]'`

	result=`uci get meshlist.$devMac.mac`
	if [ -z "$result" ];then
		uci set meshlist.$devMac=device
		uci set meshlist.$devMac.type=slave
		uci set meshlist.$devMac.mac=$mac
		uci set meshlist.$devMac.rtMac=$rtMac
		uci set meshlist.$devMac.is_wireless=$is_wireless
		uci set meshlist.$devMac.online=$updown
	else
		old_online=`uci get meshlist.$devMac.online`
		if [ $is_wireless -eq 0 -a $old_online -eq 0 ]; then
			uci set meshlist.$devMac.rtMac=$rtMac
			uci set meshlist.$devMac.is_wireless=$is_wireless
		elif [ $is_wireless -eq 1 ]; then
			uci set meshlist.$devMac.rtMac=$rtMac
			uci set meshlist.$devMac.is_wireless=$is_wireless
		fi
	fi

	uci set meshlist.$devMac.ip=$ip
	uci set meshlist.$devMac.online=$updown
	rtMac_stat_sync $mac $updown $is_wireless "$arpInfo"
	uci commit meshlist
}

update_wldevlist() {
	mac=$1
	mac=`echo $mac | tr '[A-Z]' '[a-z]'`
	devName=$2
	updown=$3
	rtMac=$4
	devMac=`echo ${mac//:/_} | tr '[a-z]' '[A-Z]'`

	if [ -z "$rtMac" ]; then
		# means direct connect to mesh master
		rtMac=`uci -d / show meshlist | grep "type='master'" | awk -F . '{print $2}'`
		rtMac=`echo ${rtMac//_/:} | tr '[A-Z]' '[a-z]'`
	fi

	is_router_mac $mac
	if [ $? -eq 1 ]; then
		staMac_to_lanMac $mac
		if [ -z "$g_lanMac" ]; then
			echo "[mesh error] sta mac to lan mac fail" > /dev/ttyS0
			return
		fi
		arpInfo=`arp-scan -l -g | grep "Unknown" | sed 's/(Unknown)//g'`
		update_meshlist $g_lanMac $g_devIp $updown $rtMac 1 "$arpInfo"
	else
		mac_to_ip $mac
		devIp=$g_devIp
		hostname=`grep $mac /tmp/dhcp.leases | awk '{print $4}'`
		result=`uci get wldevlist.$devMac.mac`
		if [ -z "$result" ];then
			uci set wldevlist.$devMac=device
			uci set wldevlist.$devMac.mac=$devMac
			uci set wldevlist.$devMac.internet=1
			uci set wldevlist.$devMac.lan=1
			uci set wldevlist.$devMac.port=1
			uci set wldevlist.$devMac.warn=0
			uci set wldevlist.$devMac.is_wireless=1
			uci set wldevlist.$devMac.hostname=$hostname
		fi
		uci set wldevlist.$devMac.dev=$devName
		uci set wldevlist.$devMac.ip=$devIp
		uci set wldevlist.$devMac.rtMac=$rtMac
		uci set wldevlist.$devMac.online=$updown
		timestamp=`date "+%s"`
		uci set wldevlist.$devMac.latest_time=$timestamp
	fi
	uci commit wldevlist
}

update_devlist() {
	devPort=$1
	updown=$2
	rtMac=$3
	restart=$4
	is_router_online=0

	if [ -z "$rtMac" ]; then
		# means direct connect to mesh master
		rtMac=`uci -d / show meshlist | grep "type='master'" | awk -F . '{print $2}'`
		rtMac=`echo ${rtMac//_/:} | tr '[A-Z]' '[a-z]'`
	fi

	search_online_eth_dev
	online_ethdev_cnt=`cat /tmp/mesh | wc -l`
	search_online_wireless_cnt
	online_wire_cnt=$?
	online_dev_cnt=$(($online_ethdev_cnt + $online_wire_cnt))

	if [ $updown -eq 0 ]; then
		set_all_ethernet_dev_offline
	else
		is_wireless_to_ethernet $devPort $updown $rtMac $restart
	fi

	index=1
	arpInfo=`arp-scan -l -g | grep "Unknown" | sed 's/(Unknown)//g'`
	arp_cnt=`echo $arpInfo |awk '{print NF}'`
	arp_dev_cnt=$(($arp_cnt / 2))
	if [ $online_dev_cnt -eq $arp_dev_cnt ]; then
		sleep 6
		arpInfo=`arp-scan -l -g | grep "Unknown" | sed 's/(Unknown)//g'`
	fi

	if [ $updown -eq 1 ]; then
		search_is_router_online "$arpInfo"
		is_router_online=$?
	fi

	for tmp in $arpInfo
	do
		if [ $((index % 2)) -ne 0 ]; then
			# odd is ip
			devIp=$tmp
		else
			# even is mac
			mac=$tmp
			hostname=`grep $mac /tmp/dhcp.leases | awk '{print $4}'`
			devMac=`echo ${mac//:/_} | tr '[a-z]' '[A-Z]'`

			is_router_mac $mac
			if [ $? -eq 1 ]; then
				is_wireless=`uci get meshlist.$devMac.is_wireless`
				online=`uci get meshlist.$devMac.online`
				if [ "$is_wireless" == "1" -a "$online" == "0" -a $updown -eq 1 ]; then
					#means wireless transfer to ethernet
					update_meshlist $mac $devIp 1 $rtMac 0 "$arpInfo"
				elif [ "$is_wireless" == "0" -a "$online" == "0" -a $updown -eq 1 ]; then
					#means router change rtMac
					update_meshlist $mac $devIp 1 $rtMac 0 "$arpInfo"
					exit
				elif [ "$is_wireless" == "" -a $updown -eq 1 ]; then
					#means new router found
					update_meshlist $mac $devIp 1 $rtMac 0 "$arpInfo"
				else
					uci set meshlist.$devMac.online=1
				fi
			else
				if [ $is_router_online -eq 1 ]; then
					index=$(($index + 1))
					continue;
				fi
				# judge is the new mac is wireless devMac
				result=`uci get wldevlist.$devMac.mac`
				if [ -z "$result" ]; then
					result=`uci get devlist.$devMac.mac`
					if [ -z "$result" ];then
						# means new ethernet dev found
						uci set devlist.$devMac=device
						uci set devlist.$devMac.mac=$devMac
						uci set devlist.$devMac.internet=1
						uci set devlist.$devMac.hostname=$hostname
						uci set devlist.$devMac.port=$devPort
						uci set devlist.$devMac.rtMac=$rtMac
						timestamp=`date "+%s"`
						uci set devlist.$devMac.latest_time=$timestamp
					else
						# check if we need to update rtMac
						result=`cat /tmp/mesh |grep $devMac`
						if [ -z "$result" ]; then
							uci set devlist.$devMac.port=$devPort
							uci set devlist.$devMac.rtMac=$rtMac
							timestamp=`date "+%s"`
							uci set devlist.$devMac.latest_time=$timestamp
						fi
					fi

					uci set devlist.$devMac.ip=$devIp
					uci set devlist.$devMac.online=1

					# monitor device traffic statistics
					tscript add $mac
				fi
			fi
		fi
		index=$(($index + 1))
	done
	uci commit devlist
	uci commit meshlist
}


is_wireless=$1
if [ $is_wireless -eq 0 ]; then
	devPort=$2
	updown=$3
	rtMac=$4
	restart=$5
	update_devlist $devPort $updown $rtMac $restart
elif [ $is_wireless -eq 1 ]; then
	mac=$2
	devName=$3
	updown=$4
	rtMac=$5
	update_wldevlist $mac $devName $updown $rtMac
elif [ $is_wireless -eq 2 ]; then
	# init meshlist
	mac=`uci get network.lan.macaddr`
	devMac=`echo ${mac//:/_} | tr '[a-z]' '[A-Z]'`
	ip=`ifconfig br-lan|grep "inet addr"|awk '{print $2}'|cut -d : -f2`

	result=`uci get meshlist.$devMac.mac`
	if [ -z "$result" ];then
		uci set meshlist.$devMac=device
		uci set meshlist.$devMac.type=master
		uci set meshlist.$devMac.mac=$mac
		uci set meshlist.$devMac.ip=$ip
		uci set meshlist.$devMac.online=1
		uci set meshlist.$devMac.is_wireless=0
		uci commit meshlist
	fi
else
	tmp=`cat /sys/kernel/debug/esw_debug | grep "link 1" | awk '{print $1}' | sed 's/phy//'`
	for port in $tmp
	do
		echo renetlink $port > /sys/kernel/debug/esw_debug
	done
fi
