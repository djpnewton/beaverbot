#ifndef _KOWHAI_PROTOCOL_SERVER_H_
#define _KOWHAI_PROTOCOL_SERVER_H_

#include "kowhai_protocol.h" 

#include <stddef.h>

/**
 * @brief callback used to send a kowhai packet to the indented target
 * @param param application specific parameter passed through
 * @param packet the packet buffer to write out
 * @param packet_size bytes in the packet buffer
 */
typedef void (*kowhai_send_packet_t)(void* param, void* packet, size_t packet_size);

/**
 * @brief called after a node has been written over the kowhai protocol
 * @param param application specific parameter passed through
 * @param node points to the node that was updated
 */
typedef void (*kowhai_node_written_t)(void* param, struct kowhai_node_t* node);

struct kowhai_protocol_server_t
{
    size_t max_packet_size;
    void* packet_buffer;
    kowhai_node_written_t node_written;
    void* node_written_param;
    kowhai_send_packet_t send_packet;
    void* send_packet_param;
    int tree_count;
    struct kowhai_node_t** tree_descriptors;
    size_t* tree_descriptor_sizes;
    void** tree_data_buffers;
};

/**
 * @brief Parse a kowhai packet and perform requested commands
 * @param server configuration for this server
 * @param packet parse this and perform commands
 * @param packet_size number bytes in packet
 */
int kowhai_server_process_packet(struct kowhai_protocol_server_t* server, void* packet, size_t packet_size);

#endif
