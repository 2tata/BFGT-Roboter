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

FIRST=0
var=0
while [ $var -eq 0 ]
do
	if [ ! -e /proc/$PID0 -a ! -e /proc/$PID1 -a ! -e /proc/$PID2 -a ! -e /proc/$PID3 -o $FIRST -eq 0 ]; then
		FIRST=1
		(ping6 -c 1 $IPV6_ADRESS_W%$W_INTERFACE &> /dev/null) &
		PID0=$!
		(ping6 -c 1 $IPV6_ADRESS_W%$E_INTERFACE &> /dev/null) &
		PID1=$!
		(ping6 -c 1 $IPV6_ADRESS_E%$E_INTERFACE &> /dev/null) &
		PID2=$!
		(ping6 -c 1 $IPV6_ADRESS_E%$W_INTERFACE &> /dev/null) &
		PID3=$!
	fi;
	if [ ! -e /proc/$PID0 ]; then
		wait $PID0
		WLANW=$?
		if [ $WLANW -eq 0 ]; then
			LP_IPV6_ADRESS="$IPV6_ADRESS_W%$W_INTERFACE"
			var="1"
		fi;
	fi;
	if [ ! -e /proc/$PID1 ]; then
		wait $PID1
		WLANE=$?
		if [ $WLANE -eq 0 ]; then
			LP_IPV6_ADRESS="$IPV6_ADRESS_W%$E_INTERFACE"
			var="1"
		fi;
	fi;
	if [ ! -e /proc/$PID2 ]; then
		wait $PID2
		ETHE=$?
		if [ $ETHE -eq 0 ]; then
			LP_IPV6_ADRESS="$IPV6_ADRESS_E%$E_INTERFACE"
			var="1"
		fi;
	fi;
	if [ ! -e /proc/$PID3 ]; then
		wait $PID3
		ETHW=$?
		if [ $ETHW -eq 0 ]; then
			LP_IPV6_ADRESS="$IPV6_ADRESS_E%$W_INTERFACE"
			var="1"
		fi;
	fi;
done

COMMANDPID=0
while true
do
	stty -F $SERIAL_INTERFACE raw ispeed $SERIAL_BAUTRATE ospeed $SERIAL_BAUTRATE cs8 -ignpar -cstopb -echo
	if [ $COMMANDPID -eq 0 ]; then
		(
			COMMAND=$( nc -6lp $PORT )
			case $COMMAND in
				1 ) EXITCODE=1
				;;
				2 ) EXITCODE=2
				;;
				w ) EXITCODE=3
				;;
				a ) EXITCODE=4
				;;
				s ) EXITCODE=5
				;;
				d ) EXITCODE=6
				;;
				q ) EXITCODE=7
				;;
			esac
			exit $EXITCODE
		) &
		COMMANDPID=$!
	fi;

	read -i -s Line < $SERIAL_INTERFACE
	echo $Line

	if [ "$COMMAND" == "1" ]; then
		raspivid -w 640 -h 480 -vf -hf -rot 90 -t 999999 -fps 10 -b 4000000 -o - | nc -6 $LP_IPV6_ADRESS $LP_STREAM_PORT &
	fi;
	if [ ! -e /proc/$COMMANDPID -a $COMMANDPID -ne 0 ]; then
		wait $COMMANDPID
		COMMAND=$?
		COMMANDPID=0
		echo $COMMAND
		if [ "$COMMAND" == "2" ]; then
			ping -c 1 8.8.8.8 &> /dev/null
			if [ $? -eq 0 ];  then
				reboot=0
				apt-get update
				reboot=$(( reboot + `apt-get upgrade -y | grep '^Inst' | wc -l` ))
				reboot=$(( reboot + `apt-get dist-upgrade -y | grep '^Inst' | wc -l` ))
				apt-get autoremove --purge -y
				apt-get autoclean
				reboot=$(( reboot + `rpi-update | grep 'reboot' | wc -l` ))
				if [ $reboot -ne 0 ]; then
					reboot
				fi;
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
	fi;
done
