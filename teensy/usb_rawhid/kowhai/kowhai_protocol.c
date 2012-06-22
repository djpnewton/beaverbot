#include "kowhai_protocol.h"

#include <string.h>

#define SYM_COUNT_SIZE 1

static int parse_version(void* payload_packet, int packet_size, struct kowhai_protocol_payload_t* payload)
{
    // check packet is large enough for version integer
    int required_size = sizeof(uint32_t);
    if (packet_size < required_size)
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;

    // get version
    payload->spec.version = *((uint32_t*)payload_packet);
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

static int parse_id_list(void* payload_packet, int packet_size, struct kowhai_protocol_payload_t* payload)
{
    if (packet_size < sizeof(struct kowhai_protocol_id_list_t))
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
    memcpy(&payload->spec, payload_packet, sizeof(struct kowhai_protocol_id_list_t));
    if (payload->spec.id_list.size > packet_size - sizeof(struct kowhai_protocol_id_list_t))
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
    payload->buffer = (void*)((char*)payload_packet + sizeof(struct kowhai_protocol_id_list_t));
    return KOW_STATUS_OK;
}

static int parse_string_list(void* payload_packet, int packet_size, struct kowhai_protocol_payload_t* payload)
{
    if (packet_size < sizeof(struct kowhai_protocol_string_list_t))
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
    memcpy(&payload->spec, payload_packet, sizeof(struct kowhai_protocol_string_list_t));
    if (payload->spec.id_list.size > packet_size - sizeof(struct kowhai_protocol_string_list_t))
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
    payload->buffer = (void*)((char*)payload_packet + sizeof(struct kowhai_protocol_string_list_t));
    return KOW_STATUS_OK;
}

static int parse_function_details(void* payload_packet, int packet_size, struct kowhai_protocol_function_details_t* details)
{
    if (packet_size < sizeof(struct kowhai_protocol_function_details_t))
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
    memcpy(details, payload_packet, sizeof(struct kowhai_protocol_function_details_t));
    return KOW_STATUS_OK;
}

static int parse_function_call(void* payload_packet, int packet_size, struct kowhai_protocol_payload_t* payload)
{
    if (packet_size < sizeof(struct kowhai_protocol_function_call_t))
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
    memcpy(&payload->spec, payload_packet, sizeof(struct kowhai_protocol_function_call_t));
    if (payload->spec.function_call.size > packet_size - sizeof(struct kowhai_protocol_function_call_t))
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
    payload->buffer = (void*)((char*)payload_packet + sizeof(struct kowhai_protocol_function_call_t));
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
        case KOW_CMD_GET_VERSION:
            return KOW_STATUS_OK;
        case KOW_CMD_GET_VERSION_ACK:
            return parse_version((void*)((uint8_t*)proto_packet + required_size), packet_size - required_size, &protocol->payload);
        case KOW_CMD_GET_TREE_LIST:
            return KOW_STATUS_OK;
        case KOW_CMD_GET_TREE_LIST_ACK:
        case KOW_CMD_GET_TREE_LIST_ACK_END:
        case KOW_CMD_GET_FUNCTION_LIST_ACK:
        case KOW_CMD_GET_FUNCTION_LIST_ACK_END:
            return parse_id_list((void*)((uint8_t*)proto_packet + required_size), packet_size - required_size, &protocol->payload);
        case KOW_CMD_READ_DATA:
            return parse_symbols((void*)((uint8_t*)proto_packet + required_size), packet_size - required_size, &protocol->payload, &required_size);
        case KOW_CMD_WRITE_DATA:
        case KOW_CMD_WRITE_DATA_END:
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
        case KOW_CMD_GET_FUNCTION_LIST:
        case KOW_CMD_GET_FUNCTION_DETAILS:
            // get function list/details command requires no more parameters
            return KOW_STATUS_OK;
        case KOW_CMD_GET_FUNCTION_DETAILS_ACK:
            return parse_function_details((void*)((uint8_t*)proto_packet + required_size), packet_size - required_size, &protocol->payload.spec.function_details);
        case KOW_CMD_CALL_FUNCTION:
        case KOW_CMD_CALL_FUNCTION_ACK:
        case KOW_CMD_CALL_FUNCTION_RESULT:
        case KOW_CMD_CALL_FUNCTION_RESULT_END:
            return parse_function_call((void*)((uint8_t*)proto_packet + required_size), packet_size - required_size, &protocol->payload);
        case KOW_CMD_CALL_FUNCTION_FAILED:
            return KOW_STATUS_OK;
        case KOW_CMD_GET_SYMBOL_LIST:
            return KOW_STATUS_OK;
        case KOW_CMD_GET_SYMBOL_LIST_ACK:
        case KOW_CMD_GET_SYMBOL_LIST_ACK_END:
            return parse_string_list((void*)((uint8_t*)proto_packet + required_size), packet_size - required_size, &protocol->payload);

        default:
            return KOW_STATUS_INVALID_PROTOCOL_COMMAND;
    }
}

int kowhai_protocol_create(void* proto_packet, int packet_size, struct kowhai_protocol_t* protocol, int* bytes_required)
{
    char* pkt = (char*)proto_packet;

    // write protocol header
    *bytes_required = sizeof(struct kowhai_protocol_header_t);
    if (packet_size < *bytes_required)
        return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
    memcpy(pkt, &protocol->header, sizeof(struct kowhai_protocol_header_t));
    pkt += sizeof(struct kowhai_protocol_header_t);

    // check protocol command
    switch (protocol->header.command)
    {
        case KOW_CMD_GET_VERSION:
            break;
        case KOW_CMD_GET_VERSION_ACK:
            // write version
            *bytes_required += sizeof(uint32_t);
            if (packet_size < *bytes_required)
                return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
            *(uint32_t*)pkt = protocol->payload.spec.version;
            break;
        case KOW_CMD_GET_TREE_LIST:
            break;
        case KOW_CMD_GET_TREE_LIST_ACK:
        case KOW_CMD_GET_TREE_LIST_ACK_END:
        case KOW_CMD_GET_FUNCTION_LIST_ACK:
        case KOW_CMD_GET_FUNCTION_LIST_ACK_END:
            // write payload spec
            *bytes_required += sizeof(struct kowhai_protocol_id_list_t);
            if (packet_size < *bytes_required)
                return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
            memcpy(pkt, &protocol->payload.spec.id_list, sizeof(struct kowhai_protocol_id_list_t));
            pkt += sizeof(struct kowhai_protocol_id_list_t);
            // write payload
            *bytes_required += protocol->payload.spec.id_list.size;
            if (packet_size < *bytes_required)
                return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
            memcpy(pkt, protocol->payload.buffer, protocol->payload.spec.id_list.size);
            pkt += protocol->payload.spec.id_list.size;
            break;
        case KOW_CMD_WRITE_DATA:
        case KOW_CMD_WRITE_DATA_END:
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
            pkt += protocol->payload.spec.data.symbols.count * sizeof(union kowhai_symbol_t);
            // read data command requires no more parameters
            if (protocol->header.command == KOW_CMD_READ_DATA)
                return KOW_STATUS_OK;
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
        case KOW_CMD_READ_DESCRIPTOR:
            // read descriptor command requires no more parameters
            break;
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
        case KOW_CMD_GET_FUNCTION_LIST:
        case KOW_CMD_GET_FUNCTION_DETAILS:
            // get function list/details command requires no more parameters
            break;
        case KOW_CMD_GET_FUNCTION_DETAILS_ACK:
            // write details
            *bytes_required += sizeof(struct kowhai_protocol_function_details_t);
            if (packet_size < *bytes_required)
                return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
            memcpy(pkt, &protocol->payload.spec.function_details, sizeof(struct kowhai_protocol_function_details_t));
            break;
        case KOW_CMD_CALL_FUNCTION:
        case KOW_CMD_CALL_FUNCTION_ACK:
        case KOW_CMD_CALL_FUNCTION_RESULT:
        case KOW_CMD_CALL_FUNCTION_RESULT_END:
            // write payload spec
            *bytes_required += sizeof(struct kowhai_protocol_function_call_t);
            if (packet_size < *bytes_required)
                return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
            memcpy(pkt, &protocol->payload.spec.function_call, sizeof(struct kowhai_protocol_function_call_t));
            pkt += sizeof(struct kowhai_protocol_function_call_t);
            // write payload
            *bytes_required += protocol->payload.spec.function_call.size;
            if (packet_size < *bytes_required)
                return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
            memcpy(pkt, protocol->payload.buffer, protocol->payload.spec.function_call.size);
            break;
        case KOW_CMD_CALL_FUNCTION_FAILED:
            break;
        case KOW_CMD_GET_SYMBOL_LIST:
            break;
        case KOW_CMD_GET_SYMBOL_LIST_ACK:
        case KOW_CMD_GET_SYMBOL_LIST_ACK_END:
            // write payload spec
            *bytes_required += sizeof(struct kowhai_protocol_string_list_t);
            if (packet_size < *bytes_required)
                return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
            memcpy(pkt, &protocol->payload.spec.string_list, sizeof(struct kowhai_protocol_string_list_t));
            pkt += sizeof(struct kowhai_protocol_string_list_t);
            // write payload
            *bytes_required += protocol->payload.spec.string_list.size;
            if (packet_size < *bytes_required)
                return KOW_STATUS_PACKET_BUFFER_TOO_SMALL;
            memcpy(pkt, protocol->payload.buffer, protocol->payload.spec.string_list.size);
            pkt += protocol->payload.spec.string_list.size;
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
        case KOW_CMD_GET_TREE_LIST:
            *overhead = sizeof(struct kowhai_protocol_header_t);
            return KOW_STATUS_OK;
        case KOW_CMD_GET_TREE_LIST_ACK:
        case KOW_CMD_GET_TREE_LIST_ACK_END:
            *overhead = sizeof(struct kowhai_protocol_header_t) + sizeof(struct kowhai_protocol_id_list_t);
            return KOW_STATUS_OK;
        case KOW_CMD_READ_DESCRIPTOR:
            *overhead = sizeof(struct kowhai_protocol_header_t);
            return KOW_STATUS_OK;
        case KOW_CMD_READ_DESCRIPTOR_ACK:
        case KOW_CMD_READ_DESCRIPTOR_ACK_END:
            *overhead = sizeof(struct kowhai_protocol_header_t) + sizeof(struct kowhai_protocol_descriptor_payload_spec_t);
            return KOW_STATUS_OK;
        case KOW_CMD_WRITE_DATA:
        case KOW_CMD_WRITE_DATA_END:
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
        case KOW_CMD_GET_FUNCTION_LIST:
        case KOW_CMD_GET_FUNCTION_DETAILS:
            *overhead = sizeof(struct kowhai_protocol_header_t);
            return KOW_STATUS_OK;
        case KOW_CMD_GET_FUNCTION_LIST_ACK:
        case KOW_CMD_GET_FUNCTION_LIST_ACK_END:
            *overhead = sizeof(struct kowhai_protocol_header_t) + sizeof(struct kowhai_protocol_id_list_t);
            return KOW_STATUS_OK;
        case KOW_CMD_GET_FUNCTION_DETAILS_ACK:
            *overhead = sizeof(struct kowhai_protocol_header_t) + sizeof(struct kowhai_protocol_function_details_t);
            return KOW_STATUS_OK;
        case KOW_CMD_CALL_FUNCTION:
        case KOW_CMD_CALL_FUNCTION_ACK:
        case KOW_CMD_CALL_FUNCTION_RESULT:
        case KOW_CMD_CALL_FUNCTION_RESULT_END:
            *overhead = sizeof(struct kowhai_protocol_header_t) + sizeof(struct kowhai_protocol_function_call_t);
            return KOW_STATUS_OK;
        case KOW_CMD_GET_SYMBOL_LIST:
            *overhead = sizeof(struct kowhai_protocol_header_t);
            return KOW_STATUS_OK;
        case KOW_CMD_GET_SYMBOL_LIST_ACK:
        case KOW_CMD_GET_SYMBOL_LIST_ACK_END:
            *overhead = sizeof(struct kowhai_protocol_header_t) + sizeof(struct kowhai_protocol_string_list_t);
            return KOW_STATUS_OK;
        default:
            return KOW_STATUS_INVALID_PROTOCOL_COMMAND;
    }
}
