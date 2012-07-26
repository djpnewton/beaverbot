#!/bin/sh

LD_LIBRARY_PATH=/home/root/teensy
export LD_LIBRARY_PATH
modprobe gpio-keys
python /home/root/user_button.py &
