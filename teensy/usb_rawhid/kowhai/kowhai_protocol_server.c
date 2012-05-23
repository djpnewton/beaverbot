#include "kowhai_protocol_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int kowhai_server_process_packet(struct kowhai_protocol_server_t* server, void* packet, size_t packet_size)
{
    struct kowhai_protocol_t prot;
    int bytes_required, status;

    if (packet_size > server->max_packet_size)
    {
        printf("    error: packet size too large\n");
        return KOW_STATUS_PACKET_BUFFER_TOO_BIG;
    }

    status = kowhai_protocol_parse(packet, packet_size, &prot);
    if (status != KOW_STATUS_OK && status != KOW_STATUS_INVALID_PROTOCOL_COMMAND)
        return status;

    if (prot.header.tree_id >= 0 && prot.header.tree_id < server->tree_count)
    {
        struct kowhai_tree_t tree;
        tree.desc = *(server->tree_descriptors + prot.header.tree_id);
        tree.data = *(server->tree_data_buffers + prot.header.tree_id);
        switch (prot.header.command)
        {
            case KOW_CMD_WRITE_DATA:
                printf("    CMD write data\n");
                status = kowhai_write(&tree, prot.payload.spec.data.symbols.count, prot.payload.spec.data.symbols.array_, prot.payload.spec.data.memory.offset, prot.payload.buffer, prot.payload.spec.data.memory.size);
                if (status == KOW_STATUS_OK)
                {
                    // call node_written callback
                    struct kowhai_node_t* node;
                    uint16_t offset;
                    kowhai_get_node(tree.desc, prot.payload.spec.data.symbols.count, prot.payload.spec.data.symbols.array_, &offset, &node);
                    server->node_written(server->node_written_param, node);
                    // send response
                    prot.header.command = KOW_CMD_WRITE_DATA_ACK;
                    kowhai_read(&tree, prot.payload.spec.data.symbols.count, prot.payload.spec.data.symbols.array_, prot.payload.spec.data.memory.offset, prot.payload.buffer, prot.payload.spec.data.memory.size);
                    kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                    server->send_packet(server->send_packet_param, server->packet_buffer, bytes_required);
                }
                else
                {
                    switch (status)
                    {
                        case KOW_STATUS_INVALID_SYMBOL_PATH:
                            printf("    invalid symbol path\n");
                            POPULATE_PROTOCOL_CMD(prot, prot.header.tree_id, KOW_CMD_ERROR_INVALID_SYMBOL_PATH);
                            break;
                        case KOW_STATUS_INVALID_OFFSET:
                            printf("    invalid payload offset\n");
                            POPULATE_PROTOCOL_CMD(prot, prot.header.tree_id, KOW_CMD_ERROR_INVALID_PAYLOAD_OFFSET);
                            break;
                        case KOW_STATUS_NODE_DATA_TOO_SMALL:
                            printf("    invalid payload size\n");
                            POPULATE_PROTOCOL_CMD(prot, prot.header.tree_id, KOW_CMD_ERROR_INVALID_PAYLOAD_SIZE);
                            break;
                        default:
                            printf("    unkown error\n");
                            POPULATE_PROTOCOL_CMD(prot, prot.header.tree_id, KOW_CMD_ERROR_UNKNOWN);
                            break;
                    }
                    kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                    server->send_packet(server->send_packet_param, server->packet_buffer, bytes_required);
                }
                break;
            case KOW_CMD_READ_DATA:
            {
                uint16_t node_offset;
                int size, overhead, max_payload_size;
                struct kowhai_node_t* node;
                struct kowhai_protocol_symbol_spec_t symbols = prot.payload.spec.data.symbols;
                printf("    CMD read data\n");
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
                    // allocate payload buffer
                    prot.payload.buffer = malloc(server->max_packet_size - overhead);

                    // send packets
                    while (size > max_payload_size)
                    {
                        prot.payload.spec.data.memory.size = max_payload_size;
                        kowhai_read(&tree, prot.payload.spec.data.symbols.count, prot.payload.spec.data.symbols.array_, prot.payload.spec.data.memory.offset, prot.payload.buffer, prot.payload.spec.data.memory.size);
                        kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                        server->send_packet(server->send_packet_param, server->packet_buffer, bytes_required);
                        // increment payload offset and decrement remaining payload size
                        prot.payload.spec.data.memory.offset += max_payload_size;
                        size -= max_payload_size;
                    }
                    // send final packet
                    prot.header.command = KOW_CMD_READ_DATA_ACK_END;
                    prot.payload.spec.data.memory.size = size;
                    kowhai_read(&tree, prot.payload.spec.data.symbols.count, prot.payload.spec.data.symbols.array_, prot.payload.spec.data.memory.offset, prot.payload.buffer, prot.payload.spec.data.memory.size);
                    kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                    server->send_packet(server->send_packet_param, server->packet_buffer, bytes_required);
                    // free payload buffer
                    free(prot.payload.buffer);
                }
                else
                {
                    switch (status)
                    {
                        case KOW_STATUS_INVALID_SYMBOL_PATH:
                            printf("    invalid symbol path\n");
                            POPULATE_PROTOCOL_CMD(prot, prot.header.tree_id, KOW_CMD_ERROR_INVALID_SYMBOL_PATH);
                            break;
                        case KOW_STATUS_INVALID_OFFSET:
                            printf("    invalid payload offset\n");
                            POPULATE_PROTOCOL_CMD(prot, prot.header.tree_id, KOW_CMD_ERROR_INVALID_PAYLOAD_OFFSET);
                            break;
                        case KOW_STATUS_NODE_DATA_TOO_SMALL:
                            printf("    invalid payload size\n");
                            POPULATE_PROTOCOL_CMD(prot, prot.header.tree_id, KOW_CMD_ERROR_INVALID_PAYLOAD_SIZE);
                            break;
                        default:
                            printf("    unkown error\n");
                            POPULATE_PROTOCOL_CMD(prot, prot.header.tree_id, KOW_CMD_ERROR_UNKNOWN);
                            break;
                    }
                    kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                    server->send_packet(server->send_packet_param, server->packet_buffer, bytes_required);
                }
                break;
            }
            case KOW_CMD_READ_DESCRIPTOR:
            {
                int size, overhead, max_payload_size;
                printf("    CMD read descriptor\n");
                // get descriptor size
                size = *(server->tree_descriptor_sizes + prot.header.tree_id);
                // get protocol overhead
                prot.header.command = KOW_CMD_READ_DESCRIPTOR_ACK;
                kowhai_protocol_get_overhead(&prot, &overhead);
                // setup max payload size and payload offset
                max_payload_size = server->max_packet_size - overhead;
                prot.payload.spec.descriptor.offset = 0;
                prot.payload.spec.descriptor.node_count = size / sizeof(struct kowhai_node_t);
                // allocate payload buffer
                prot.payload.buffer = malloc(server->max_packet_size - overhead);

                // send packets
                while (size > max_payload_size)
                {
                    prot.payload.spec.descriptor.size = max_payload_size;
                    memcpy(prot.payload.buffer, (char*)tree.desc + prot.payload.spec.descriptor.offset, prot.payload.spec.descriptor.size);
                    kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                    server->send_packet(server->send_packet_param, server->packet_buffer, bytes_required);
                    // increment payload offset and decrement remaining payload size
                    prot.payload.spec.descriptor.offset += max_payload_size;
                    size -= max_payload_size;
                }
                // send final packet
                prot.header.command = KOW_CMD_READ_DESCRIPTOR_ACK_END;
                prot.payload.spec.descriptor.size = size;
                memcpy(prot.payload.buffer, (char*)tree.desc + prot.payload.spec.descriptor.offset, prot.payload.spec.descriptor.size);
                kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                server->send_packet(server->send_packet_param, server->packet_buffer, bytes_required);
                // free payload buffer
                free(prot.payload.buffer);
                break;
            }
            default:
                printf("    invalid command (%d)\n", prot.header.command);
                POPULATE_PROTOCOL_CMD(prot, prot.header.tree_id, KOW_CMD_ERROR_INVALID_COMMAND);
                kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
                server->send_packet(server->send_packet_param, server->packet_buffer, bytes_required);
                break;
        }
    }
    else
    {
        printf("    invalid tree id (%d)\n", prot.header.tree_id);
        POPULATE_PROTOCOL_CMD(prot, prot.header.tree_id, KOW_CMD_ERROR_INVALID_TREE_ID);
        kowhai_protocol_create(server->packet_buffer, server->max_packet_size, &prot, &bytes_required);
        server->send_packet(server->send_packet_param, server->packet_buffer, bytes_required);
    }

    return KOW_STATUS_OK;
}
