from kowhai import *
from kowhai_protocol import *
import hidapi

import ctypes

TEENSY_REPORT_SIZE = 64

# find teensy hid device
def find_teensy():
    path = None
    orig_devinfo = devinfo = hidapi.HID.enumerate(0xBEEF, 0xBABE)
    while devinfo:
        print "Path:      ", devinfo.contents.path
        print "Vendor Id: ", hex(devinfo.contents.vendor_id)
        print "Product Id:", hex(devinfo.contents.product_id)
        print "Usage Page:", hex(devinfo.contents.usage_page)
        print "Usage:     ", hex(devinfo.contents.usage)
        print
        #TEMP TODO (linux problem with hidapi and hidraw)
        if 1 or (devinfo.contents.usage_page == 0xFF00 and devinfo.contents.usage == 0x100):
            print "Found device"
            path = devinfo.contents.path
            break
        devinfo = devinfo.contents.next
    hidapi.HID.free_enumeration(orig_devinfo)
    return path

def write_teensy(dev, buf, buf_size):
    # add report id
    buf = ctypes.create_string_buffer("\x00" + buf.raw)
    buf_size = buf_size + 1
    # write data
    return hidapi.hid_write(dev, buf, buf_size)

def create_teensy_program_packet(program):
    # create packet
    buf_size = TEENSY_REPORT_SIZE - 1
    buf = ctypes.create_string_buffer("\x00" * buf_size)
    prot = kowhai_protocol_t()
    prot.header.command = KOW_CMD_WRITE_DATA_END
    prot.header.id_ = 0 # tree_id SYM_TEENSY
    num_symbols = 2
    symbols_program = (kowhai_symbol_t * num_symbols)(
            kowhai_symbol_t(0),  # SYM_TEENSY
            kowhai_symbol_t(14)) # SYM_PROGRAM
    prot.payload.spec.data.symbols.count = num_symbols
    prot.payload.spec.data.symbols.array_ = symbols_program
    prot.payload.spec.data.memory.offset = 0
    prot.payload.spec.data.memory.size = 1
    prot.payload.buffer_ = ctypes.cast(ctypes.pointer(ctypes.create_string_buffer(chr(program))), ctypes.c_void_p)
    bytes_required = ctypes.c_int()
    res = create(buf, buf_size, prot, bytes_required)
    return buf, buf_size

def create_teensy_set_motor_packet(direction1, pwm1, direction2, pwm2):
    # create packet
    buf_size = TEENSY_REPORT_SIZE - 1
    buf = ctypes.create_string_buffer("\x00" * buf_size)
    prot = kowhai_protocol_t()
    prot.header.command = KOW_CMD_CALL_FUNCTION
    prot.header.id_ = 21 # tree_id SYM_SET_MOTOR
    prot.payload.spec.function_call.offset = 0
    prot.payload.spec.function_call.size = 4
    string = chr(direction1) + chr(pwm1) + chr(direction2) + chr(pwm2)
    prot.payload.buffer_ = ctypes.cast(ctypes.pointer(ctypes.create_string_buffer(string)), ctypes.c_void_p)
    bytes_required = ctypes.c_int()
    res = create(buf, buf_size, prot, bytes_required)
    return buf, buf_size

def create_teensy_beep_packet(freq, duration):
    # create packet
    buf_size = TEENSY_REPORT_SIZE - 1
    buf = ctypes.create_string_buffer("\x00" * buf_size)
    prot = kowhai_protocol_t()
    prot.header.command = KOW_CMD_CALL_FUNCTION
    prot.header.id_ = 24 # tree_id SYM_BEEP
    prot.payload.spec.function_call.offset = 0
    prot.payload.spec.function_call.size = 4
    string = chr(freq & 0x00ff) + chr(freq >> 8) + chr(duration & 0x00ff) + chr(duration >> 8)
    for s in string:
        print  ord(s)
    prot.payload.buffer_ = ctypes.cast(ctypes.pointer(ctypes.create_string_buffer(string)), ctypes.c_void_p)
    bytes_required = ctypes.c_int()
    res = create(buf, buf_size, prot, bytes_required)
    return buf, buf_size

if __name__ == "__main__":
    # get program param
    program = None
    dir1 = None
    dir2 = None
    pwm1 = None
    pwm2 = None
    freq = None
    duration = None
    can_chase = None
    import getopt, sys
    opts, args = getopt.getopt(sys.argv[1:], "p:d:D:s:S:f:t:c", ["program="])
    for o, a in opts:
        if o == '-p':
            program = int(a)
            print "program:", program
        elif o == '-d':
            dir1 = int(a)
            print "dir1", dir1
        elif o == '-D':
            dir2 = int(a)
            print "dir2", dir2
        elif o == '-s':
            pwm1 = int(a)
            print "pwm1", pwm1
        elif o == '-S':
            pwm2 = int(a)
            print "pwm2", pwm2
        elif o == '-f':
            freq = int(a)
            print "freq", freq
        elif o == '-t':
            duration = int(a)
            print "duration", duration
        elif o == '-c':
            can_chase = True
            print "can_chase"

    # open teensy hid
    path = find_teensy()
    if path:
        # open hid
        dev = hidapi.hid_open_path(path)
        if program != None:
            print "set program", program
            # create write program packet
            buf, buf_size = create_teensy_program_packet(program)
            # write to teensy
            write_teensy(dev, buf, buf_size)
        if dir1 != None:
            print "set motors", dir1, pwm1, dir2, pwm2
            # create set_motor packet
            buf, buf_size = create_teensy_set_motor_packet(dir1, pwm1, dir2, pwm2)
            # write to teensy
            write_teensy(dev, buf, buf_size)
        if freq != None:
            print "beep", freq, duration
            # create beep packet
            buf, buf_size = create_teensy_beep_packet(freq, duration)
            # write to teensy
            write_teensy(dev, buf, buf_size)
        if can_chase:
            import cv
            import dancanfind # need PYTHONPATH set for this
            # get cam and set the width and height
            cap = cv.CaptureFromCAM(-1)
            cv.SetCaptureProperty(cap, cv.CV_CAP_PROP_FRAME_WIDTH, 320);
            cv.SetCaptureProperty(cap, cv.CV_CAP_PROP_FRAME_HEIGHT, 240);
            # hunt can
            while True:
                image = cv.QueryFrame(cap)
                rect = dancanfind.find_can(dancanfind.INDIGO_LASER, image)
                print rect
                if rect:
                    dir1, dir2 = 1, 1
                    pwm1, pwm2 = 50, 50
                    x = rect[0] + rect[2] / 2
                    h = 320 / 2.0
                    if x > h:
                        m = int((x - h) / h * 50)
                        pwm1 += m
                        pwm2 -= m
                    else:
                        m = int((h - x) / h * 50)
                        pwm2 += m
                        pwm1 -= m
                else:
                    dir1, dir2, pwm1, pwm2 = 0, 0, 0, 0
                print "set motors", dir1, pwm1, dir2, pwm2
                # create set_motor packet
                buf, buf_size = create_teensy_set_motor_packet(dir1, pwm1, dir2, pwm2)
                # write to teensy
                write_teensy(dev, buf, buf_size)

        # close hid
        hidapi.hid_close(dev)
