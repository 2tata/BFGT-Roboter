#!/bin/bash
clear

CONFIGPART="part to config"

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
                                        RPI_WLAN_IPV6 ) RPI_WLAN_IPV6=${arr[1]}
                                        ;;
                                        RPI_ETH_IPV6 ) RPI_ETH_IPV6=${arr[1]}
                                        ;;
                                        ETH_INTERFACE ) E_INTERFACE=${arr[1]}
                                        ;;
                                        WLAN_INTERFACE ) W_INTERFACE=${arr[1]}
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

echo "Ping test..."
(ping6 -c 1 $RPI_WLAN_IPV6%$W_INTERFACE &> /dev/null) &
PID0=$!
(ping6 -c 1 $RPI_WLAN_IPV6%$E_INTERFACE &> /dev/null) &
PID1=$!
(ping6 -c 1 $RPI_ETH_IPV6%$E_INTERFACE &> /dev/null) &
PID2=$!
(ping6 -c 1 $RPI_ETH_IPV6%$W_INTERFACE &> /dev/null) &
PID3=$!

EXIT=0
var=0
while [ $var -eq 0 ]
do
	if [ $EXIT -ge 4 ]; then
		echo "Roboter nicht errechtbar!"
		exit
	fi;
	if [ ! -e /proc/$PID0 ]; then
		wait $PID0
		WLANW=$?
		if [ $WLANW -eq 0 ]; then
			INTERFACE="$RPI_WLAN_IPV6%$W_INTERFACE"
			echo "WLAN <--> WLAN"
			var="1"
		else
			EXIT=$(( $EXIT + 1 ))
		fi;
	fi;
	if [ ! -e /proc/$PID1 ]; then
		wait $PID1
		WLANE=$?
		if [ $WLANE -eq 0 ]; then
			INTERFACE="$RPI_WLAN_IPV6%$E_INTERFACE"
			echo "WLAN <--> ETH"
			var="1"
		else
			EXIT=$(( $EXIT + 1 ))
		fi;
	fi;
	if [ ! -e /proc/$PID2 ]; then
		wait $PID2
		ETHE=$?
		if [ $ETHE -eq 0 ]; then
			INTERFACE="$RPI_ETH_IPV6%$E_INTERFACE"
			echo "ETH <--> ETH"
			var="1"
		else
			EXIT=$(( $EXIT + 1 ))
		fi;
	fi;
	if [ ! -e /proc/$PID3 ]; then
		wait $PID3
		ETHW=$?
		if [ $ETHW -eq 0 ]; then
			INTERFACE="$RPI_ETH_IPV6%$W_INTERFACE"
			echo "ETH <--> WLAN"
			var="1"
		else
			EXIT=$(( $EXIT + 1 ))
		fi;
	fi;
sleep 0.1
done

echo "Befel eingeben:"
while true
do
	read -n 1 -s COMMAND
	echo $COMMAND

	if [ "$COMMAND" == "1"  ]; then
		nc -6lp $LP_STREAM_PORT | mplayer -fps 15 -cache 512 - &
		echo "1" | nc -6 $INTERFACE $PORT
	fi;
	if [ "$COMMAND" == "2" -o "$COMMAND" == "w" -o "$COMMAND" == "s" -o "$COMMAND" == "a" -o "$COMMAND" == "d" -o "$COMMAND" == "q" ]; then
		echo $COMMAND | nc -6 $INTERFACE $PORT
	fi;
done
