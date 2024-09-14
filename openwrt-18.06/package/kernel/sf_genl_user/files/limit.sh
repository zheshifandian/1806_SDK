#!/bin/sh

port=$1
speed_limit=$2

if [ -z $1 ]; then
	echo "input error! please suffix [port] to get port limit_rate"
	echo "input error! please suffix [port] [limit_rate] to set port limit_rate"
	exit
fi

if [ -z $2 ]; then
	ubus call tmu.port.shaper get '{"port":'$port', "shaper":0 }'
else
	ubus call tmu.port.shaper set '{"port":'$port', "shaper":0, "credit_min":261888, "credit_max":1024, "enabled":true }'
	ubus call tmu.port.shaper rate_limit '{"port":'$port', "shaper":0, "Mbps":'$speed_limit', "clk_div":5 }'
fi
