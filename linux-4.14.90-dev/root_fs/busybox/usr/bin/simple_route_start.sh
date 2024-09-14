#! /bin/sh
ifconfig eth0 192.168.2.1
ifconfig eth1 192.168.12.55
ifconfig eth0 up
ifconfig eth1 up
echo 1 > /proc/sys/net/ipv4/ip_forward
iptables -t nat -A POSTROUTING -s 192.168.2.0/24 -o eth1 -j SNAT --to-source 192.168.12.55
route add default gw 192.168.12.1
