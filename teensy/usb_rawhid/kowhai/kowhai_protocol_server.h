#ifndef _KOWHAI_PROTOCOL_SERVER_H_
#define _KOWHAI_PROTOCOL_SERVER_H_

#include "kowhai_protocol.h" 

#include <stddef.h>

typedef struct kowhai_protocol_server_t* pkowhai_protocol_server_t;

/**
 * @brief callback used to send a kowhai packet to the indented target
 * @param server the protocol server object
 * @param param application specific parameter passed through
 * @param packet the packet buffer to write out
 * @param packet_size bytes in the packet buffer
 */
typedef void (*kowhai_send_packet_t)(pkowhai_protocol_server_t server, void* param, void* packet, size_t packet_size);

/**
 * @brief called before node has been written via the kowhai protocol
 * @param server the protocol server object
 * @param param application specific parameter passed through
 * @param tree_id the tree that the node belongs to
 * @param node points to the node that was updated
 * @param offset is the relative offset of the node data within the tree data block
 */
typedef void (*kowhai_node_pre_write_t)(pkowhai_protocol_server_t server, void* param, uint16_t tree_id, struct kowhai_node_t* node, int offset);

/**
 * @brief called after a node has been written via the kowhai protocol
 * @param server the protocol server object
 * @param param application specific parameter passed through
 * @param tree_id the tree that the node belongs to
 * @param node points to the node that was updated
 * @param offset is the relative offset of the node data within the tree data block
 * @param bytes_written the number of bytes written to the tree data block
 */
typedef void (*kowhai_node_post_write_t)(pkowhai_protocol_server_t server, void* param, uint16_t tree_id, struct kowhai_node_t* node, int offset, int bytes_written);

/**
 * @brief called after a function has been called over the kowhai protocol
 * @param server the protocol server object
 * @param param application specific parameter passed through
 * @param function_id the id of the function that was called
 * @return function was successfully called or not
 */
typedef int (*kowhai_function_called_t)(pkowhai_protocol_server_t server, void* param, uint16_t function_id);

struct kowhai_protocol_server_tree_item_t
{
    struct kowhai_protocol_id_list_item_t list_id;
    struct kowhai_node_t* descriptor;
    size_t descriptor_size;
    void* data;
};

struct kowhai_protocol_server_function_item_t
{
    struct kowhai_protocol_id_list_item_t list_id;
    struct kowhai_protocol_function_details_t details;
};

struct kowhai_protocol_server_t
{
    size_t max_packet_size;
    void* packet_buffer;
    kowhai_node_pre_write_t node_pre_write;
    kowhai_node_post_write_t node_post_write;
    void* node_write_param;
    kowhai_send_packet_t send_packet;
    void* send_packet_param;
    int tree_list_count;
    struct kowhai_protocol_server_tree_item_t* tree_list;
    struct kowhai_protocol_id_list_item_t* tree_id_list;
    int function_list_count;
    struct kowhai_protocol_server_function_item_t* function_list;
    struct kowhai_protocol_id_list_item_t* function_id_list;
    kowhai_function_called_t function_called;
    void* function_called_param;
    int symbol_list_count;
    char** symbol_list;

    struct kowhai_node_t* current_write_node;
    int current_write_node_offset;
    int current_write_node_bytes_written;
};

void kowhai_server_init(struct kowhai_protocol_server_t* server,
    size_t max_packet_size,
    void* packet_buffer,
    kowhai_node_pre_write_t node_pre_write,
    kowhai_node_post_write_t node_post_write,
    void* node_write_param,
    kowhai_send_packet_t send_packet,
    void* send_packet_param,
    int tree_list_count,
    struct kowhai_protocol_server_tree_item_t* tree_list,
    struct kowhai_protocol_id_list_item_t* tree_id_list,
    int function_list_count,
    struct kowhai_protocol_server_function_item_t* function_list,
    struct kowhai_protocol_id_list_item_t* function_id_list,
    kowhai_function_called_t function_called,
    void* function_called_param,
    int symbol_list_count,
    char** symbol_list);

/**
 * @brief Parse a kowhai packet and perform requested commands
 * @param server configuration for this server
 * @param packet parse this and perform commands
 * @param packet_size number bytes in packet
 */
int kowhai_server_process_packet(struct kowhai_protocol_server_t* server, void* packet, size_t packet_size);

/**
 * @brief Process a kowhai event and send protocol response
 * @param tree_id the tree id (the description of the data contained in this event)
 * @param buffer the event data buffer
 * @param buffer_size the size of the buffer
 */
int kowhai_server_process_event(struct kowhai_protocol_server_t* server, uint16_t tree_id, void* buffer, int buffer_size);


#endif
