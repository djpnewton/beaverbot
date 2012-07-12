#!/usr/bin/env python

import sys
import ctypes

# load library
if sys.platform == "win32":
    libname = "kowhai.dll"
else:
    libname = "libkowhai.so"
kowhai_lib = ctypes.cdll.LoadLibrary(libname)

# basic type alias'
uint8_t = ctypes.c_uint8
uint16_t = ctypes.c_uint16
uint32_t = ctypes.c_uint32

# kowhai node types
KOW_BRANCH_START = 0x00
KOW_BRANCH_END = 0x01
KOW_INT8 = 0x70
KOW_UINT8 = 0x71
KOW_INT16 = 0x72
KOW_UINT16 = 0x73
KOW_INT32 = 0x74
KOW_UINT32 = 0x75
KOW_FLOAT = 0x76
KOW_CHAR = 0x77

# kowhai node structure
class kowhai_node_t(ctypes.Structure):
    _pack_ = 1
    _fields_ = [('type_', uint16_t),
                ('symbol', uint16_t),
                ('count', uint16_t),
                ('tag', uint16_t)]
    def __str__(self):
        return "kowhai_node_t(%d, %d, %d, %d)" % (self.type_, self.symbol, self.count, self.tag)

# kowhai tree structure
class kowhai_tree_t(ctypes.Structure):
    _pack_ = 1
    _fields_ = [('desc', ctypes.POINTER(kowhai_node_t)),
                ('data', ctypes.c_void_p)]

class kowhai_symbol_parts_t(ctypes.Structure):
    _pack_ = 1
    _fields_ = [('name', uint16_t),
                ('array_index', uint16_t)]

# kowhai symbol type
class kowhai_symbol_t(ctypes.Union):
    _pack_ = 1
    _fields_ = [('symbol', uint32_t),
                ('parts', kowhai_symbol_parts_t)]

def KOWHAI_SYMBOL(name, array_index):
    return (array_index << 16) + name

# kowhai function return values
KOW_STATUS_OK                       = 0
KOW_STATUS_INVALID_SYMBOL_PATH      = 1
KOW_STATUS_INVALID_DESCRIPTOR       = 2
KOW_STATUS_INVALID_OFFSET           = 3
KOW_STATUS_NODE_DATA_TOO_SMALL      = 4
KOW_STATUS_INVALID_NODE_TYPE        = 5
KOW_STATUS_PACKET_BUFFER_TOO_SMALL  = 6
KOW_STATUS_INVALID_PROTOCOL_COMMAND = 7
KOW_STATUS_PACKET_BUFFER_TOO_BIG    = 8
KOW_STATUS_TARGET_BUFFER_TOO_SMALL  = 9
KOW_STATUS_BUFFER_INVALID           = 10
KOW_STATUS_SCRATCH_TOO_SMALL        = 11
KOW_STATUS_NOT_FOUND                = 12
KOW_STATUS_INVALID_SEQUENCE         = 13
KOW_STATUS_NO_DATA                  = 14

#uint32_t kowhai_version(void);
def version():
    return kowhai_lib.kowhai_version()

#int kowhai_get_node_type_size(uint16_t type);
def get_node_type_size(type_):
    return kowhai_lib.kowhai_get_node_type_size(uint16_t(type_))

#int kowhai_get_node(const struct kowhai_node_t *node, int num_symbols, const union kowhai_symbol_t *symbols, int *offset, struct kowhai_node_t **target_node);
def get_node(node, num_symbols, symbols, offset, target_node):
    return kowhai_lib.kowhai_get_node(ctypes.byref(node), ctypes.c_int(num_symbols), ctypes.byref(symbols),
            ctypes.byref(offset), ctypes.byref(target_node))

#int kowhai_get_node_size(const struct kowhai_node_t *node, int *size);
def get_node_size(node, size):
    return kowhai_lib.kowhai_get_node_size(ctypes.byref(node), ctypes.byref(size))

#int kowhai_get_node_count(const struct kowhai_node_t *node, int *count);
def get_node_count(node, count):
    return kowhai_lib.kowhai_get_node_count(ctypes.byref(node), ctypes.byref(count))

#int kowhai_read(struct kowhai_tree_t *tree, int num_symbols, union kowhai_symbol_t* symbols, int read_offset, void* result, int read_size);
def read(tree, num_symbols, symbols, read_offset, result, read_size):
    return kowhai_lib.kowhai_read(ctypes.byref(tree), ctypes.c_int(num_symbols), ctypes.byref(symbols), ctypes.c_int(read_offset), result, ctypes.c_int(read_size))

#int kowhai_write(struct kowhai_tree_t *tree, int num_symbols, union kowhai_symbol_t* symbols, int write_offset, void* value, int write_size);
def write(tree, num_symbols, symbols, write_offset, value, write_size):
    return kowhai_lib.kowhai_write(ctypes.byref(tree), ctypes.c_int(num_symbols), ctypes.byref(symbols), ctypes.c_int(write_offset), value, ctypes.c_int(write_size))

#int kowhai_get_int8(struct kowhai_tree_t *tree, int num_symbols, union kowhai_symbol_t* symbols, int8_t* result);
def get_int8(tree, num_symbols, symbols, result):
    return kowhai_lib.kowhai_get_int8(ctypes.byref(tree), ctypes.c_int(num_symbols), ctypes.byref(symbols), ctypes.byref(result)) 

#int kowhai_get_int16(struct kowhai_tree_t *tree, int num_symbols, union kowhai_symbol_t* symbols, int16_t* result);
def get_int16(tree, num_symbols, symbols, result):
    return kowhai_lib.kowhai_get_int16(ctypes.byref(tree), ctypes.c_int(num_symbols), ctypes.byref(symbols), ctypes.byref(result)) 

#int kowhai_get_int32(struct kowhai_tree_t *tree, int num_symbols, union kowhai_symbol_t* symbols, int16_t* result);
def get_int32(tree, num_symbols, symbols, result):
    return kowhai_lib.kowhai_get_int32(ctypes.byref(tree), ctypes.c_int(num_symbols), ctypes.byref(symbols), ctypes.byref(result)) 

#int kowhai_get_float(struct kowhai_tree_t *tree, int num_symbols, union kowhai_symbol_t* symbols, float* result);
def get_float(tree, num_symbols, symbols, result):
    return kowhai_lib.kowhai_get_float(ctypes.byref(tree), ctypes.c_int(num_symbols), ctypes.byref(symbols), ctypes.byref(result)) 

#int kowhai_set_int8(struct kowhai_tree_t *tree, int num_symbols, union kowhai_symbol_t* symbols, uint8_t value);
def set_int8(tree, num_symbols, symbols, value):
    return kowhai_lib.kowhai_set_int8(ctypes.byref(tree), ctypes.c_int(num_symbols), ctypes.byref(symbols), ctypes.c_byte(value))

#int kowhai_set_int16(struct kowhai_tree_t *tree, int num_symbols, union kowhai_symbol_t* symbols, int16_t value);
def set_int16(tree, num_symbols, symbols, value):
    return kowhai_lib.kowhai_set_int16(ctypes.byref(tree), ctypes.c_int(num_symbols), ctypes.byref(symbols), ctypes.c_short(value))

#int kowhai_set_int32(struct kowhai_tree_t *tree, int num_symbols, union kowhai_symbol_t* symbols, int32_t value);
def set_int32(tree, num_symbols, symbols, value):
    return kowhai_lib.kowhai_set_int32(ctypes.byref(tree), ctypes.c_int(num_symbols), ctypes.byref(symbols), ctypes.c_int(value))

#int kowhai_set_float(struct kowhai_tree_t *tree, int num_symbols, union kowhai_symbol_t* symbols, float value);
def set_float(tree, num_symbols, symbols, value):
    return kowhai_lib.kowhai_set_float(ctypes.byref(tree), ctypes.c_int(num_symbols), ctypes.byref(symbols), ctypes.c_float(value))

if __name__ == "__main__":
    descriptor = (kowhai_node_t * 9)(
            kowhai_node_t(KOW_BRANCH_START, 0, 1, 0),
            kowhai_node_t(KOW_UINT8,        1, 1, 1),
            kowhai_node_t(KOW_BRANCH_START, 2, 1, 2),
            kowhai_node_t(KOW_FLOAT,        3, 1, 3),
            kowhai_node_t(KOW_UINT8,        4, 1, 5),
            kowhai_node_t(KOW_UINT16,       5, 1, 6),
            kowhai_node_t(KOW_UINT32,       6, 1, 7),
            kowhai_node_t(KOW_BRANCH_END,   2, 0, 8),
            kowhai_node_t(KOW_BRANCH_END,   0, 0, 9)
            )
    class test_data_t(ctypes.Structure):
        _pack_ = 1
        _fields_ = [('a', uint8_t),
                    ('b', ctypes.c_float),
                    ('c', uint8_t),
                    ('d', uint16_t),
                    ('e', uint32_t)]

    tree = kowhai_tree_t()
    tree.desc = descriptor
    tree.data = ctypes.cast(ctypes.pointer(test_data_t(11, 22.2, 33, 44, 55)), ctypes.c_void_p)
    num_symbols = 3
    symbols_f = (kowhai_symbol_t * num_symbols)(
            kowhai_symbol_t(0),
            kowhai_symbol_t(2),
            kowhai_symbol_t(3))
    symbols_u8 = (kowhai_symbol_t * num_symbols)(
            kowhai_symbol_t(0),
            kowhai_symbol_t(2),
            kowhai_symbol_t(4))
    symbols_u16 = (kowhai_symbol_t * num_symbols)(
            kowhai_symbol_t(0),
            kowhai_symbol_t(2),
            kowhai_symbol_t(5))
    symbols_u32 = (kowhai_symbol_t * num_symbols)(
            kowhai_symbol_t(0),
            kowhai_symbol_t(2),
            kowhai_symbol_t(6))
    print "test kowhai wrapper"
    print "  kowhai_version() =", version()
    print "  KOW_INT32 size is", get_node_type_size(KOW_INT32)
    offset = ctypes.c_int()
    target_node = ctypes.pointer(kowhai_node_t())
    res = get_node(descriptor, num_symbols, symbols_u8, offset, target_node)
    print "  kowhai_get_node() - res: %d, offset: %s, target_node: %s" % (res, offset, target_node.contents)
    size = ctypes.c_int()
    res = get_node_size(descriptor, size)
    print "  kowhai_get_node_size() - res: %d, size: %d" % (res, size.value)
    count = ctypes.c_int()
    res = get_node_count(descriptor, count)
    print "  kowhai_get_node_count() - res: %d, count: %d" % (res, count.value)
    ptr = ctypes.pointer(uint8_t(34))
    res = write(tree, num_symbols, symbols_u8, 0, ptr, 1) 
    print "  kowhai_write() - res: %d" % (res)
    ptr = ctypes.pointer(uint8_t())
    res = read(tree, num_symbols, symbols_u8, 0, ptr, 1) 
    print "  kowhai_read() - res: %d, value: %d" % (res, ptr.contents.value)
    res = set_int8(tree, num_symbols, symbols_u8, 10)
    print "  kowhai_set_int8() - res: %d" % res
    res = set_int16(tree, num_symbols, symbols_u16, 20)
    print "  kowhai_set_int16() - res: %d" % res
    res = set_int32(tree, num_symbols, symbols_u32, 30)
    print "  kowhai_set_int32() - res: %d" % res
    res = set_float(tree, num_symbols, symbols_f, 40.5)
    print "  kowhai_set_float() - res: %d" % res
    value = uint8_t()
    res = get_int8(tree, num_symbols, symbols_u8, value)
    print "  kowhai_get_int8() - res: %d, value: %d" % (res, value.value)
    value = uint16_t()
    res = get_int16(tree, num_symbols, symbols_u16, value)
    print "  kowhai_get_int16() - res: %d, value: %d" % (res, value.value)
    value = uint32_t()
    res = get_int32(tree, num_symbols, symbols_u32, value)
    print "  kowhai_get_int32() - res: %d, value: %d" % (res, value.value)
    value = ctypes.c_float()
    res = get_float(tree, num_symbols, symbols_f, value)
    print "  kowhai_get_float() - res: %d, value: %f" % (res, value.value)
