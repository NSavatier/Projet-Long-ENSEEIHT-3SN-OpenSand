#!/bin/bash

#send modify bandwidth
#$1 = type
#$2 = gw id
#$3 = spot type
#$4 = valeur

#check params number
if [ $# != 4 ]; then
	echo "syntaxe : ./modify_bandwidth type gw_id spot-id newValue"
        echo "exemple : ./modify_bandwidth forward 0 1 500"
        echo "for forward link, gw0, spot1 and 500Mhz as new value" 
	exit 1
fi



if [ $1 == "forward" ] ; then
	type=0
else 
	if [ $1 == "return" ]; then
		type=1
	else
		echo "type must be forward or return"
		exit 1
	fi
fi


message="$3:$2:$type:$4"


simulation_id=0 #TODO change it, use parameter or read from file

ip_sat="192.168.1${simulation_id}.10"
ip_gw="192.168.1${simulation_id}.20"
ip_st="192.168.1$simulation_id.40"

echo "sending $message to $ip_sat $ip_gw, and $ip_st"
echo $message | nc $ip_sat 5335 &
echo $message | nc $ip_gw 5335 &
echo $message | nc $ip_st 5335 &

