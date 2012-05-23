#include "kowhai_protocol.h"

#include <stdio.h>
#include <string.h>

#define TREE_ID_SIZE 1
#define CMD_SIZE 1
#define SYM_COUNT_SIZE 1

int kowhai_protocol_get_tree_id(void* proto_packet, int packet_size, uint8_t* tree_id)
{
    if (packet_size < TREE_ID_SIZE)
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;

    // establish tree id
    *tree_id = *((uint8_t*)proto_packet);

    return KOW_STATUS_OK;
}

/**
 * @brief Parse the symbols from a read packet into a formatted kowhai_protocol_payload_t structure
 * @param payload_packet a packet read over the protocol that needs the symbols parsed out of 
 * @param packet_size number of bytes in the payload_packet
 * @param payload parse the payload_packet and place the symbol path into this
 * @param symbols_size number of bytes needed to store the symbol path ?dan please check this?
 * @return KOW_STATUS_OK on success otherwise a KOW_STATUS error code
 */
static int parse_symbols(void* payload_packet, int packet_size, struct kowhai_protocol_payload_t* payload, int* symbols_size)
{
    // check packet is large enough for symbol count byte
    int required_size = sizeof(payload->spec.data.symbols.count);
    if (packet_size < required_size)
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;

    // get symbol count
    payload->spec.data.symbols.count = *((uint8_t*)payload_packet);
    required_size += sizeof(union kowhai_symbol_t) * payload->spec.data.symbols.count;

    // check packet is large enough for symbols array
    if (packet_size < required_size)
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;

    // get symbol array
    payload->spec.data.symbols.array_ = (union kowhai_symbol_t*)((uint8_t*)payload_packet + sizeof(payload->spec.data.symbols.count));

    // return OK
    *symbols_size = required_size;
    return KOW_STATUS_OK;
}

/**
 * @brief Parse a read/write request packet read over the kowhai protocol into a kowhai_protocol_payload_t structure
 * @param payload_packet a packet that needs parsing
 * @param packet_size number of bytes in the payload_packet
 * @param payload parse the payload_packet into the data and buffer sections of this structure
 * @return KOW_STATUS_OK on success otherwise a KOW_STATUS error code
 */
static int parse_data_payload(void* payload_packet, int packet_size, struct kowhai_protocol_payload_t* payload)
{
    // parse symbols
    int required_size;
    int status = parse_symbols(payload_packet, packet_size, payload, &required_size);
    if (status != KOW_STATUS_OK)
        return status;

    // check packet is large enough for the rest of the payload spec
    required_size += sizeof(struct kowhai_protocol_data_payload_memory_spec_t);
    if (packet_size < required_size)
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;

    // copy the rest of the payload spec
    memcpy(&payload->spec.data.memory,
        (uint8_t*)payload_packet + sizeof(payload->spec.data.symbols.count) + sizeof(union kowhai_symbol_t) * payload->spec.data.symbols.count,
        sizeof(struct kowhai_protocol_data_payload_memory_spec_t));

    // check the packet is large enough to hold the payload buffer
    if (payload->spec.data.memory.size > packet_size - required_size)
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;

    // set payload buffer pointer
    payload->buffer = (void*)((char*)payload_packet + sizeof(payload->spec.data.symbols.count) + sizeof(union kowhai_symbol_t) * payload->spec.data.symbols.count + sizeof(struct kowhai_protocol_data_payload_memory_spec_t));

    return KOW_STATUS_OK;
}

/**
 * @brief Parse a descriptor request packet read over the kowhai protocol into a kowhai_protocol_payload_t structure
 * @param payload_packet a packet that needs parsing
 * @param packet_size number of bytes in the payload_packet
 * @param payload parse the payload_packet into the descriptor sections of this structure
 * @return KOW_STATUS_OK on success otherwise a KOW_STATUS error code
 */
static int parse_descriptor_payload(void* payload_packet, int packet_size, struct kowhai_protocol_payload_t* payload)
{
    if (packet_size < sizeof(struct kowhai_protocol_descriptor_payload_spec_t))
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
    memcpy(&payload->spec, payload_packet, sizeof(struct kowhai_protocol_descriptor_payload_spec_t));
    if (payload->spec.descriptor.size > packet_size - sizeof(struct kowhai_protocol_descriptor_payload_spec_t))
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
    payload->buffer = (void*)((char*)payload_packet + sizeof(struct kowhai_protocol_descriptor_payload_spec_t));
    return KOW_STATUS_OK;
}

int kowhai_protocol_parse(void* proto_packet, int packet_size, struct kowhai_protocol_t* protocol)
{
    int required_size = sizeof(struct kowhai_protocol_header_t);

    memset(protocol, 0, sizeof(struct kowhai_protocol_t));

    // check packet is large enough for header
    if (packet_size < required_size)
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
    memcpy(&protocol->header, proto_packet, required_size);

    switch (protocol->header.command)
    {
        case KOW_CMD_READ_DATA:
            return parse_symbols((void*)((uint8_t*)proto_packet + required_size), packet_size - required_size, &protocol->payload, &required_size);
        case KOW_CMD_WRITE_DATA:
        case KOW_CMD_WRITE_DATA_ACK:
        case KOW_CMD_READ_DATA_ACK:
        case KOW_CMD_READ_DATA_ACK_END:
            return parse_data_payload((void*)((uint8_t*)proto_packet + required_size), packet_size - required_size, &protocol->payload);
        case KOW_CMD_READ_DESCRIPTOR:
            // read descriptor command requires no more parameters
            return KOW_STATUS_OK;
        case KOW_CMD_READ_DESCRIPTOR_ACK:
        case KOW_CMD_READ_DESCRIPTOR_ACK_END:
            return parse_descriptor_payload((void*)((uint8_t*)proto_packet + required_size), packet_size - required_size, &protocol->payload);
        default:
            return KOW_STATUS_INVALID_PROTOCOL_COMMAND;
    }
}

int kowhai_protocol_create(void* proto_packet, int packet_size, struct kowhai_protocol_t* protocol, int* bytes_required)
{
    char* pkt = (char*)proto_packet;

    // write tree id
    *bytes_required = TREE_ID_SIZE;
    if (packet_size < *bytes_required)
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
    *pkt = protocol->header.tree_id;
    pkt += TREE_ID_SIZE;

    // write protocol command
    *bytes_required += CMD_SIZE;
    if (packet_size < *bytes_required)
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
    *pkt = protocol->header.command;
    pkt += CMD_SIZE;

    // read descriptor command requires no more parameters
    if (protocol->header.command == KOW_CMD_READ_DESCRIPTOR)
        return KOW_STATUS_OK;

    // check protocol command
    switch (protocol->header.command)
    {
        case KOW_CMD_WRITE_DATA:
        case KOW_CMD_WRITE_DATA_ACK:
        case KOW_CMD_READ_DATA_ACK:
        case KOW_CMD_READ_DATA:
        case KOW_CMD_READ_DATA_ACK_END:
            // write symbol count
            *bytes_required += SYM_COUNT_SIZE;
            if (packet_size < *bytes_required)
                return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
            *pkt = protocol->payload.spec.data.symbols.count;
            pkt += SYM_COUNT_SIZE;

            // write symbols
            *bytes_required += protocol->payload.spec.data.symbols.count * sizeof(union kowhai_symbol_t);
            if (packet_size < *bytes_required)
                return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
            memcpy(pkt, protocol->payload.spec.data.symbols.array_, protocol->payload.spec.data.symbols.count * sizeof(union kowhai_symbol_t));
            pkt += protocol->payload.spec.data.symbols.count * sizeof(union kowhai_symbol_t);;

            break;
    }

    // read data command requires no more parameters
    if (protocol->header.command == KOW_CMD_READ_DATA)
        return KOW_STATUS_OK;

    // check protocol command
    switch (protocol->header.command)
    {
        case KOW_CMD_READ_DESCRIPTOR_ACK:
        case KOW_CMD_READ_DESCRIPTOR_ACK_END:
            // write payload spec
            *bytes_required += sizeof(struct kowhai_protocol_descriptor_payload_spec_t);
            if (packet_size < *bytes_required)
                return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
            memcpy(pkt, &protocol->payload.spec.descriptor, sizeof(struct kowhai_protocol_descriptor_payload_spec_t));
            pkt += sizeof(struct kowhai_protocol_descriptor_payload_spec_t);

            // write payload
            *bytes_required += protocol->payload.spec.descriptor.size;
            if (packet_size < *bytes_required)
                return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
            memcpy(pkt, protocol->payload.buffer, protocol->payload.spec.descriptor.size);

            break;

        case KOW_CMD_WRITE_DATA:
        case KOW_CMD_WRITE_DATA_ACK:
        case KOW_CMD_READ_DATA_ACK:
        case KOW_CMD_READ_DATA_ACK_END:
            // write payload spec
            *bytes_required += sizeof(struct kowhai_protocol_data_payload_memory_spec_t);
            if (packet_size < *bytes_required)
                return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
            memcpy(pkt, &protocol->payload.spec.data.memory, sizeof(struct kowhai_protocol_data_payload_memory_spec_t));
            pkt += sizeof(struct kowhai_protocol_data_payload_memory_spec_t);

            // write payload
            *bytes_required += protocol->payload.spec.data.memory.size;
            if (packet_size < *bytes_required)
                return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
            memcpy(pkt, protocol->payload.buffer, protocol->payload.spec.data.memory.size);
            break;

        default:
            return KOW_STATUS_INVALID_PROTOCOL_COMMAND;
    }

    return KOW_STATUS_OK;
}

int kowhai_protocol_get_overhead(struct kowhai_protocol_t* protocol, int* overhead)
{
    // check protocol command
    switch (protocol->header.command)
    {
        case KOW_CMD_READ_DESCRIPTOR:
            *overhead = sizeof(struct kowhai_protocol_header_t);
            return KOW_STATUS_OK;
        case KOW_CMD_READ_DESCRIPTOR_ACK:
        case KOW_CMD_READ_DESCRIPTOR_ACK_END:
            *overhead = sizeof(struct kowhai_protocol_header_t) + sizeof(struct kowhai_protocol_descriptor_payload_spec_t);
            return KOW_STATUS_OK;
        case KOW_CMD_WRITE_DATA:
        case KOW_CMD_WRITE_DATA_ACK:
        case KOW_CMD_READ_DATA_ACK:
        case KOW_CMD_READ_DATA_ACK_END:
            *overhead = sizeof(struct kowhai_protocol_t) - sizeof(protocol->payload.spec.data.symbols.array_) +
                sizeof(union kowhai_symbol_t) * protocol->payload.spec.data.symbols.count -
                sizeof(protocol->payload.buffer);
            return KOW_STATUS_OK;
        case KOW_CMD_READ_DATA:
            *overhead = sizeof(struct kowhai_protocol_header_t) + sizeof(protocol->payload.spec.data.symbols.count) +
                sizeof(union kowhai_symbol_t) * protocol->payload.spec.data.symbols.count;
            return KOW_STATUS_OK;
        default:
            return KOW_STATUS_INVALID_PROTOCOL_COMMAND;
    }
}
