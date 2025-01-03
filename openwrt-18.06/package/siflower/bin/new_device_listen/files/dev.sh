#!/bin/sh

if [ $2 -eq 1 ]; then
	timeout=30
	interval=5
	start_time=$(date +%s)
	mac=`echo ${3//:/_} | tr '[a-z]' '[A-Z]'`
	dev_mac=`echo ${mac//_/:}`
	tscript add $dev_mac
	result=`uci get devlist.$mac.mac`
	mode=$(uci get network.lan.proto)

	if [ -e "/tmp/dev.log" ]; then
		rm -f  "/tmp/dev.log"
	fi

	echo "$dev_mac" >>  /tmp/dev.log
	
    if [ $1 -eq 2 ] &&  [ "$mode" == "dhcp" ] ;then

		# Check if/etc/nodogspash/ip_monitor.sh or/etc/nodogspash/set_made.ap.sh is running
		if pgrep -f "/etc/nodogsplash/ip_monitor.sh" > /dev/null ||  pgrep -f "/etc/nodogsplash/second_ip_monitor.sh" > /dev/null || pgrep -f "/etc/nodogsplash/set_mode_ap.sh" > /dev/null; then
			echo "Either /etc/nodogsplash/ip_monitor.sh or /etc/nodogsplash/set_mode_ap.sh is running. Exiting." >> /tmp/dev.log
			exit 0
		fi
		
		# Distinguish between wired bridging and wireless relay
        REPEATER_CONNET_STATUS=$(cat /etc/config/wireless | grep wds)

		# It's not a wireless relay, it's a wired bridge
        if [ -z "$REPEATER_CONNET_STATUS" ] ;then
			if [ -z "$result"];then
				# The superior has changed
				if [ -f "/etc/nodogsplash/ap_upper_change.sh" ] &&  [ ! -e "/tmp/ap_upper_change.lock" ];then
					echo "mac change" >> /tmp/dev.log
					rm -f  /tmp/dhcp.leases
					/etc/nodogsplash/ap_upper_change.sh &
				else
					echo "/etc/nodogsplash/ap_upper_change.sh or /tmp/ap_upper_change.lock exist!"
					exit 0
				fi
				udhcpc -i br-lan -n 10 -q # Exit immediately after obtaining the IP address,timeout:10s
				wifi
			else
				ip=$(uci -d / show devlist | grep "$mac" | grep ip | awk -F"'" '{print $2}')
				PING_OUTPUT=$(ping -c 4  -w 1 $ip 2>&1)
				if echo "$PING_OUTPUT" | grep -q "64 bytes from $ip"; then
					# The higher-level network segment has not been changed
					echo "no change" >> /tmp/dev.log
				else
					# The higher-level network segment has been changed
					echo "ip change" >> /tmp/dev.log
					rm -f  /tmp/dhcp.leases
					if [ -f "/etc/nodogsplash/ap_upper_change.sh" ];then
						/etc/nodogsplash/ap_upper_change.sh &
					fi
					udhcpc -i br-lan -n 10 -q # Exit immediately after obtaining the IP address,timeout:10s
					wifi
				fi
			fi
			echo "wired bridge" >> /tmp/dev.log
		else
			echo "wireless relay" >> /tmp/dev.log
        fi
    fi

	if [ -z "$3" ];then
		echo "[$0] err: not give mac, please check" > /dev/console
		return
	fi
	while true; do
		ip=`grep \$3 /tmp/dhcp.leases | awk '{print \$3}'`
		if [ -n "$ip" ]; then
				break;
		fi

		ip=`arp-scan -l -g | grep \$3 | awk '{print \$1}'`
		if [ -n "$ip" ]; then
			break;
		fi

		current_time=$(date +%s)
		elapsed=$((current_time - start_time))
		if [ $elapsed -ge $timeout ]; then
			return
		fi

		sleep $interval
	done
	hostname=`grep $3 /tmp/dhcp.leases | awk '{print $4}'`
	if [ -z "$result" ];then
		uci set devlist.$mac=device
		uci set devlist.$mac.mac=$mac
		uci set devlist.$mac.internet=1
		uci set devlist.$mac.hostname=$hostname
	fi
	uci set devlist.$mac.ip=$ip
	uci set devlist.$mac.port=$1
	uci set devlist.$mac.online=1
	timestamp=`date "+%s"`
	uci set devlist.$mac.latest_time=$timestamp
	uci commit devlist
elif [ $2 -eq 0 ]; then
	dev_mac=`echo ${mac//_/:}`
	tscript del $dev_mac
	port=$1
	tmp=`uci -d / show devlist | grep "port='$port'" | awk -F . '{print $2}'`
	for mac in $tmp
	do
		uci set devlist.$mac.online=0
	done
	uci commit devlist
else
	tmp=`cat sys/kernel/debug/esw_debug | grep "link 1" | awk '{print $1}' | sed 's/phy//'`
	for port in $tmp
	do
		echo renetlink $port > sys/kernel/debug/esw_debug
	done
fi