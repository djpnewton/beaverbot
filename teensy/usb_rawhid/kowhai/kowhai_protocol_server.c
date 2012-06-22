#include "kowhai_protocol_server.h"

#include <stdlib.h>
#include <string.h>

void kowhai_server_init_tree_descriptor_sizes(struct kowhai_node_t** descriptors, size_t* sizes, int num)
{
    int i;
    for (i = 0; i < num; i++)
    {
        struct kowhai_node_t* desc = descriptors[i];
        sizes[i] = 0;
        if (desc != NULL)
        {
            int c = 0;
            do
            {
                sizes[i] += sizeof(struct kowhai_node_t);
                if (desc->type == KOW_BRANCH_END)
                    c--;
                else if (desc->type == KOW_BRANCH_START)
                    c++;
                desc++;
            }
            while (c > 0);
        }
    }
}

void kowhai_server_init(struct kowhai_protocol_server_t* server,
    size_t max_packet_size,
    void* packet_buffer,
    kowhai_node_pre_write_t node_pre_write,
    kowhai_node_post_write_t node_post_write,
    void* node_write_param,
    kowhai_send_packet_t send_packet,
    void* send_packet_param,
    int tree_list_count,
    uint16_t* tree_list,
    struct kowhai_node_t** tree_descriptors,
    size_t* tree_descriptor_sizes,
    void** tree_data_buffers,
    int function_list_count,
    uint16_t* function_list,
    struct kowhai_protocol_function_details_t* function_list_details,
    kowhai_function_called_t function_called,
    void* function_called_param,
    int symbol_list_count,
    char** symbol_list)
{
    server->max_packet_size = max_packet_size;
    server->packet_buffer = packet_buffer;
    server->node_pre_write = node_pre_write;
    server->node_post_write = node_post_write;
    server->node_write_param = node_write_param;
    server->send_packet = send_packet;
    server->send_packet_param = send_packet_param;
    server->tree_list_count = tree_list_count;
    server->tree_list = tree_list;
    server->tree_descriptors = tree_descriptors;
    server->tree_descriptor_sizes = tree_descriptor_sizes;
    server->tree_data_buffers = tree_data_buffers;
    server->function_list_count = function_list_count;
    server->function_list = function_list;
    server->function_list_details = function_list_details;
    server->function_called = function_called;
    server->symbol_list_count = symbol_list_count;
    server->symbol_list = symbol_list;

    server->current_write_node = NULL;
}

int _get_tree_index(struct kowhai_protocol_server_t* server , uint16_t id, int* index)
{
    int i = 0;
    while (i < server->tree_list_count)
    {
        if (server->tree_list[i] == id)
        {
            *index = i;
            return 1;
        }
        i++;
    }
    return 0;
}

struct kowhai_tree_t _populate_tree(struct kowhai_protocol_server_t* server, uint16_t tree_id)
{
    struct kowhai_tree_t tree = {NULL, NULL};
    int index;
    if (tree_id != KOW_UNDEFINED_SYMBOL &&
        _get_tree_index(server, tree_id, &index))
    {

        tree.desc = server->tree_descriptors[index];
        tree.data = server->tree_data_buffers[index];
    }
    return tree;
}

int _check_tree_id(struct kowhai_protocol_server_t* server, uint16_t id)
{
    int i;
    for (i = 0; i < server->tree_list_count; i++)
    {
        if (server->tree_list[i] == id)
            return 1;
    }
    return 0;
}

void _invalid_tree_id(struct kowhai_protocol_server_t* server, struct kowhai_protocol_t* prot)
{
    int bytes_required;
    KOW_LOG("    invalid tree id (%d)\n", prot->header.id);
    prot->header.command = KOW_CMD_ERROR_INVALID_TREE_ID;
    kowhai_protocol_create(server->packet_buffer, server->max_packet_size, prot, &bytes_required);
    server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
}

int _get_function_index(struct kowhai_protocol_server_t* server , uint16_t id, int* index)
{
    int i = 0;
    while (i < server->function_list_count)
    {
        if (server->function_list[i] == id)
        {
            *index = i;
            return 1;
        }
        i++;
    }
    return 0;
}

void _send_id_list(struct kowhai_protocol_server_t* server, struct kowhai_protocol_t* prot,
                    uint8_t cmd_ack, uint8_t cmd_ack_end,
                    int id_list_count, uint16_t* id_list)
{
    int bytes_required;
    int overhead, max_payload_size;
    int size = id_list_count * sizeof(uint16_t);
    // get protocol overhead
    prot->header.command = cmd_ack;
    kowhai_protocol_get_overhead(prot, &overhead);
    // setup max payload size and payload offset
    max_payload_size = server->max_packet_size - overhead;
    prot->payload.spec.id_list.offset = 0;
    prot->payload.spec.id_list.list_count = id_list_count;
    // send packets
    while (size > max_payload_size)
    {
        prot->payload.spec.id_list.size = max_payload_size;
        prot->payload.buffer = (char*)id_list + prot->payload.spec.id_list.offset;
        kowhai_protocol_create(server->packet_buffer, server->max_packet_size, prot, &bytes_required);
        server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
        // increment payload offset and decrement remaining payload size
        prot->payload.spec.id_list.offset += max_payload_size;
        size -= max_payload_size;
    }
    // send final packet
    prot->header.command = cmd_ack_end;
    prot->payload.spec.id_list.size = size;
    prot->payload.buffer = (char*)id_list + prot->payload.spec.id_list.offset;
    kowhai_protocol_create(server->packet_buffer, server->max_packet_size, prot, &bytes_required);
    server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
}

size_t _get_string_list_size(char** list, int count)
{
    int i;
    size_t total = 0;
    for (i = 0; i < count; i++)
        total += strlen(list[i]) + 1;
    return total;
}

void _copy_string_list_to_buffer(char** string_list, int count, int offset, void* buffer, int buffer_size)
{
    int i, marker = 0;
    for (i = 0; i < count; i++)
    {
        int l = strlen(string_list[i]) + 1;
        if (marker + l >= offset)
        {
            int string_offset, copy_size;
            string_offset = marker - offset;
            if (string_offset < 0)
                string_offset = -string_offset;
            copy_size = l - string_offset;
            if (copy_size > buffer_size)
                copy_size = buffer_size;
            memcpy(buffer, string_list[i] + string_offset, copy_size);
            offset += copy_size;
            buffer = (char*)buffer + copy_size;
            buffer_size -= copy_size;
        }
        if (buffer_size == 0)
            break;
        marker += l;
    }
}

void _send_string_list(struct kowhai_protocol_server_t* server, struct kowhai_protocol_t* prot,
                    uint8_t cmd_ack, uint8_t cmd_ack_end,
                    int string_list_count, char** string_list)
{
    int bytes_required;
    int overhead, max_payload_size;
    int size = _get_string_list_size(string_list, string_list_count);
    // get protocol overhead
    prot->header.command = cmd_ack;
    kowhai_protocol_get_overhead(prot, &overhead);
    // setup max payload size and payload offset
    max_payload_size = server->max_packet_size - overhead;
    prot->payload.spec.string_list.offset = 0;
    prot->payload.spec.string_list.list_count = string_list_count;
    prot->payload.spec.string_list.list_total_size = size;
    prot->payload.buffer = (char*)server->packet_buffer + overhead;
    // send packets
    while (size > max_payload_size)
    {
        prot->payload.spec.string_list.size = max_payload_size;
        _copy_string_list_to_buffer(string_list, string_list_count, prot->payload.spec.string_list.offset, prot->payload.buffer, max_payload_size);
        kowhai_protocol_create(server->packet_buffer, server->max_packet_size, prot, &bytes_required);
        server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
        // increment payload offset and decrement remaining payload size
        prot->payload.spec.string_list.offset += max_payload_size;
        size -= max_payload_size;
    }
    // send final packet
    prot->header.command = cmd_ack_end;
    prot->payload.spec.string_list.size = size;
    _copy_string_list_to_buffer(string_list, string_list_count, prot->payload.spec.string_list.offset, prot->payload.buffer, max_payload_size);
    kowhai_protocol_create(server->packet_buffer, server->max_packet_size, prot, &bytes_required);
    server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
}

void _set_error_cmd(struct kowhai_protocol_t* prot, int status)
{
    switch (status)
    {
        case KOW_STATUS_INVALID_SYMBOL_PATH:
            KOW_LOG("    invalid symbol path\n");
            prot->header.command = KOW_CMD_ERROR_INVALID_SYMBOL_PATH;
            break;
        case KOW_STATUS_INVALID_OFFSET:
            KOW_LOG("    invalid payload offset\n");
            prot->header.command = KOW_CMD_ERROR_INVALID_PAYLOAD_OFFSET;
            break;
        case KOW_STATUS_NODE_DATA_TOO_SMALL:
            KOW_LOG("    invalid payload size\n");
            prot->header.command = KOW_CMD_ERROR_INVALID_PAYLOAD_SIZE;
            break;
        case KOW_STATUS_INVALID_SEQUENCE:
            KOW_LOG("    invalid sequence\n");
            prot->header.command = KOW_CMD_ERROR_INVALID_SEQUENCE;
            break;
        default:
            KOW_LOG("    unknown error\n");
            prot->header.command = KOW_CMD_ERROR_UNKNOWN;
            break;
    }
}

int kowhai_server_process_packet(struct kowhai_protocol_server_t* server, void* packet, size_t packet_size)
{
    struct kowhai_protocol_t prot;
    int bytes_required, status;

    if (packet_size > server->max_packet_size)
    {
        KOW_LOG("    error: packet size too large\n");
        return KOW_STATUS_PACKET_BUFFER_TOO_BIG;
    }

    status = kowhai_protocol_parse(packet, packet_size, &prot);
    if (status != KOW_STATUS_OK && status != KOW_STATUS_INVALID_PROTOCOL_COMMAND)
    {
        KOW_LOG("    ERROR: invalid protocol command\n");
        prot.header.command = KOW_CMD_ERROR_INVALID_COMMAND;
        kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
        server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
        return status;
    }

    switch (prot.header.command)
    {
        case KOW_CMD_GET_VERSION:
            KOW_LOG("    CMD get version\n");
            prot.header.command = KOW_CMD_GET_VERSION_ACK;
            prot.payload.spec.version = kowhai_version();
            kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
            server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
            break;
        case KOW_CMD_GET_TREE_LIST:
        case KOW_CMD_GET_TREE_LIST_ACK_END:
            _send_id_list(server, &prot,
                KOW_CMD_GET_TREE_LIST_ACK, KOW_CMD_GET_TREE_LIST_ACK_END,
                server->tree_list_count, server->tree_list);
            break;
        case KOW_CMD_WRITE_DATA:
        case KOW_CMD_WRITE_DATA_END:
        {
            struct kowhai_tree_t tree;
            int offset;
            struct kowhai_node_t* node_to_write;
            KOW_LOG("    CMD write data\n");
            if (!_check_tree_id(server, prot.header.id))
            {
                _invalid_tree_id(server, &prot);
                break;
            }
            // init tree helper struct
            tree = _populate_tree(server, prot.header.id);
            // check/set current write node
            status = kowhai_get_node(tree.desc, prot.payload.spec.data.symbols.count, prot.payload.spec.data.symbols.array_, &offset, &node_to_write);
            if (status == KOW_STATUS_OK)
            {
                if (server->current_write_node != NULL)
                {
                    if (node_to_write != server->current_write_node)
                        // current_write_node *should* match node_to_write
                        status = KOW_STATUS_INVALID_SEQUENCE;
                }
                else
                {
                    // set current write node
                    server->current_write_node = node_to_write;
                    server->current_write_node_offset = offset;
                    server->current_write_node_bytes_written = 0;
                    // call node_pre_write callback
                    server->node_pre_write(server, server->node_write_param, prot.header.id, server->current_write_node, server->current_write_node_offset);
                }
            }
            // write to tree
            if (status == KOW_STATUS_OK)
            {
                //TODO: user kowhai_get_node result in kowhai_write(2?)
                status = kowhai_write(&tree, prot.payload.spec.data.symbols.count, prot.payload.spec.data.symbols.array_, prot.payload.spec.data.memory.offset, prot.payload.buffer, prot.payload.spec.data.memory.size);
                if (status == KOW_STATUS_OK)
                {
                    // update current_write_node_bytes_written
                    int bytes_written = prot.payload.spec.data.memory.offset + prot.payload.spec.data.memory.size;
                    if (bytes_written > server->current_write_node_bytes_written)
                        server->current_write_node_bytes_written = bytes_written;
                    // call node_post_write callback
                    if (prot.header.command == KOW_CMD_WRITE_DATA_END)
                    {
                        server->node_post_write(server, server->node_write_param, prot.header.id, server->current_write_node, server->current_write_node_offset, server->current_write_node_bytes_written);
                        // clear current write node if at end of write sequence
                        server->current_write_node = NULL;
                    }
                    // send response
                    prot.header.command = KOW_CMD_WRITE_DATA_ACK;
                    kowhai_read(&tree, prot.payload.spec.data.symbols.count, prot.payload.spec.data.symbols.array_, prot.payload.spec.data.memory.offset, prot.payload.buffer, prot.payload.spec.data.memory.size);
                    kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                    server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
                    break;
                }
            }
            // clear current write node if error encountered
            server->current_write_node = NULL;
            // send error response
            _set_error_cmd(&prot, status);
            kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
            server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
            break;
        }
        case KOW_CMD_READ_DATA:
        {
            struct kowhai_tree_t tree;
            int node_offset;
            int size, overhead, max_payload_size;
            struct kowhai_node_t* node;
            struct kowhai_protocol_symbol_spec_t symbols = prot.payload.spec.data.symbols;
            KOW_LOG("    CMD read data\n");
            if (!_check_tree_id(server, prot.header.id))
            {
                _invalid_tree_id(server, &prot);
                break;
            }
            // init tree helper struct
            tree = _populate_tree(server, prot.header.id);
            // get node information
            status = kowhai_get_node(tree.desc, prot.payload.spec.data.symbols.count, prot.payload.spec.data.symbols.array_, &node_offset, &node);
            if (status == KOW_STATUS_OK)
            {
                union kowhai_symbol_t last_sym = symbols.array_[symbols.count-1];
                kowhai_get_node_size(node, &size);
                if (node->count > 1)
                    size = size - size / node->count * last_sym.parts.array_index;
                // get protocol overhead
                prot.header.command = KOW_CMD_READ_DATA_ACK;
                kowhai_protocol_get_overhead(&prot, &overhead);
                // setup max payload size and payload offset
                max_payload_size = server->max_packet_size - overhead;
                prot.payload.spec.data.memory.offset = 0;
                prot.payload.spec.data.memory.type = node->type;
                // set payload buffer pointer
                // (this will make a part of the kowhai_protocol_create call redundant
                // but we do not need to allocate any memory at least)
                prot.payload.buffer = (char*)server->packet_buffer + overhead;
                // send packets
                while (size > max_payload_size)
                {
                    prot.payload.spec.data.memory.size = max_payload_size;
                    kowhai_read(&tree, prot.payload.spec.data.symbols.count, prot.payload.spec.data.symbols.array_, prot.payload.spec.data.memory.offset, prot.payload.buffer, prot.payload.spec.data.memory.size);
                    kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                    server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
                    // increment payload offset and decrement remaining payload size
                    prot.payload.spec.data.memory.offset += max_payload_size;
                    size -= max_payload_size;
                }
                // send final packet
                prot.header.command = KOW_CMD_READ_DATA_ACK_END;
                prot.payload.spec.data.memory.size = size;
                kowhai_read(&tree, prot.payload.spec.data.symbols.count, prot.payload.spec.data.symbols.array_, prot.payload.spec.data.memory.offset, prot.payload.buffer, prot.payload.spec.data.memory.size);
                kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
            }
            else
            {
                _set_error_cmd(&prot, status);
                kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
            }
            break;
        }
        case KOW_CMD_READ_DESCRIPTOR:
        {
            struct kowhai_tree_t tree;
            int size, overhead, max_payload_size, index;
            KOW_LOG("    CMD read descriptor\n");
            if (!_check_tree_id(server, prot.header.id))
            {
                _invalid_tree_id(server, &prot);
                break;
            }
            // init tree helper struct
            tree = _populate_tree(server, prot.header.id);
            // get descriptor size
            _get_tree_index(server, prot.header.id, &index);
            size = *(server->tree_descriptor_sizes + index);
            // get protocol overhead
            prot.header.command = KOW_CMD_READ_DESCRIPTOR_ACK;
            kowhai_protocol_get_overhead(&prot, &overhead);
            // setup max payload size and payload offset
            max_payload_size = server->max_packet_size - overhead;
            prot.payload.spec.descriptor.offset = 0;
            prot.payload.spec.descriptor.node_count = size / sizeof(struct kowhai_node_t);
            // send packets
            while (size > max_payload_size)
            {
                prot.payload.spec.descriptor.size = max_payload_size;
                prot.payload.buffer = (char*)tree.desc + prot.payload.spec.descriptor.offset;
                kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
                // increment payload offset and decrement remaining payload size
                prot.payload.spec.descriptor.offset += max_payload_size;
                size -= max_payload_size;
            }
            // send final packet
            prot.header.command = KOW_CMD_READ_DESCRIPTOR_ACK_END;
            prot.payload.spec.descriptor.size = size;
            prot.payload.buffer = (char*)tree.desc + prot.payload.spec.descriptor.offset;
            kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
            server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
            break;
        }
        case KOW_CMD_GET_FUNCTION_LIST:
        {
            KOW_LOG("    CMD get function list\n");
            _send_id_list(server, &prot,
                KOW_CMD_GET_FUNCTION_LIST_ACK, KOW_CMD_GET_FUNCTION_LIST_ACK_END,
                server->function_list_count, server->function_list);
            break;
        }
        case KOW_CMD_GET_FUNCTION_DETAILS:
        {
            int i;
            KOW_LOG("    CMD get function details\n");
            // setup function details
            prot.header.command = KOW_CMD_ERROR_INVALID_FUNCTION_ID;
            if (_get_function_index(server, prot.header.id, &i))
            {
                prot.header.command = KOW_CMD_GET_FUNCTION_DETAILS_ACK;
                prot.payload.spec.function_details = server->function_list_details[i];
            }
            // set payload buffer
            prot.payload.buffer = NULL;

            // send packet
            kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
            server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
            break;
        }
        case KOW_CMD_CALL_FUNCTION:
        {
            int function_index, tree_data_size;
            KOW_LOG("    CMD call function\n");
            // call function
            prot.header.command = KOW_CMD_ERROR_INVALID_FUNCTION_ID;
            if (_get_function_index(server, prot.header.id, &function_index))
            {
                struct kowhai_tree_t tree = _populate_tree(server, server->function_list_details[function_index].tree_in_id);
                if (server->function_list_details[function_index].tree_in_id != KOW_UNDEFINED_SYMBOL &&
                    !_check_tree_id(server, server->function_list_details[function_index].tree_in_id))
                {
                    _invalid_tree_id(server, &prot);
                    break;
                }
                tree_data_size = 0;
                if (tree.desc == NULL || kowhai_get_node_size(tree.desc, &tree_data_size) == KOW_STATUS_OK)
                {
                    if (prot.payload.spec.function_call.offset > tree_data_size)
                    {
                        KOW_LOG("        KOW_CMD_ERROR_INVALID_PAYLOAD_OFFSET\n");
                        prot.header.command = KOW_CMD_ERROR_INVALID_PAYLOAD_OFFSET;
                    }
                    else if (prot.payload.spec.function_call.offset + prot.payload.spec.function_call.size > tree_data_size)
                    {
                        KOW_LOG("        KOW_CMD_ERROR_INVALID_PAYLOAD_SIZE\n");
                        prot.header.command = KOW_CMD_ERROR_INVALID_PAYLOAD_SIZE;
                    }
                    else
                    {
                        int offset = prot.payload.spec.function_call.offset;
                        int size = prot.payload.spec.function_call.size;
                        KOW_LOG("        write data (offset: %d, size: %d, tree_data_size: %d)\n", offset, size, tree_data_size);
                        memcpy((char*)tree.data + offset, prot.payload.buffer, size);
                        // setup response details
                        prot.header.command = KOW_CMD_CALL_FUNCTION_ACK;
                        prot.payload.spec.function_call.offset = 0;
                        prot.payload.spec.function_call.size = 0;
                        prot.payload.buffer = NULL;
                        // handle server->function_called when all data has been written
                        if (tree_data_size == 0 || offset + size == tree_data_size)
                        {
                            struct kowhai_tree_t tree = _populate_tree(server, server->function_list_details[function_index].tree_out_id);
                            KOW_LOG("        function_called callback\n");
                            if (server->function_called(server, server->function_called_param, prot.header.id))
                            {
                                // respond with result tree or not
                                if (tree.desc != NULL)
                                {
                                    int size, overhead, max_payload_size;
                                    KOW_LOG("        send return tree\n");
                                    prot.header.command = KOW_CMD_CALL_FUNCTION_RESULT;
                                    kowhai_protocol_get_overhead(&prot, &overhead);
                                    // setup size
                                    kowhai_get_node_size(tree.desc, &size);
                                    // setup max payload size and payload offset
                                    max_payload_size = server->max_packet_size - overhead;
                                    prot.payload.spec.function_call.offset = 0;
                                    prot.payload.spec.function_call.size = max_payload_size;
                                    prot.payload.buffer = tree.data;
                                    // send packets
                                    while (size > max_payload_size)
                                    {
                                        prot.payload.spec.function_call.size = max_payload_size;
                                        prot.payload.buffer = (char*)tree.data + prot.payload.spec.function_call.offset;
                                        kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                                        server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
                                        // increment payload offset and decrement remaining payload size
                                        prot.payload.spec.function_call.offset += max_payload_size;
                                        size -= max_payload_size;
                                    }
                                    // send final packet
                                    prot.header.command = KOW_CMD_CALL_FUNCTION_RESULT_END;
                                    prot.payload.spec.function_call.size = size;
                                    prot.payload.buffer = (char*)tree.data + prot.payload.spec.function_call.offset;
                                    kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                                    server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
                                    break;
                                }
                                else
                                {
                                    KOW_LOG("        send no return tree\n");
                                    prot.header.command = KOW_CMD_CALL_FUNCTION_RESULT_END;
                                }
                            }
                            else
                            {
                                KOW_LOG("        function call failed\n");
                                prot.header.command = KOW_CMD_CALL_FUNCTION_FAILED;
                            }
                        }
                        else
                        {
                            KOW_LOG("        send partial function call acknowledge\n");
                        }
                    }
                }
                else
                {
                    _invalid_tree_id(server, &prot);
                    break;
                }
            }
            else
            {
                KOW_LOG("        cant find function index\n");
            }
            // send packet
            kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
            server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
            break;
        }
        case KOW_CMD_GET_SYMBOL_LIST:
        {
            KOW_LOG("    CMD get symbol list\n");
            _send_string_list(server, &prot,
                KOW_CMD_GET_SYMBOL_LIST_ACK, KOW_CMD_GET_SYMBOL_LIST_ACK_END,
                server->symbol_list_count, server->symbol_list);
            break;
        }
        default:
            KOW_LOG("    invalid command (%d)\n", prot.header.command);
            POPULATE_PROTOCOL_CMD(prot, KOW_CMD_ERROR_INVALID_COMMAND, prot.header.id);
            kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
            server->send_packet(server, server->send_packet_param, server->packet_buffer, bytes_required);
            break;
    }

    return KOW_STATUS_OK;
}
