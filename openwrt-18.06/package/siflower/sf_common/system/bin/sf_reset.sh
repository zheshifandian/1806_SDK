#!/bin/sh
led_enable=$(uci get basic_setting.led.enable)
reset_led=$(uci get basic_setting.led.reset_led)
reset_option=$(uci get basic_setting.led.reset_option)
default_led=$(uci get basic_setting.led.default_led)

if [ -n "$reset_led" ] && [ "$led_enable" -eq 1 ]; then
    if [ "$reset_option" = "on" ]; then
        echo 1 > /sys/class/leds/$reset_led/brightness
    elif [ "$reset_option" = "flash" ]; then
        echo 0 > /sys/class/leds/$default_led/brightness
        for i in `seq 1 5`; do
            echo 1 > /sys/class/leds/$reset_led/brightness
            sleep 1
            echo 0 > /sys/class/leds/$reset_led/brightness
            sleep 1
        done
    fi
fi

/bin/led-button -l 33 &
/sbin/jffs2reset -y && /sbin/reboot
