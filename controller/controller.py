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
        #TEMP TODO (linux problem??)
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
    prot.payload.buffer_ = ctypes.cast(ctypes.pointer(ctypes.create_string_buffer(program)), ctypes.c_void_p)
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
    string = direction1 + pwm1 + direction2 + pwm2
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
    import getopt, sys
    opts, args = getopt.getopt(sys.argv[1:], "p:d:D:s:S:", ["program="])
    for o, a in opts:
        if o == '-p':
            program = chr(int(a))
            print "program:", program
        elif o == '-d':
            dir1 = chr(int(a))
            print "dir1", dir1
        elif o == '-D':
            dir2 = chr(int(a))
            print "dir2", dir2
        elif o == '-s':
            pwm1 = chr(int(a))
            print "pwm1", pwm1
        elif o == '-S':
            pwm2 = chr(int(a))
            print "pwm2", pwm2

    # open teensy hid
    path = find_teensy()
    if path:
        # open hid
        dev = hidapi.hid_open_path(path)
        if program:
            print "set program", program
            # create write program packet
            buf, buf_size = create_teensy_program_packet(program)
            # write to teensy
            write_teensy(dev, buf, buf_size)
        if dir1:
            print "set motors", dir1, pwm1, dir2, pwm2
            # create set_motor packet
            buf, buf_size = create_teensy_set_motor_packet(dir1, pwm1, dir2, pwm2)
            # write to teensy
            write_teensy(dev, buf, buf_size)
        # close hid
        hidapi.hid_close(dev)
