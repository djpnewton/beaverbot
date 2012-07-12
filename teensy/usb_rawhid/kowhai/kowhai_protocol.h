#ifndef _KOWHAI_PROTOCOL_H_
#define _KOWHAI_PROTOCOL_H_

#include "kowhai.h" 

//
// Protocol commands
//

// Get the protocol version
#define KOW_CMD_GET_VERSION                  0x00
#define KOW_CMD_GET_VERSION_ACK              0x0F

// Get the tree list
#define KOW_CMD_GET_TREE_LIST                0x10
// Acknowledge get tree list command
#define KOW_CMD_GET_TREE_LIST_ACK            0x1F
// Acknowledge get tree list command (this is the final packet)
#define KOW_CMD_GET_TREE_LIST_ACK_END        0x1E

// Write tree data
#define KOW_CMD_WRITE_DATA                   0x20
// Write tree data (this is the final write packet)
#define KOW_CMD_WRITE_DATA_END               0x21
// Acknowledge write tree data command
#define KOW_CMD_WRITE_DATA_ACK               0x2F

// Read tree data
#define KOW_CMD_READ_DATA                    0x30
// Acknowledge read tree data command (and return the data)
#define KOW_CMD_READ_DATA_ACK                0x3F
// Acknowledge read tree data command (this is the final packet)
#define KOW_CMD_READ_DATA_ACK_END            0x3E

// Read the tree descriptor
#define KOW_CMD_READ_DESCRIPTOR              0x40
// Acknowledge read tree command (and return tree contents)
#define KOW_CMD_READ_DESCRIPTOR_ACK          0x4F
// Acknowledge read tree command (this is the final packet)
#define KOW_CMD_READ_DESCRIPTOR_ACK_END      0x4E

// Get the function list
#define KOW_CMD_GET_FUNCTION_LIST            0x50
// Acknowledge get function list command (and return list)
#define KOW_CMD_GET_FUNCTION_LIST_ACK        0x5F
// Acknowledge get function list command (this is the final packet)
#define KOW_CMD_GET_FUNCTION_LIST_ACK_END    0x5E

// Get function details
#define KOW_CMD_GET_FUNCTION_DETAILS         0x60
// Acknowledge get function details command
#define KOW_CMD_GET_FUNCTION_DETAILS_ACK     0x6F

// Call function
#define KOW_CMD_CALL_FUNCTION                0x70
// Acknowledge call function command
#define KOW_CMD_CALL_FUNCTION_ACK            0x7F
// Call function result command
#define KOW_CMD_CALL_FUNCTION_RESULT         0x7E
// Call function result command (this is the final packet)
#define KOW_CMD_CALL_FUNCTION_RESULT_END     0x7D
// Call function failed
#define KOW_CMD_CALL_FUNCTION_FAILED         0x7C

// Server event
#define KOW_CMD_EVENT                        0x80
// Server event (final packet)
#define KOW_CMD_EVENT_END                    0x8F

// Get the symbol list
#define KOW_CMD_GET_SYMBOL_LIST              0x90
// Acknowledge get symbol list command (and return list)
#define KOW_CMD_GET_SYMBOL_LIST_ACK          0x9F
// Acknowledge get symbol list command (this is the final packet)
#define KOW_CMD_GET_SYMBOL_LIST_ACK_END      0x9E


// Error codes
#define KOW_CMD_ERROR_INVALID_COMMAND        0xF0
#define KOW_CMD_ERROR_INVALID_TREE_ID        0xF1
#define KOW_CMD_ERROR_INVALID_FUNCTION_ID    0xF2
#define KOW_CMD_ERROR_INVALID_SYMBOL_PATH    0xF3
#define KOW_CMD_ERROR_INVALID_PAYLOAD_OFFSET 0xF4
#define KOW_CMD_ERROR_INVALID_PAYLOAD_SIZE   0xF5
#define KOW_CMD_ERROR_INVALID_SEQUENCE       0xF6
#define KOW_CMD_ERROR_NO_DATA                0xF7
#define KOW_CMD_ERROR_UNKNOWN                0xFF

//
// Protocol structures
//

#pragma pack(1)

/**
 * @brief 
 */
struct kowhai_protocol_header_t
{
    uint8_t command;
    uint16_t id;
};

/**
 * @brief 
 */
struct kowhai_protocol_symbol_spec_t
{
    uint8_t count;
    union kowhai_symbol_t* array_;
};

/**
 * @brief 
 */
struct kowhai_protocol_data_payload_memory_spec_t
{
    uint16_t type;
    uint16_t offset;
    uint16_t size;
};

/**
 * @brief 
 */
struct kowhai_protocol_data_payload_spec_t
{
    struct kowhai_protocol_symbol_spec_t symbols;
    struct kowhai_protocol_data_payload_memory_spec_t memory;
};

/**
 * @brief 
 */
struct kowhai_protocol_descriptor_payload_spec_t
{
    uint16_t node_count;
    uint16_t offset;
    uint16_t size;
};

/**
 * @brief 
 */
struct kowhai_protocol_id_list_t
{
    uint16_t list_count;
    uint16_t offset;
    uint16_t size;
};

/**
 * @brief 
 */
struct kowhai_protocol_string_list_t
{
    uint16_t list_count;
    uint32_t list_total_size;
    uint16_t offset;
    uint16_t size;
};

/**
 * @brief 
 */
struct kowhai_protocol_function_details_t
{
    uint16_t tree_in_id;
    uint16_t tree_out_id;
};

/**
 * @brief 
 */
struct kowhai_protocol_function_call_t
{
    uint16_t offset;
    uint16_t size;
};

/**
 * @brief 
 */
struct kowhai_protocol_event_t
{
    uint16_t offset;
    uint16_t size;
};

/**
 * @brief 
 */
union kowhai_protocol_payload_spec_t
{
    uint32_t version;
    struct kowhai_protocol_id_list_t id_list;
    struct kowhai_protocol_data_payload_spec_t data;
    struct kowhai_protocol_descriptor_payload_spec_t descriptor;
    struct kowhai_protocol_function_details_t function_details;
    struct kowhai_protocol_function_call_t function_call;
    struct kowhai_protocol_event_t event;
    struct kowhai_protocol_string_list_t string_list;
};

/**
 * @brief 
 */
struct kowhai_protocol_payload_t
{
    union kowhai_protocol_payload_spec_t spec;
    void* buffer;
};

/**
 * @brief 
 */
struct kowhai_protocol_t
{
    struct kowhai_protocol_header_t header;
    struct kowhai_protocol_payload_t payload;
};

#pragma pack()

/**
 * @brief format protocol to request a operation on a given tree id
 * @param protocol, this is a kowhai_protocol_t struct used to make the request
 * @param id_, the id of the tree or function to address this operation to
 */
#define POPULATE_PROTOCOL_CMD(protocol, cmd, id_)                \
    {                                                            \
        protocol.header.command = cmd;                           \
        protocol.header.id = id_;                                \
    }

/**
 * @brief format protocol to request reading the tree list
 * @param protocol, this is a kowhai_protocol_t struct used to make the request
 */
#define POPULATE_PROTOCOL_GET_TREE_LIST(protocol)       \
    {                                                   \
        protocol.header.command = KOW_CMD_GET_TREE_LIST;\
        protocol.header.id = 0;                         \
    }

/**
 * @brief format protocol to request reading a nodes value
 * @param protocol, this is a kowhai_protocol_t struct used to make the request
 * @param tree_id_, the id of the tree to address this read to
 * @param symbol_count_ the number of symbols in the symbols_ path
 * @param symbols_ a collection of symbols to identify the node to read
 */
#define POPULATE_PROTOCOL_READ(protocol, cmd, tree_id_, symbol_count_, symbols_) \
    {                                                            \
        POPULATE_PROTOCOL_CMD(protocol, cmd, tree_id_);          \
        protocol.payload.spec.data.symbols.count = symbol_count_;\
        protocol.payload.spec.data.symbols.array_ = symbols_;    \
    }

/**
 * @brief format protocol to request writing a nodes value
 * @param protocol, this is a kowhai_protocol_t struct used to make the request
 * @param tree_id_, the id of the tree to address this write to
 * @param symbol_count_ the number of symbols in the symbols_ path
 * @param symbols_ a collection of symbols to identify the node to write
 */
#define POPULATE_PROTOCOL_WRITE(protocol, cmd, tree_id_, symbol_count_, symbols_, data_type, data_offset, data_size, buffer_) \
    {                                                            \
        POPULATE_PROTOCOL_READ(protocol, cmd, tree_id_, symbol_count_, symbols_);\
        protocol.payload.spec.data.memory.type = data_type;      \
        protocol.payload.spec.data.memory.offset = data_offset;  \
        protocol.payload.spec.data.memory.size = data_size;      \
        protocol.payload.buffer = buffer_;                       \
    }

/**
 * @brief format protocol to request reading the function list
 * @param protocol, this is a kowhai_protocol_t struct used to make the request
 */
#define POPULATE_PROTOCOL_GET_FUNCTION_LIST(protocol)        \
    {                                                        \
        protocol.header.command = KOW_CMD_GET_FUNCTION_LIST; \
        protocol.header.id = 0;                              \
    }

#define POPULATE_PROTOCOL_GET_FUNCTION_DETAILS(protocol, function_id)  \
    {                                                                  \
        protocol.header.command = KOW_CMD_GET_FUNCTION_DETAILS;        \
        protocol.header.id = function_id;                              \
    }

#define POPULATE_PROTOCOL_CALL_FUNCTION(protocol, function_id, data_offset, data_size, data)\
    {                                                                                       \
        protocol.header.command = KOW_CMD_CALL_FUNCTION;                                    \
        protocol.header.id = function_id;                                                   \
        protocol.payload.spec.function_call.offset = data_offset;                           \
        protocol.payload.spec.function_call.size = data_size;                               \
        protocol.payload.buffer = data;                                                     \
    }

#define POPULATE_PROTOCOL_CALL_FUNCTION_END(protocol, function_id, data_offset, data_size, data)\
    {                                                                                           \
        POPULATE_PROTOCOL_CALL_FUNCTION(protocol, function_id, data_offset, data_size, data);   \
        protocol.header.command = KOW_CMD_CALL_FUNCTION_END;                                    \
    }

#define POPULATE_PROTOCOL_GET_SYMBOL_LIST(protocol)        \
    {                                                      \
        protocol.header.command = KOW_CMD_GET_SYMBOL_LIST; \
        protocol.header.id = 0;                            \
    }

//
// Functions
//

/**
 * @brief Parse a packet read over the kowhai protocol in to a kowhai_protocol_t struct
 * @param proto_packet a packet read over the protocol that needs parsing
 * @param packet_size number of bytes in the proto_packet
 * @param protocol update this with the information in the proto_packet
 * @return KOW_STATUS_OK on success otherwise an error occurred
 */
int kowhai_protocol_parse(void* proto_packet, int packet_size, struct kowhai_protocol_t* protocol);

/**
 * @brief Create a new protocol packet used to communicate with another kowhai enabled program
 * @param proto_packet place the packet information into this buffer
 * @param packet_size bytes allocated for the proto_packet
 * @param protocol make the packet from the request info found in this structure
 * @param bytes_required on KOW_STATUS_OK this contains the actual numbers of bytes used in proto_packet 
 * @return KOW_STATUS_OK on success otherwise an error occurred
 */
int kowhai_protocol_create(void* proto_packet, int packet_size, struct kowhai_protocol_t* protocol, int* bytes_required);

/**
 * @brief Returkn the protocol overhead (header, payload specification etc, ie the meta part of the protocol that describes the payload)
 * @param protocol parse this for the overhead
 * @param overhead number of bytes taken up by the header, payload etc)
 */
int kowhai_protocol_get_overhead(struct kowhai_protocol_t* protocol, int* overhead);

#endif

