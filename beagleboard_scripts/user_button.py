#!/usr/bin/python
# Beagleboard user button
import struct
inputDevice = "/dev/input/event1"
# format of the event structure (int, int, short, short, int)
inputEventFormat = 'iihhi'
inputEventSize = 16
 
file = open(inputDevice, "rb") # standard binary file input
event = file.read(inputEventSize)
state = 0
while event:
  (time1, time2, type, code, value) = struct.unpack(inputEventFormat, event)
  if type == 1 and code == 276 and value == 1:
    import os
    if state % 2 == 0:
      print "start!", state
      os.system("python /home/root/teensy/controller.py 1")
      os.system("python /home/root/teensy/controller.py 3")
    else:
      print "stop!", state
      os.system("python /home/root/teensy/controller.py 0")
    state = state + 1
  event = file.read(inputEventSize)
file.close()
