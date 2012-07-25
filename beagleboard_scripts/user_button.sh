#!/bin/sh

LD_LIBRARY_PATH=/home/root/teensy
export LD_LIBRARY_PATH
modprobe gpio-keys
INPUT_DEV=`cat /proc/bus/input/devices | grep "gpio-keys" | grep "Sysfs" | sed 's/.*\/input\(\)/\1/'`
INPUT_DEVNODE=/dev/input/event$INPUT_DEV
rm /home/root/gpio-keys-event
ln -s $INPUT_DEVNODE /home/root/gpio-keys-event
python /home/root/user_button.py &
