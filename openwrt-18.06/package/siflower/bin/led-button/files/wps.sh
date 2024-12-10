#!/bin/sh
. /usr/share/led-button/wps_func.sh

status=0
wps_status=0

#for key diagnose webpage
echo pressed > /tmp/key

#flags
#@wps_start means enter wds.sh
#@wps_status means wpa_ci_event receive the event WPS-SUCCESS
check_status() {
	[ -f /tmp/wps_start ] && status=`cat /tmp/wps_start`
	[ -f /tmp/wps_status  ] && wps_status=`cat /tmp/wps_status`
	[ $status = 1 -o "$wps_status" != 0 ] && {
		exit 0
	}
}

#repeater network use different config.
local_network="wwan"
mode=`uci -q get basic_setting.dev_mode.mode`
#@mode means wds(rep), ap(wps). default: ap.
case $mode in
	ap)
		directory="hostapd"
		mode="hostapd_cli"
		;;
	wds|rep)
		directory="wpa_supplicant"
		if [ "$mode" = "rep" ]; then
			local_network="wwwan"
		else
			# if wds has config of network wwan, it will
			# cause error when sfi0 sfi1 are added.
			#FIXME shall we delete station here?
			uci_delete_network
			uci commit
			output=`wifi reload`
		fi
		mode="wpa_cli"
		check_status
		echo 1 > /tmp/wps_start
		uci_add_station "sfi0" "wwan"
		uci_add_station "sfi1" "$local_network"
		uci commit
		output=`wifi reload`

		sleep 2
		;;
	*)
		directory="hostapd"
		mode="hostapd_cli"
		;;
esac

wps_led=$(uci get basic_setting.led.wps_led)
led_enable=$(uci get basic_setting.led.enable)
wps_option=$(uci get basic_setting.led.wps_option)
default_led=$(uci get basic_setting.led.default_led)

wps_time=$(uci get basic_setting.vendor.wps_time)
if [ -z "$wps_time" ]; then
	wps_time=10
fi
flash_times=$((wps_time / 2))

cd /var/run/$directory
for socket in *; do
	[ -S "$socket" ] || continue
	"$mode" -i "$socket" wps_pbc

	if [ -n "$wps_led" ] && [ "$led_enable" -eq 1 ]; then
		if [ "$wps_option" = "on" ]; then
			echo 0 > /sys/class/leds/$default_led/brightness
			echo 1 > /sys/class/leds/$wps_led/brightness
		fi
	fi
done

if [ -n "$wps_led" ] && [ "$led_enable" -eq 1 ]; then
	if [ "$wps_option" = "flash" ]; then
		echo 0 > /sys/class/leds/$default_led/brightness
		for i in `seq 1 $flash_times`; do
			echo 1 > /sys/class/leds/$wps_led/brightness
			sleep 1
			echo 0 > /sys/class/leds/$wps_led/brightness
			sleep 1
		done
		echo 1 > /sys/class/leds/$default_led/brightness
		sh /sbin/wan_detect.sh 2
	else
		sleep $wps_time
	fi
else
	sleep $wps_time
fi

for socket in *; do
	[ -S "$socket" ] || continue
	"$mode" -i "$socket" wps_cancel
	if [ -n "$wps_led" ] && [ "$led_enable" -eq 1 ]; then
		if [ "$wps_option" = "on" ]; then
			echo 0 > /sys/class/leds/$wps_led/brightness
			echo 1 > /sys/class/leds/$default_led/brightness
			sh /sbin/wan_detect.sh 2
		fi
	fi
done

#rm wps button event flag.
[ -f /tmp/wps_start ] && rm /tmp/wps_start
exit 0
