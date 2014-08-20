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

STATUS=0

echo "Ping test..."
ping6 -c 1 $RPI_WLAN_IPV6%$W_INTERFACE &> /dev/null
WLANW=$?
ping6 -c 1 $RPI_WLAN_IPV6%$E_INTERFACE &> /dev/null
WLANE=$?
ping6 -c 1 $RPI_ETH_IPV6%$E_INTERFACE &> /dev/null
ETHE=$?
ping6 -c 1 $RPI_ETH_IPV6%$W_INTERFACE &> /dev/null
ETHW=$?

if [ $WLANW -ne 0 -a $WLANE -ne 0 -a $ETHE -ne 0 -a $ETHW -ne 0 ]; then
        echo "Roboter nicht errechtbar!"
	exit
fi;

if [ $WLANW -eq 0 ]; then
	INTERFACE="$RPI_WLAN_IPV6%$W_INTERFACE"
	echo "Wlan verbindung"
fi;

if [ $WLANE -eq 0 ]; then
        INTERFACE="$RPI_WLAN_IPV6%$E_INTERFACE"
        echo "Wlan verbindung"
fi;

if [ $ETHE -eq 0 ]; then
        INTERFACE="$RPI_ETH_IPV6%$E_INTERFACE"
	echo "Lan verbindung"
fi;

if [ $ETHW -eq 0 ]; then
        INTERFACE="$RPI_ETH_IPV6%$W_INTERFACE"
        echo "Lan verbindung"
fi;

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
