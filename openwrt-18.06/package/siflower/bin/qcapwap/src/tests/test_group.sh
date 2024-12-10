#!/bin/sh

wait_status() {
	local dev=$1
	local status=`uci get capwap_devices.${dev}.status`
	local i=0
	set +x
	while [ "$status" != "1" ] && [ $i -lt 30 ]; do
		sleep 1
		i=$(($i+1))
		status=`uci get capwap_devices.${dev}.status`
	done
	[ $i -ge 30 ] && exit 3
	set -x
}

while [ 1 ]; do
	devices=`uci show capwap_devices | grep "status='1'" | awk -F '.' '{print $2}'`
	groups=`uci show ap_groups | grep "=group" | awk -F '[=.]' '{print $2}'`
	for group in $groups; do
		for dev in $devices; do
			WUM -j "{\"command\":\"set_device_to_group\",\"device\":\"${dev}\",\"name_of_group\":\"${group}\"}"
			[ $? -ne 0 ] && exit 1
		done
		for dev in $devices; do
			wait_status $dev
			g=`uci get capwap_devices.${dev}.group`
			[ "$g" != "$group" ] && exit 2
		done
	done

	for group in $groups; do
		WUM -j "{\"command\":\"ap_group_change\",\"name_of_group\":\"${group}\"}"
		[ $? -ne 0 ] && exit 1
		for dev in $devices; do
			wait_status $dev
		done
	done
done