#!/bin/sh
led_enable=$(uci get basic_setting.led.enable)
reboot_led=$(uci get basic_setting.led.reboot_led)
reboot_option=$(uci get basic_setting.led.reboot_option)
default_led=$(uci get basic_setting.led.default_led)

if [ -n "$reboot_led" ] && [ "$led_enable" -eq 1 ]; then
    if [ "$reboot_option" = "on" ]; then
        echo 1 > /sys/class/leds/$reboot_led/brightness
    elif [ "$reboot_option" = "flash" ]; then
        echo 0 > /sys/class/leds/$default_led/brightness
        for i in `seq 1 5`; do
            echo 1 > /sys/class/leds/$reboot_led/brightness
            sleep 1
            echo 0 > /sys/class/leds/$reboot_led/brightness
            sleep 1
        done
    fi
fi

/sbin/reboot