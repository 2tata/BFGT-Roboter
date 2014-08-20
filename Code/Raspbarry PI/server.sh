#!/bin/bash
#-----------------------
# Raspbarry PI Server zur Steuerung des Roboters
# Autor: Jan-Tarek Butt
# Datum: 12.03.2014
#-----------------------
clear
CONFIGPART="Part to config"

# Read Config file
if [ -f $CONFIGPART ]; then
	while read line;
	do
		if [ -n "$line" ]; then
			if [[ ${line:0:1} != '#' ]]; then
				IFS=' = ' read -a arr <<< "$line"
				case ${arr[0]} in
					PORT ) PORT=${arr[1]}
					;;
					SERIAL_BAUTRATE ) SERIAL_BAUTRATE=${arr[1]}
					;;
					SERIAL_INTERFACE ) SERIAL_INTERFACE=${arr[1]}
					;;
					LAPTOP_IPV6_ADRESS_WIRELESS ) IPV6_ADRESS_W=${arr[1]}
					;;
					LAPTOP_IPV6_ADRESS_ETHERNET ) IPV6_ADRESS_E=${arr[1]}
					;;
					ETHERNET_INTERFACE ) E_INTERFACE=${arr[1]}
					;;
					WIRELESS_INTERFACE ) W_INTERFACE=${arr[1]}
					;;
					LAPTOP_PORT ) LP_PORT=${arr[1]}
					;;
					LAPTOP_STREAM_PORT ) LP_STREAM_PORT=${arr[1]}
					;;
					* ) echo `date` "| fehler in der config datei" >> ~/log.txt
						exit
					;;
				esac
			fi;
		fi;
	done < $CONFIGPART
else
	echo `date` "| config datei nicht vorhanden" >> ~/log.txt
	exit
fi

#Check for first running
if [ -f /tmp/server.lock ]; then
	echo "server ist bereits aktiv"
	echo `date` "| server ist bereits aktiv" >> ~/log.txt
	exit
else
	touch /tmp/server.lock
fi

var="0"
while [ $var -eq 0 ]
do
	ping6 -c 1 $LP_ADRESSE_W%$W_INTERFACE &> /dev/null
	WLANW=$?
	ping6 -c 1 $LP_ADRESSE_W%$E_INTERFACE &> /dev/null
	WLANE=$?
	ping6 -c 1 $LP_ADRESSE_E%$E_INTERFACE &> /dev/null
	ETHE=$?
	ping6 -c 1 $LP_ADRESSE_E%$W_INTERFACE &> /dev/null
	ETHW=$?
	if [ $WLANW -eq 0 ]; then
		LP_IPV6_ADRESS="$LP_ADRESSE_W%$W_INTERFACE"
		var="1"
	fi;
	if [ $WLANE -eq 0 ]; then
		LP_IPV6_ADRESS="$LP_ADRESSE_W%$E_INTERFACE"
		var="1"
	fi;
	if [ $ETHE -eq 0 ]; then
		LP_IPV6_ADRESS="$LP_ADRESSE_E%$E_INTERFACE"
		var="1"
	fi;
	if [ $ETHW -eq 0 ]; then
		LP_IPV6_ADRESS="$LP_ADRESSE_E%$W_INTERFACE"
		var="1"
	fi;
done

while true
do
	stty -F $SERIAL_INTERFACE raw ispeed $SERIAL_BAUTRATE ospeed $SERIAL_BAUTRATE cs8 -ignpar -cstopb -echo
	COMMAND=$( nc -6lp $PORT )
	echo $COMMAND

	read -i -s Line < $SERIAL_INTERFACE
	echo $Line

	if [ "$COMMAND" == "1" ]; then
		raspivid -w 640 -h 480 -t 999999 -fps 10 -b 4000000 -o - | nc -6 $LP_IPV6_ADRESS $LP_STREAM_PORT &
	fi;
	
	if [ "$COMMAND" == "2" ]; then
		ping -c 1 8.8.8.8 &> /dev/null
		if [ $? -eq 0 ];  then
			apt-get update &> /dev/null
			apt-get --assume-yes upgrade &> /dev/null
			apt-get --assume-yes dist-upgrade &> /dev/null
			apt-get --assume-yes autoremove --purge &> /dev/null
			apt-get autoclean &> /dev/null
			rpi-update &> /dev/null
		fi;
	fi;

	if [ "$COMMAND" == "s" ]; then
		echo -n "1" >$SERIAL_INTERFACE
		echo -n "2" >$SERIAL_INTERFACE
		echo -n "3" >$SERIAL_INTERFACE
		echo -n "255" >$SERIAL_INTERFACE
		echo -n "255" >$SERIAL_INTERFACE
		echo -n "1" >$SERIAL_INTERFACE
	fi;
	if [ "$COMMAND" == "w" ]; then
		echo -n "1" >$SERIAL_INTERFACE
		echo -n "1" >$SERIAL_INTERFACE
		echo -n "4" >$SERIAL_INTERFACE
		echo -n "255" >$SERIAL_INTERFACE
		echo -n "255" >$SERIAL_INTERFACE
		echo -n "1" >$SERIAL_INTERFACE
	fi;
	if [ "$COMMAND" == "d" ]; then
		echo -n "1" >$SERIAL_INTERFACE
		echo -n "2" >$SERIAL_INTERFACE
		echo -n "3" >$SERIAL_INTERFACE
		echo -n "255" >$SERIAL_INTERFACE
		echo -n "000" >$SERIAL_INTERFACE
		echo -n "1" >$SERIAL_INTERFACE
	fi;
	if [ "$COMMAND" == "a" ]; then
		echo -n "1" >$SERIAL_INTERFACE
		echo -n "2" >$SERIAL_INTERFACE
		echo -n "3" >$SERIAL_INTERFACE
		echo -n "000" >$SERIAL_INTERFACE
		echo -n "255" >$SERIAL_INTERFACE
		echo -n "1" >$SERIAL_INTERFACE
	fi;
	if [ "$COMMAND" == "q"  ]; then
	        echo -n "1" >$SERIAL_INTERFACE
		echo -n "5" >$SERIAL_INTERFACE
		echo -n "5" >$SERIAL_INTERFACE
	fi;
done
