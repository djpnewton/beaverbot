#!/usr/bin/env python

import ctypes

from kowhai import *

# protocol commands 
KOW_CMD_GET_VERSION = 0x00
KOW_CMD_GET_VERSION_ACK = 0x0F
KOW_CMD_GET_TREE_LIST = 0x10
KOW_CMD_GET_TREE_LIST_ACK = 0x1F
KOW_CMD_GET_TREE_LIST_ACK_END = 0x1E
KOW_CMD_WRITE_DATA = 0x20
KOW_CMD_WRITE_DATA_END = 0x21
KOW_CMD_WRITE_DATA_ACK = 0x2F
KOW_CMD_READ_DATA = 0x30
KOW_CMD_READ_DATA_ACK = 0x3F
KOW_CMD_READ_DATA_ACK_END = 0x3E
KOW_CMD_READ_DESCRIPTOR = 0x40
KOW_CMD_READ_DESCRIPTOR_ACK = 0x4F
KOW_CMD_READ_DESCRIPTOR_ACK_END = 0x4E
KOW_CMD_GET_FUNCTION_LIST = 0x50
KOW_CMD_GET_FUNCTION_LIST_ACK = 0x5F
KOW_CMD_GET_FUNCTION_LIST_ACK_END = 0x5E
KOW_CMD_GET_FUNCTION_DETAILS = 0x60
KOW_CMD_GET_FUNCTION_DETAILS_ACK = 0x6F
KOW_CMD_CALL_FUNCTION = 0x70
KOW_CMD_CALL_FUNCTION_ACK = 0x7F
KOW_CMD_CALL_FUNCTION_RESULT = 0x7E
KOW_CMD_CALL_FUNCTION_RESULT_END = 0x7D
KOW_CMD_CALL_FUNCTION_FAILED = 0x7C
KOW_CMD_GET_SYMBOL_LIST = 0x80
KOW_CMD_GET_SYMBOL_LIST_ACK = 0x8F
KOW_CMD_GET_SYMBOL_LIST_ACK_END = 0x8E

# protocol error codes
KOW_CMD_ERROR_INVALID_COMMAND = 0xF0
KOW_CMD_ERROR_INVALID_TREE_ID = 0xF1
KOW_CMD_ERROR_INVALID_FUNCTION_ID = 0xF2
KOW_CMD_ERROR_INVALID_SYMBOL_PATH = 0xF3
KOW_CMD_ERROR_INVALID_PAYLOAD_OFFSET = 0xF4
KOW_CMD_ERROR_INVALID_PAYLOAD_SIZE = 0xF5
KOW_CMD_ERROR_INVALID_SEQUENCE = 0xF6
KOW_CMD_ERROR_UNKNOWN = 0xFF

class kowhai_protocol_header_t(ctypes.Structure):
    _pack_ = 1
    _fields_ = [('command', uint8_t),
                ('id_', uint16_t)]

class kowhai_protocol_symbol_spec_t(ctypes.Structure):
    _pack_ = 1
    _fields_ = [('count', uint8_t),
                ('array_', ctypes.POINTER(kowhai_symbol_t))]

class kowhai_protocol_data_payload_memory_spec_t(ctypes.Structure):
    _pack_ = 1
    _fields_ = [('type_', uint16_t),
                ('offset', uint16_t),
                ('size', uint16_t)]

class kowhai_protocol_data_payload_spec_t(ctypes.Structure):
    _pack_ = 1
    _fields_ = [('symbols', kowhai_protocol_symbol_spec_t),
                ('memory', kowhai_protocol_data_payload_memory_spec_t)]

class kowhai_protocol_descriptor_payload_spec_t(ctypes.Structure):
    _pack_ = 1
    _fields_ = [('node_count', uint16_t),
                ('offset', uint16_t),
                ('size', uint16_t)]

class kowhai_protocol_id_list_t(ctypes.Structure):
    _pack_ = 1
    _fields_ = [('list_count', uint16_t),
                ('offset', uint16_t),
                ('size', uint16_t)]

class kowhai_protocol_string_list_t(ctypes.Structure):
    _pack_ = 1
    _fields_ = [('list_count', uint16_t),
                ('list_total_size', uint32_t),
                ('offset', uint16_t),
                ('size', uint16_t)]

class kowhai_protocol_function_details_t(ctypes.Structure):
    _pack_ = 1
    _fields_ = [('tree_in_id', uint16_t),
                ('tree_out_id', uint16_t)]

class kowhai_protocol_function_call_t(ctypes.Structure):
    _pack_ = 1
    _fields_ = [('offset', uint16_t),
                ('size', uint16_t)]

class kowhai_protocol_payload_spec_t(ctypes.Union):
    _pack_ = 1
    _fields_ = [('version', uint32_t),
                ('id_list', kowhai_protocol_id_list_t),
                ('data', kowhai_protocol_data_payload_spec_t),
                ('descriptor', kowhai_protocol_descriptor_payload_spec_t),
                ('function_details', kowhai_protocol_function_details_t),
                ('function_call', kowhai_protocol_function_call_t),
                ('string_list', kowhai_protocol_string_list_t)]

class kowhai_protocol_payload_t(ctypes.Structure):
    _pack_ = 1
    _fields_ = [('spec', kowhai_protocol_payload_spec_t),
                ('buffer_', ctypes.c_void_p)]

class kowhai_protocol_t(ctypes.Structure):
    _pack_ = 1
    _fields_ = [('header', kowhai_protocol_header_t),
                ('payload', kowhai_protocol_payload_t)]

#int kowhai_protocol_parse(void* proto_packet, int packet_size, struct kowhai_protocol_t* protocol);
def parse(proto_packet, packet_size, protocol):
    return kowhai_lib.kowhai_protocol_parse(ctypes.byref(proto_packet), ctypes.c_int(packet_size), ctypes.byref(protocol))

#int kowhai_protocol_create(void* proto_packet, int packet_size, struct kowhai_protocol_t* protocol, int* bytes_required);
def create(proto_packet, packet_size, protocol, bytes_required):
    return kowhai_lib.kowhai_protocol_create(ctypes.byref(proto_packet), ctypes.c_int(packet_size), ctypes.byref(protocol), ctypes.byref(bytes_required))

#int kowhai_protocol_get_overhead(struct kowhai_protocol_t* protocol, int* overhead);
def get_overhead(protocol, overhead):
    return kowhai_lib.kowhai_protocol_get_overhead(ctypes.byref(protocol), ctypes.byref(overhead))

if __name__ == "__main__":
    print "test kowhai protocol wrapper"
    buf = ctypes.create_string_buffer("\x10\x01\x00")
    prot = kowhai_protocol_t()
    res = parse(buf, 3, prot) 
    print "kowhai_protocol_parse() - res %d, prot.command: %d, prot.id: %d" % (res, prot.header.command, prot.header.id_)
    prot.header.command = KOW_CMD_READ_DESCRIPTOR
    prot.header.id_ = 65535
    bytes_required = ctypes.c_int()
    res = create(buf, 3, prot, bytes_required)
    print "kowhai_protocol_create() - res %d, bytes_required: %d, buf: %s" % (res, bytes_required.value, repr(buf.raw))
    overhead = ctypes.c_int()
    res = get_overhead(prot, overhead)
    print "kowhai_protocol_get_overhead() - res: %d, overhead: %d" % (res, overhead.value)
