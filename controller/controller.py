from kowhai import *
from kowhai_protocol import *
import hidapi

import ctypes

# find teensy hid device
path = None
orig_devinfo = devinfo = hidapi.HID.enumerate(0xBEEF, 0xBABE)
while devinfo:
    print "Path:      ", devinfo.contents.path
    print "Vendor Id: ", hex(devinfo.contents.vendor_id)
    print "Product Id:", hex(devinfo.contents.product_id)
    print "Usage Page:", hex(devinfo.contents.usage_page)
    print "Usage:     ", hex(devinfo.contents.usage)
    print
    if devinfo.contents.usage_page == 0xFF00 and devinfo.contents.usage == 0x100:
        print "Found device"
        path = devinfo.contents.path
        break
    devinfo = devinfo.contents.next
hidapi.HID.free_enumeration(orig_devinfo)

# get program param
program = "\x01"
import sys
if len(sys.argv) > 1:
    program = chr(int(sys.argv[1]))
print "program:", program

# open teensy hid
if path:
    # open hid
    dev = hidapi.hid_open_path(path)
    # create packet
    buf_size = 63
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
    # add report id
    buf = ctypes.create_string_buffer("\x00" + buf.raw)
    buf_size = buf_size + 1
    # write data
    hidapi.hid_write(dev, buf, buf_size)
    # close hid
    hidapi.hid_close(dev)

