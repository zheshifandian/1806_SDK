#!/bin/sh

all_devices() {
	uci show capwap_devices | grep "status" | awk -F "." '{print $2}'
}

online() {
	uci show capwap_devices | grep "status='1'" | awk -F "." '{print $2}'
}

offline() {
	uci show capwap_devices | grep "status" | grep -v "status='1'" | awk -F "." '{print $2}'
}

delete() {
	for dev in $@; do
		WUM -j "{\"command\":\"delete_device\", \"device\":\"${dev}\"}"
	done
}

set_device_to_group() {
	local group=$1
	shift
	for dev in $@; do
		WUM -j "{\"command\":\"set_device_to_group\", \"device\":\"${dev}\", \"name_of_group\":\"${group}\"}"
	done
}

reset_all() {
	set_device_to_group "default" $(all_devices)
}

led() {
	local led=$1
	shift
	local devices=$@
	[ "$devices" == "all" ] && devices=$(online)
	if [ "$led" == "off" ]; then
		led=0
	else
		led=1
	fi
	for dev in $devices; do
		WUM -j "{\"command\":\"set_device_config\", \"device\":\"${dev}\", \"dev_config\":{\"led\":$led}}"
	done
}

update() {
	local specific_version=$1
	local md5=$(md5sum $2 | awk '{print $1}')
	local version updates
	for dev in $(online); do
		version=$(uci get capwap_devices.$dev.firmware_version)
		if [ "$version" == "$specific_version" ]; then
			updates="$dev "$updates
		fi
	done
	wtp_update 0 $md5 $updates
}

update_all() {
	local md5=$(md5sum $1 | awk '{print $1}')
	local version updates
	for dev in $(online); do
		local update_status=$(uci get capwap_devices.$dev.updating)
		if [ $update_status -ne 1 ] && [ $update_status -ne 2 ]; then
			updates="$dev "$updates
		fi
	done
	wtp_update 0 $md5 $updates
}

force_update() {
	local path=$1
	shift
	local devices=$@
	if [ "$devices" == "all" ]; then
		devices=$(online)
	fi
	for dev in $devices; do
		scp $path admin@$(get_ip $dev):/tmp/firmware.img
		WUM -j "{\"command\":\"do_wtp_command\", \"device\":\"$dev\", \"wtp_command\":\"/sbin/sysupgrade /tmp/firmware.img\"}" &
	done
}

check_version() {
	local cmp=$1
	local version=$2
	local dev_version

	for dev in $(online); do
		dev_version=`uci get capwap_devices.$dev.firmware_version`
		if $(eval [ "$dev_version" $cmp "$version" ]); then
			uci show capwap_devices.$dev
		fi
	done
}

get_ip() {
	local mac=$1
	cat /tmp/dhcp.leases | grep ${mac//_/:} | awk '{print $3}'
}

get_dev() {
	local ip=$1
	local dev=$(cat /tmp/dhcp.leases | grep "$ip" | awk '{print $2}')
	echo ${dev//:/_}
}

wtp_cmd() {
	local cmd=$1
	shift
	local devices=$@
	if [ "$devices" == "all" ]; then
		devices=$(online)
	fi
	for dev in $devices; do
		WUM -j "{\"command\":\"do_wtp_command\", \"device\":\"$dev\", \"wtp_command\":\"$cmd\"}"
	done
}

log() {
	local ip=$1
	wtp_cmd "logread | grep capwap" $(get_dev $ip)
}

show_group() {
	local groups=$(uci show ap_groups | grep =group | awk -F "[.=]" '{print $2}')
	local total_on=0
	local total_off=0
	for group in $groups; do
		eval ${group}_on=0
		eval ${group}_off=0
	done
	for dev in $(online); do
		local group=$(uci get capwap_devices.$dev.group)
		eval ${group}_on=$((${group}_on+1))
	done
	for dev in $(offline); do
		local group=$(uci get capwap_devices.$dev.group)
		eval ${group}_off=$((${group}_off+1))
	done
	for group in $groups; do
		eval "printf \"$group\\\\t\$${group}_on / \$${group}_off\\\\n\""
		eval total_on=$(($total_on+${group}_on))
		eval total_off=$(($total_off+${group}_off))
	done
	printf "total\t$total_on / $total_off  $(($total_on+$total_off))\n"
}

case $1 in
"reset")
	shift
	set_device_to_group "default" $@
	;;
*)
	local cmd=$1
	shift
	eval $cmd $@
esac
