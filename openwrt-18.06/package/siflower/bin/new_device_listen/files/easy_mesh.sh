#!bin/sh

. /lib/functions/system.sh
. /lib/netifd/wireless/mac80211.sh

mac=""

send_netlink()
{
	count=0
	while [ $count -lt 60 ];
	do
		if ping -c 2 192.168.4.1 > /dev/null
		then
			logger -t easymesh device_listen connect success
			tmp=`cat /sys/kernel/debug/esw_debug | grep "link 1" | awk '{print $1}' | sed 's/phy//'`
			wan=`uci get network.@switch_vlan[1].ports | tr -d '5t'`
			for port in $tmp
			do
				if [ $port != $wan ]; then
					echo renetlink $port > /sys/kernel/debug/esw_debug
				fi
			done
			let count=60
		else
			sleep 1
			let count=$count+1
		fi
	done
}

agent_status()
{
	count=0
	while [ $count -lt 120 ];
	do
		status=`cat /tmp/agent_is_ok`
		if [ "$status" == "0" ]; then
			sleep 1
			let count=$count+1
		else
			let count=120
			logger -t easymesh device_listen agent is ok
		fi
	done
}

wan_link()
{
	sleep 2
	wan=`uci get network.@switch_vlan[1].ports | tr -d '5t'`
	port=phy$wan
	link=`cat /sys/kernel/debug/esw_debug | grep $port | awk -F ' ' '{print$3}'`
	return $link
}

get_macaddr()
{
	macidx=7
	mac="$(mac80211_generate_mac $1)"
}

type=`ubus call web.wireless easymesh_get_mesh_type | grep type | awk -F ' ' '{print$2}' | tr -d '\"'`

if [ "$type" == "not support" ]; then
	logger -t easymesh device_listen prplmesh is not support
	return

elif [ "$type" == "disabled" ]; then
	logger -t easymesh device_listen prplmesh is disabled
	return

elif [ "$type" == "controller" ]; then
	logger -t easymesh device_listen controller
	return

else
	# if the wan is up, turn to controller or wired connect agent
	if [ $1 == 1 ]; then
		wan_link
		if [ $? -eq 0 ]; then
			logger -t easymesh device_listen just network restart
			return
		fi
		ifconfig wlan0-2 down
		ifconfig wlan1-2 down
		# get the agent type now
		lan_mac=`uci get network.lan.macaddr`
		mac=$(macaddr_add "$lan_mac" +11)
		# if agent connect with wlan0, change to wired
		if [ "$type" == "wlan0" ]; then
			logger -t easymesh device_listen wlan0 change to wired connect
			uci set wireless.default_radio11.disabled='1'
			uci set wireless.default_radio22.disabled='1'
			uci set network.lan_dev.macaddr=$mac
			uci commit
			/etc/init.d/prplmesh restart
			agent_status
			logger -t easymesh device_listen wired connect success
			send_netlink
		# if agent connect with wlan1, change to wired
		elif [ "$type" == "wlan1" ]; then
			logger -t easymesh device_listen wlan1 change to wired connect
			uci set wireless.default_radio11.disabled='1'
			uci set wireless.default_radio22.disabled='1'
			uci set network.lan_dev.macaddr=$mac
			uci commit
			/etc/init.d/prplmesh restart
			agent_status
			logger -t easymesh device_listen wired connect success
			send_netlink
		# if agent connect with wired, don't do anything
		elif [ "$type" == "wired" ]; then
			logger -t easymesh device_listen wired connect now
		# if no role now ,change to controller or wired connect
		else
			agent_status
			sleep 15
			ssid_old=`uci get wireless.default_radio1.ssid`
			ssid_new=`cat /var/run/hostapd-phy1.conf | grep -w ssid | sed -n '1p' | awk -F '=' '{print$2}'`
			# change to controller
			if [ "$ssid_old" == "$ssid_new" ]; then
				logger -t easymesh device_listen change to controller
				ubus call web.wireless easymesh_set_role '{"role":"controller"}'
				agent_status
				#let the connect PC get the new ip
				tmp=`uci get network.@switch_vlan[0].ports | tr -d '5t'`
				for port in $tmp
				do
					echo rwPHYReg $port 0x00 0x1840 > sys/kernel/debug/esw_debug
					sleep 1
					echo rwPHYReg $port 0x00 0x1040 > sys/kernel/debug/esw_debug
				done
				/etc/init.d/device_listen restart
				/etc/init.d/dnsmasq restart
			else
				proto=`uci get network.lan.proto`
				if [ "$proto" == "static" ]; then
					logger -t easymesh device_listen chenge to wired connect
					uci set network.lan.proto='dhcp'
					uci del network.lan.ipaddr
					uci set dhcp.lan.ignore='1'
					uci set wireless.default_radio11.disabled='1'
					uci set wireless.default_radio22.disabled='1'
					uci commit
					/etc/init.d/prplmesh restart
					# let the wan port up down for the device listen just sleep 3 to scan new ip
					port=`uci get network.@switch_vlan[1].ports | tr -d '5t'`
					echo rwPHYReg $port 0x00 0x1840 > sys/kernel/debug/esw_debug
					sleep 1
					echo rwPHYReg $port 0x00 0x1040 > sys/kernel/debug/esw_debug
					return
				fi
				uci set wireless.default_radio22.ssid=`cat /var/run/hostapd-phy1.conf | grep -w ssid | sed -n '2p' | awk -F '=' '{print$2}'`
				uci set wireless.default_radio22.key=`cat /var/run/hostapd-phy1.conf | grep -w wpa_passphrase | sed -n '2p' | awk -F '=' '{print$2}'`
				uci commit
				/etc/init.d/dnsmasq restart
				logger -t easymesh device_listen wired connect success
			fi
		fi

	# if the wan is down, turn to wlan connect agent
	elif [ $1 == 0 ]; then
		wan_link
		if [ $? -eq 1 ]; then
			logger -t easymesh device_listen just network restart
			return
		fi
		ssid0=`uci get wireless.default_radio11.ssid`
		ssid1=`uci get wireless.default_radio22.ssid`
		if [ "$ssid0" != "easymesh" ]; then
			#wlan0_mac=`ifconfig wlan0 | grep HWaddr | awk -F ' ' '{print$5}'`
			#mac=$(macaddr_add "$wlan0_mac" +7)
			get_macaddr phy0
			logger -t easymesh device_listen change to wlan0 connect
			uci set wireless.default_radio11.disabled='0'
			uci set network.lan_dev.macaddr=$mac
			uci commit
			/etc/init.d/prplmesh restart
			agent_status
			logger -t easymesh device_listen wlan0 connect success
		elif [ "$ssid1" != "easymesh" ]; then
			logger -t easymesh device_listen change to wlan1 connect
			#wlan1_mac=`ifconfig wlan1 | grep HWaddr | awk -F ' ' '{print$5}'`
			#mac=$(macaddr_add "$wlan1_mac" -1)
			get_macaddr phy1
			uci set wireless.default_radio22.disabled='0'
			uci set network.lan_dev.macaddr=$mac
			uci commit
			/etc/init.d/prplmesh restart
			agent_status
			logger -t easymesh device_listen wlan1 connect success
		fi
	fi
fi
