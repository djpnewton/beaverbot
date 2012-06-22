# To compile hidapi.dylib in osx:
# Download and unpack hidapi-0.7.0 from https://github.com/signal11/hidapi/downloads
# cd hidapi-0.7.0/mac
# gcc -I../hidapi -fPIC -c hid.c 
# gcc -shared -o hidapi.dylib -framework IOKit -framework CoreFoundation hid.o
# cp hidapi.dylib /opt/local/lib/

#
# simple ctypes wrapper for HIDAPI shared library
#
# Copyright (c) 2011 Pulse-Robotics, Inc.
# Tyler Wilson
#

from ctypes import *

# opaque hidapi structure
#
class device(Structure):
    pass

#device = c_void_p       # opaque hidapi structure

# device_info structure. in two parts to handle the next pointer
class device_info(Structure):
    pass

device_info._fields_ = [
        ("path", c_char_p),                 # Platform-specific device path 
        ("vendor_id", c_ushort),            # Device Vendor ID
        ("product_id", c_ushort),           # Device Product ID
        ("serial_number", c_wchar_p),       # Serial Number
        ("release_number", c_ushort),       # Device Release Number in binary-coded decimal (also known as Device Version Number)
        ("manufacturer_string", c_wchar_p), # Manufacturer String
        ("product_string", c_wchar_p),      # Product string
        ("usage_page", c_ushort),           # Usage Page for this Device/Interface (Windows/Mac only)
        ("usage", c_ushort),                # Usage for this Device/Interface (Windows/Mac only)
        ("interface_number", c_int),        # The USB interface which this logical device represents (Linux/libusb implementation only)
        ("next", POINTER(device_info))      # Pointer to the next device
    ]

# try loading the shared library.
lib = CDLL("hidapi.dll")
if not lib:
    raise ImportError("Cannot find hidapi dynamic library")

# set up all the functions so they match the C versions
if lib:
    
    # int hid_init(void);
    hid_init = lib.hid_init
    hid_init.args = []
    hid_init.restype = c_int

    # int hid_exit(void);
    hid_exit = lib.hid_exit
    hid_exit.args = []
    hid_exit.restype = c_int

    # struct hid_device_info * hid_enumerate(unsigned short vendor_id, unsigned short product_id);
    hid_enumerate = lib.hid_enumerate
    hid_enumerate.args = [c_ushort, c_ushort]
    hid_enumerate.restype = POINTER(device_info)

    # void hid_free_enumeration(struct hid_device_info *devs);
    hid_free_enumeration = lib.hid_free_enumeration
    hid_free_enumeration.args = []
    #hid_free_enumeration.restype = c_void

    # hid_device * hid_open(unsigned short vendor_id, unsigned short product_id, wchar_t *serial_number);
    hid_open = lib.hid_open
    hid_open.args = [c_ushort, c_ushort, c_wchar_p]
    hid_open.restype = POINTER(device)

    # hid_device * hid_open_path(const char *path);
    hid_open_path = lib.hid_open_path
    hid_open_path.args = [c_char_p]
    hid_open_path.restype = POINTER(device)

    # int hid_write(hid_device *device, const unsigned char *data, size_t length);
    hid_write = lib.hid_write
    hid_write.args = [POINTER(device), c_char_p, c_int]
    hid_write.restype = c_int

    # int hid_read(hid_device *device, unsigned char *data, size_t length);
    hid_read = lib.hid_read
    hid_read.args = [POINTER(device), c_char_p, c_int]
    hid_read.restype = c_int

    # int hid_read_timeout(hid_device *dev, unsigned char *data, size_t length, int milliseconds);
    hid_read_timeout = lib.hid_read_timeout
    hid_read_timeout.args = [POINTER(device), c_char_p, c_int, c_int]
    hid_read_timeout.restype = c_int

    # int hid_set_nonblocking(hid_device *device, int nonblock);
    hid_set_nonblocking = lib.hid_set_nonblocking
    hid_set_nonblocking.args = [POINTER(device), c_int]
    hid_set_nonblocking.restype = c_int

    # int hid_send_feature_report(hid_device *device, const unsigned char *data, size_t length);
    hid_send_feature_report = lib.hid_send_feature_report
    hid_send_feature_report.args = [POINTER(device), c_char_p, c_int]
    hid_send_feature_report.restype = c_int

    # int hid_get_feature_report(hid_device *device, unsigned char *data, size_t length);
    hid_get_feature_report = lib.hid_get_feature_report
    hid_get_feature_report.args = [POINTER(device), c_char_p, c_int]
    hid_get_feature_report.restype = c_int

    # void hid_close(hid_device *device);
    hid_close = lib.hid_close
    hid_close.args = [POINTER(device)]
    #hid_close.restype = c_void

    # int hid_get_manufacturer_string(hid_device *device, wchar_t *string, size_t maxlen);
    hid_get_manufacturer_string = lib.hid_get_manufacturer_string
    hid_get_manufacturer_string.args = [POINTER(device), c_wchar_p, c_uint]
    hid_get_manufacturer_string.restype = c_int

    # int hid_get_product_string(hid_device *device, wchar_t *string, size_t maxlen);
    hid_get_product_string = lib.hid_get_product_string
    hid_get_product_string.args = [POINTER(device), c_wchar_p, c_uint]
    hid_get_product_string.restype = c_int

    # int hid_get_serial_number_string(hid_device *device, wchar_t *string, size_t maxlen);
    hid_get_serial_number_string = lib.hid_get_serial_number_string
    hid_get_serial_number_string.args = [POINTER(device), c_wchar_p, c_uint]
    hid_get_serial_number_string.restype = c_int

    # int hid_get_indexed_string(hid_device *device, int string_index, wchar_t *string, size_t maxlen);

    # const wchar_t* hid_error(hid_device *device);
    hid_error = lib.hid_error
    hid_error.args = [POINTER(device)]
    hid_error.restype = c_wchar_p

# static helper functions
def enumerate(vid=0, pid=0):
    return hid_enumerate(vid, pid)

def free_enumeration(di):
    hid_free_enumeration(di)

# TODO: we ought to also include a nice generator wrapper to make the client code more Pythonic

    
# a helper class

class HID:

    def __init__(self):
        self.device = device()

    # struct hid_device_info * hid_enumerate(unsigned short vendor_id, unsigned short product_id);
    @staticmethod
    def enumerate(pid=0, vid=0):
        return hid_enumerate(pid, vid)

    # void hid_free_enumeration(struct hid_device_info *devs);
    @staticmethod
    def free_enumeration(di):
        hid_free_enumeration(di)

    # hid_device * hid_open(unsigned short vendor_id, unsigned short product_id, wchar_t *serial_number);
    def open(self, pid, vid, serial_number_string=None):
        self.device = hid_open(pid, vid, serial_number_string) 
        return self.device

    # hid_device * hid_open_path(const char *path);
    def open_path(self, path):
        self.device = hid_open_path(path)
        return self.device

    # int hid_write(hid_device *device, const unsigned char *data, size_t length);
    def write(self, data):
        return hid_write(self.device, data, sizeof(data))

    # int hid_read(hid_device *device, unsigned char *data, size_t length);
    def read(self, data):
        return hid_read(self.device, data, sizeof(data))

    # int hid_read_timeout(hid_device *dev, unsigned char *data, size_t length, int milliseconds);
    def read_timeout(self, data, timeout):
        return hid_read_timeout(self.device, data, sizeof(data), timeout)

    # int hid_set_nonblocking(hid_device *device, int nonblock);
    def set_nonblocking(self, nonblock):
        return hid_set_nonblocking(self.device, nonblock)

    # int hid_get_input_report(hid_device *device, unsigned char *data, size_t length);
    def get_input_report(self, data):
        return hid_get_input_report(self.device, data, sizeof(data))

    # int hid_send_feature_report(hid_device *device, const unsigned char *data, size_t length);
    def send_feature_report(self, data):
        return hid_send_feature_report(self.device, data, sizeof(data))

    # int hid_get_feature_report(hid_device *device, unsigned char *data, size_t length);
    def get_feature_report(self, data):
        return hid_get_feature_report(self.device, data, sizeof(data))

    # void hid_close(hid_device *device);
    def close(self):
        hid_close(self.device)
        self.device = None

    # int hid_get_manufacturer_string(hid_device *device, wchar_t *string, size_t maxlen);
    def get_manufacturer_string(self):
        s = create_unicode_buffer(256)
        r = hid_get_manufacturer_string(self.device, s, sizeof(s))
        print r, s, self.device
        print s.value
        if r >= 0:
            return s.value
        else:
            raise Exception(self.error())

    # int hid_get_product_string(hid_device *device, wchar_t *string, size_t maxlen);
    def get_product_string(self):
        s = create_unicode_buffer(256)
        r = hid_get_product_string(self.device, s, sizeof(s))
        if r >= 0:
            return s.value
        else:
            raise Exception(self.error())

    # int hid_get_serial_number_string(hid_device *device, wchar_t *string, size_t maxlen);
    def get_serial_number_string(self):
        s = create_unicode_buffer(256)
        r = hid_get_serial_number_string(self.device, s, sizeof(s))
        if r > 0:
            return s.value
        else:
            return "Unknown"
            #raise Exception(self.error())

    # int hid_get_indexed_string(hid_device *device, int string_index, wchar_t *string, size_t maxlen);
    def get_indexed_string(self, string_index):
        s = create_unicode_buffer(256)
        r = hid_get_indexed_string(self.device, string_index, s, sizeof(s))
        if r > 0:
            return s.value
        else:
            raise Exception(self.error())

    # const wchar_t* hid_error(hid_device *device);
    def error(self):
        return hid_error()

#### weather stuff below

def sendSetReport(hid, buffer2):
    buffer = create_string_buffer(len(buffer2))
    for i in range (sizeof(buffer)): buffer[i] = chr(buffer2[i])
    #print(sizeof(buffer), repr(buffer.raw))
    return hid.write(buffer)

def readExact(hid, minl):
    buffer = create_string_buffer(32)
    buf=''
    hid.set_nonblocking(0)
    while len(buf) < minl:
        l = hid.read_timeout(buffer, timeout=10000)
        if l <= 0:
            raise l
        print("read returns %d" % l)
        #print len(buffer), repr(buffer.raw[0:l])
        buf = buf + buffer.raw[0:l]
    #print len(buf), repr(buf)
    block = []
    for i in range(len(buf)):
        block.append(ord(buf[i]))
        #print ord(buf[i]),",",
    return block
        
def query(hid, buf_1, buf_2):
    buffer2 = [0x00, 0xA1, buf_1, buf_2, 0x20, 0xA1, buf_1, buf_2, 0x20]
    print("write %d %d returns %d" % (buf_1, buf_2, sendSetReport(hid, buffer2)))
    buf = readExact(hid, 32)
    print len(buf), repr(buf)
    return buf

if __name__ == "__main__":
    # Enumeration test

    di = POINTER(device_info)
    root = di = enumerate()

    while di:
        print("PID:0x%04x, VID:0x%04x, Product:" %(di.contents.vendor_id, di.contents.product_id), di.contents.product_string)
        print("Path:%s" % (di.contents.path))
        di = di.contents.next

    free_enumeration(root)

    
    # also works with HID static methods
#    root = di = HID.enumerate()

#    while di:
#        print("PID:0x%04x, VID:0x%04x, Product:%s" % (di.contents.vendor_id, di.contents.product_id, di.contents.product_string))
        #print("Path:%s" % (di.contents.path))
#        di = di.contents.next

#    HID.free_enumeration(root)

    # instance test
    hid = HID()

    #device = hid.open(0x10C4, 0xBEEF)
    # Apple IR
    #device = hid.open(0x05ac, 0x8240)
    # Weather station
    device = hid.open(0x1941, 0x8021)

    if device:
        hid.set_nonblocking(1)

        print("ManStr returned: %s" % hid.get_manufacturer_string())
        print("ProStr returned: %s" % hid.get_product_string())
        print("SerStr returned: %s" % hid.get_serial_number_string())

        query(hid, 0,0)
        query(hid, 0,32)
        hid.close()

    else:
        print("Could not open device")
