#!/bin/sh

set -e

# IP for device upgrades, operational tests, Boardfarm data network, ...
uci set network.lan.ipaddr='192.168.1.110'

uci set wireless.radio0.disabled=0
uci set wireless.radio1.disabled=0

uci commit
/etc/init.d/network restart
