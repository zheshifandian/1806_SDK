#!/bin/sh
. /lib/functions.sh
. /usr/share/libubox/jshn.sh

cookie_path=/tmp/andlink_cookie

get_stok() {
    auth=$(cat /etc/shadow | sed -n '6p' | awk -F ':' '{print $2}')
    if [ "x$auth" = "x" ];then
        curl -D $1 "http://127.0.0.1/cgi-bin/luci/api/sfsystem/get_stok_local" -d '{"version":"V10","luci_password":"admin"}'
    else
        curl -D $1 -H "Authorization: $auth" "http://127.0.0.1/cgi-bin/luci/api/sfsystem/get_stok_local" -d '{"version":"V10","luci_password":"admin"}'
    fi  
    result=`awk -F: ' /sysauth/ {split($2,myarry,"=")} END {print myarry[2]}' $1 | awk -F ";" '{print $1}'`
    if [ -n "$result" ]; then
        echo "$result"
	return 1
    else
        return 0
    fi  
}

get_mac() {
	tmp1=$(cat /sys/devices/platform/factory-read/macaddr | sed -n "4p" | sed "s/0x//g")
	tmp2=$(cat /sys/devices/platform/factory-read/macaddr | sed -n "5p" | sed "s/0x//g")
	echo ${tmp1}${tmp2}
}


soft_ap_mode() {
	##first should check wds is enabled
	#sta=$(uci get network.)
	interface=$(uci get network.stabridge)
	if [ -n "$interface" ]; then
		##wds has enabled, disable wds
		url="http://127.0.0.1/cgi-bin/luci/;stok="$1"/api/sfsystem/wds_disable"
		result=$(curl -b $2 -H "Content-type:application/json" -X POST $url -d '{"version" : "V10"}')
		json_load "$result"
		json_get_var code "code"
		if [ $code -ne 0 ]; then
			echo "disable wds failed"
			return 1
		fi
	fi
	mac=$(get_mac)
	ssid="CMQLINK-${device_type}-${mac}"
	uci set wireless.@wifi-iface[0].ssid="$ssid"
	uci set wireless.@wifi-iface[0].encryption="none"
	uci set wireless.@wifi-iface[1].ssid="$ssid"
	uci set wireless.@wifi-iface[1].encryption="none"
	uci set network.lan.ipaddr="192.168.188.1"
	uci commit network
	uci commit wireless
	/etc/init.d/network restart
}


set_sfi0() {
	config_get ifname "$1" ifname
	if [ "$ifname" == "sfi0" ]; then
		config_set "$1" ssid $2
		config_set "$1" encryption $3
		config_set "$1" key $4
	fi
}

wds_connect(){
	wif=`cat /etc/config/wireless | grep sfi0`
	 [ -n "$wif" ] && {
		uci set wireless.@wifi-iface[2].disabled='0'
	}

	if [ ! -n "$wif" ]; then
	cat >> /etc/config/wireless << EOF
config wifi-iface "tmp"
	option device   radio0
	option ifname   sfi0
	option network  wwan
	option mode     sta
	option ssid     $1
	option encryption $2
	option key      $3
EOF
	else
		tmp=$(uci show wireless | grep "sfi0")
		pre=${tmp%.*}
		uci set "${pre}.ssid=$1"
		uci set "${pre}.encryption=$2"
		uci set "${pre}.key=$3"
	fi

	uci set network.wwan=interface
	uci set network.wwan.ifname=sfi0
	uci set network.wwan.proto=dhcp
	uci commit network
	uci commit wireless
	wifi reload
}

quick_link_mode() {
	local band=0
	wds_connect CMCC-QLINK none 12345678
	channel=$(iwinfo wlan0 info | grep "Channel" |sed "s/^.*Channel: \([0-9]\+\).*/\1/")
	##delete wireless connect info
	for tmp in $(uci show wireless | grep "sfi0" | sort -r)
	do
		iface=${tmp%.*}
		uci delete $iface
	done
	uci commit wireless
	url="http://127.0.0.1/cgi-bin/luci/api/sfsystem/wifi_connect"
	request_data='{"version" : "V10", "channel" :'$channel', "encryption" :"none", "key" : "12345678", "ssid" : "CMCC-QLINK"}'
	echo $request_data
	result=$(curl -b $2 -H "Content-type:application/json" -X POST $url -d '{"version" : "V10", "channel" :'6',"encryption" :"ccmp+psk2","key" : "12345678", "ssid" : "CMCC-QLINK"}')
	json_load "$result"
	json_get_var code "code"
	if [ $code -ne 0 ]; then
		echo "wifi connect failed\n"
		#TODO if failed, what should we do.
		return 1
	fi
	json_get_var band "band"
	url="http://127.0.0.1/cgi-bin/luci/api/sfsystem/wds_enable"
	request_data='{"version" : "V10", "ifaces":{"band": "'$band'", "ssid" : "CMCC-QLINK-WDS", "key" : "12345678"} }'
	echo $request_data
	##call wds enable
	result=$(curl -b $2 -H "Content-type:application/json" -X POST $url -d '{"version" : "V10", "ifaces":{"band": "2.4G", "ssid" : "CMCC-QLINK-WDS", "key" : "12345678"} }')
	echo $result
	json_load "$result"
	json_get_var code "code"
	if [ $code -ne 0 ]; then
		echo "enable wds failed"
		return 1
	fi
	url="http://127.0.0.1/cgi-bin/luci/api/sfsystem/wds_getwanip"
	request_data='{"version" : "V10", "band": "'$band'"} }'
	local retries=10
	while [ $retries -gt 0 ]
	do
		sleep 1
		let retries=retries-1
		request_data='{"version" : "V10", "band": "'$band'"}'
		result=$(curl -b $2 -H "Content-type:application/json" -X POST $url -d '{"version" : "V10", "band": "2.4G"} }')
		json_load "$result"
		json_get_var code "code"
		if [ $code -eq 0 ]; then
			echo "wds connect finished"
			return 0
		fi
	done
	return 1
}

station_mode() {
	local band=0
	ssid=$(uci get andlink.andlink.SSID)
	key=$(uci get andlink.andlink.password)
	encrypt=$(uci get andlink.andlink.encrypt)
	channel=$(uci get andlink.andlink.channel)
	[ -z "$ssid" ] && [ -z "$key" ] && [ -z "$encrypt" ] && [ -z "$channel" ] && {
		echo "missing some important info"
		return 1
	}

	interface=$(uci get network.stabridge)
	if [ -n "$interface" ]; then
		##wds has enabled, disable wds
		url="http://127.0.0.1/cgi-bin/luci/api/sfsystem/wds_disable"
		result=$(curl -b $2 -H "Content-type:application/json" -X POST $url -d '{"version" : "V10"}')
		json_load "$result"
		json_get_var code "code"
		if [ $code -ne 0 ]; then
			echo "disable wds failed"
			return 1
		fi
	fi

	#if channel is equal to 0, this means auto select the channel
	if [ $channel -eq 0 ]; then
		wds_connect $ssid $encrypt $key
		channel=$(iwinfo wlan0 info | grep "Channel" |sed "s/^.*Channel: \([0-9]\+\).*/\1/")
		##delete wireless connect info
		for tmp in $(uci show wireless | grep "sfi0" | sort -r)
		do
			iface=${tmp%.*}
			uci delete $iface
		done
		uci commit wireless
	fi
	
	url="http://127.0.0.1/cgi-bin/luci/api/sfsystem/wifi_connect"
	request_data='{"version" : "V10", "channel" :'$channel', "encryption" :"'$encrypt'", "key" : "'$key'", "ssid" : "'$ssid'"}'
	echo $request_data
	result=$(curl -b $2 -H "Content-type:application/json" -X POST $url -d '{"version" : "V10", "channel" :'$channel', "encryption" :"'$encrypt'", "key" : "'$key'", "ssid" : "'$ssid'"}')
	json_load "$result"
	json_get_var code "code"
	if [ $code -ne 0 ]; then
		echo "wifi connect failed\n"
		#TODO if failed, what should we do.
		return 1
	fi
	json_get_var band "band"
	url="http://127.0.0.1/cgi-bin/luci/api/sfsystem/wds_enable"
	request_data='{"version" : "V10", "ifaces":{"band": "'$band'", "ssid" : "'$ssid'", "key" : "'$encrypt'"} }'
	echo $request_data
	##call wds enable
	result=$(curl -b $2 -H "Content-type:application/json" -X POST $url -d '{"version" : "V10", "ifaces":{"band": "'$band'", "ssid" : "'$ssid'", "key" : "'$encrypt'"} }')
	json_load "$result"
	json_get_var code "code"
	if [ $code -ne 0 ]; then
		echo "enable wds failed"
		return 1
	fi
	url="http://127.0.0.1/cgi-bin/luci/api/sfsystem/wds_getwanip"
	request_data='{"version" : "V10", "band": "'$band'"} }'
	local retries=10
	while [ $retries -gt 0 ]
	do
		sleep 1
		let retries=retries-1
		request_data='{"version" : "V10", "band": "'$band'"}'
		result=$(curl -b $2 -H "Content-type:application/json" -X POST $url -d '{"version" : "V10", "band": "'$band'"} }')
		json_load "$result"
		json_get_var code "code"
		if [ $code -eq 0 ]; then
			echo "wds connect finished"
			return 0
		fi
	done
	return 1
}

cookie=`get_stok $cookie_path | tr -d "\r"`
if [ -z $cookie ]; then
    echo "get stok fail :$cookie"
fi

device_type="123456"
case "$1" in
	*SOFT_AP*)
	soft_ap_mode $cookie $cookie_path
	return $?
	;;
	*QUICK_LINK*)
	quick_link_mode $cookie $cookie_path
	return $?
	;;
	*STATION*)
	station_mode $cookie $cookie_path
	return $?
	;;
	*)
	echo "$1 do nothing"
	;;
esac
