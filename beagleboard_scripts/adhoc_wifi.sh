#!/bin/sh

echo Configuring adhoc wifi
ifconfig wlan0 down
iwconfig wlan0 mode ad-hoc essid beagleboard-xm channel 1 key s:13char_passwd
ifconfig -a wlan0 10.10.10.10 netmask 255.255.255.0
ifconfig wlan0 up
