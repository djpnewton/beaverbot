#!/usr/bin/python
# Beagleboard user button
import os

CONTROLLER_PATH = "python /home/root/teensy/controller.py"
counter = 0

def start_controller():
    global counter
    if counter == 10:
        # can chase
        print "can chase!"
        os.system("%s -c0" % (CONTROLLER_PATH))
    elif counter == 11:
        # can chase
        print "can chase debug!"
        os.system("%s -c1" % (CONTROLLER_PATH))
    elif counter == 12:
        # can chase
        print "can guidance!"
        os.system("%s -g" % (CONTROLLER_PATH))
    else:
        # teensy program
        print "teensy program %d!" % (counter - 1)
        os.system("%s -p%d" % (CONTROLLER_PATH, counter - 1))
    counter = 0

def kill_controller():
    p = os.popen(r"ps -ef | grep -v 'grep' | grep teensy/control | sed 's/root\s*\([0-9]*\)\s.*/\1/'")
    s = p.read()
    p.close()
    if s:
        print "kill process:", s
        os.system("kill %s" % s)

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
            kill_controller()
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
    # alert that we are ready to go
    for i in range(5):
        os.system("%s -f%d -t%d" % (CONTROLLER_PATH, 200 - i * 10, 10000))
    # start reading the button
    read_button()
