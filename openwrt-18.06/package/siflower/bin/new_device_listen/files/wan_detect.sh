#!/bin/sh

updown=$1
led_enable=$2
if [ -z "$led_enable" ]; then
    led_enable=$(uci get basic_setting.led.enable)
fi
internet_led=$(uci get basic_setting.led.internet_led)
default_led=$(uci get basic_setting.led.default_led)

if [ $updown -eq 1 ]; then
    echo "Detect wan up"
    start_time=$(date +%s)
    end_time=$((start_time + 300))

    while [ $(date +%s) -lt $end_time ]; do
        result=$(ubus call network.interface.wan status)
        up=$(echo "$result" | grep '"up"' | awk -F ' ' '{print $2}' | awk -F ',' '{print $1}')
        address=$(echo "$result" | grep '"address": ' | awk -F '"' '{print $4}')
        if [ "$up" = "true" ] && [ -n "$address" ]; then
            if [ -n "$internet_led" ] && [ "$led_enable" -eq 1 ]; then
                echo 0 > /sys/class/leds/$default_led/brightness
                echo 1 > /sys/class/leds/$internet_led/brightness
                uci set basic_setting.led.wan_status=1
                uci commit
            fi
            break
        elif [ "$up" = "false" ] || [ -z "$address" ]; then
            if [ -n "$internet_led" ] && [ "$led_enable" -eq 1 ]; then
                echo 0 > /sys/class/leds/$internet_led/brightness
                echo 1 > /sys/class/leds/$default_led/brightness
                uci set basic_setting.led.wan_status=0
                uci commit
            fi
        fi
        sleep 1
    done
elif [ $updown -eq 0 ]; then
    echo "Detect wan down"
    if [ -n "$internet_led" ] && [ "$led_enable" -eq 1 ]; then
        echo 0 > /sys/class/leds/$internet_led/brightness
        echo 1 > /sys/class/leds/$default_led/brightness
        uci set basic_setting.led.wan_status=0
        uci commit
    fi
elif [ $updown -eq 2 ]; then
    echo "Update wan status"
    wan_status=$(uci get basic_setting.led.wan_status)
    if [ -n "$internet_led" ] && [ "$led_enable" -eq 1 ]; then
        if [ $wan_status -eq 1 ]; then
            echo 0 > /sys/class/leds/$default_led/brightness
            echo 1 > /sys/class/leds/$internet_led/brightness
        else
            echo 0 > /sys/class/leds/$internet_led/brightness
            echo 1 > /sys/class/leds/$default_led/brightness
        fi
    fi
else
    echo "Error updown"
fi