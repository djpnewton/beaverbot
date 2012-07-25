#!/usr/bin/python
# Beagleboard user button
import os

CONTROLLER_PATH = "python /home/root/teensy/controller.py"
counter = 0

def start_controller():
    global counter
    print "start controller %d!" % (counter - 1)
    os.system("%s -p%d" % (CONTROLLER_PATH, counter - 1))

    counter = 0

def read_button():
    import threading
    import struct
    inputDevice = "/home/root/gpio-keys-event"
    # format of the event structure (int, int, short, short, int)
    inputEventFormat = 'iihhi'
    inputEventSize = 16

    file = open(inputDevice, "rb") # standard binary file input
    event = file.read(inputEventSize)
    timer = None
    while event:
        (time1, time2, type, code, value) = struct.unpack(inputEventFormat, event)
        if type == 1 and code == 276 and value == 1:
            global counter
            counter += 1
            print "user button", counter
            if timer:
                timer.cancel()
            timer = threading.Timer(2, start_controller)
            timer.start()
            os.system("%s -f%d -t%d" % (CONTROLLER_PATH, 50 + counter * 10, 3000))
        event = file.read(inputEventSize)
    file.close()

if __name__ == "__main__":
    read_button()
