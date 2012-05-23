#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "kowhai_serialize.h"
#include "../3rdparty/jsmn/jsmn.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <stdarg.h>
//#define snprintf debug_printf
int debug_printf(char* buf, size_t buf_size, char* format, ...)
{
    va_list args;
    va_start(args, format);
    return vprintf(format, args);
    va_end(args);
}

#define NAME "name"
#define TYPE "type"
#define SYMBOL "symbol"
#define COUNT "count"
#define TAG "tag"
#define VALUE "value"
#define ARRAY "array"
#define CHILDREN "children"

int write_string(char* buffer, size_t buffer_size, const char* format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = vsnprintf(buffer, buffer_size, format, args);
    va_end(args);

    // we want to abort in the case of a truncated result
    if (result > (long long)buffer_size)
        return -1000;
    return result;
}

int add_header(char** dest, size_t* dest_size, int* current_offset, struct kowhai_node_t* node, void* get_name_param, kowhai_get_symbol_name_t get_name)
{
    int chars = write_string(*dest, *dest_size,
            "{\""NAME"\": \"%s\", \""TYPE"\": %d, \""SYMBOL"\": %d, \""COUNT"\": %d, \""TAG"\": %d",
            get_name(get_name_param, node->symbol), node->type, node->symbol, node->count, node->tag);
    if (chars >= 0)
    {
        *dest += chars;
        *dest_size -= chars;
        *current_offset += chars;
    }
    return chars;
}

int add_string(char** dest, size_t* dest_size, int* current_offset, char* string)
{
    int chars = write_string(*dest, *dest_size, string);
    if (chars >= 0)
    {
        *dest += chars;
        *dest_size -= chars;
        *current_offset += chars;
    }
    return chars;
}

int add_indent(char** dest, size_t* dest_size, int* current_offset, int depth)
{
    int i, chars = 0;
    for (i = 0; i < depth; i++)
    {
        int c = add_string(dest, dest_size, current_offset, "\t");
        if (c < 0)
            return c;
        chars += c;
    }
    return chars;
}

int add_value(char** dest, size_t* dest_size, int* current_offset, uint16_t node_type, void* data)
{
    int chars;
    switch (node_type)
    {
        case KOW_CHAR:
            chars = write_string(*dest, *dest_size, "%d", *((char *)data));
            break;
        case KOW_INT8:
            chars = write_string(*dest, *dest_size, "%d", *((int8_t*)data));
            break;
        case KOW_INT16:
            chars = write_string(*dest, *dest_size, "%d", *((int16_t*)data));
            break;
        case KOW_INT32:
            chars = write_string(*dest, *dest_size, "%d", *((int32_t*)data));
            break;
        case KOW_UINT8:
            chars = write_string(*dest, *dest_size, "%d", *((uint8_t*)data));
            break;
        case KOW_UINT16:
            chars = write_string(*dest, *dest_size, "%d", *((uint16_t*)data));
            break;
        case KOW_UINT32:
            chars = write_string(*dest, *dest_size, "%d", *((uint32_t*)data));
            break;
        case KOW_FLOAT:
            chars = write_string(*dest, *dest_size, "%f", *((float*)data));
            break;
        default:
            return -1;
    }
    if (chars >= 0)
    {
        *dest += chars;
        *dest_size -= chars;
        *current_offset += chars;
    }
    return chars;
}

int serialize_node(struct kowhai_node_t** desc, void** data, char* target_buffer, size_t target_size, int level, void* get_name_param, kowhai_get_symbol_name_t get_name)
{
    int target_offset = 0;
    struct kowhai_node_t* node;
    int i, chars;
    char* node_end_str;

    while (1)
    {
        node = *desc;

        if (node->type == KOW_BRANCH_END)
            return target_offset;

        // indent to current level using tabs
        chars = add_indent(&target_buffer, &target_size, &target_offset, level);
        if (chars < 0)
            return chars;

        //
        // write node
        //

        switch (node->type)
        {
            case KOW_BRANCH_START:
            {
                // write header
                chars = add_header(&target_buffer, &target_size, &target_offset, node, get_name_param, get_name);
                if (chars < 0)
                    return chars;
                if (node->count > 1)
                {
                    struct kowhai_node_t* initial_node = *desc;
                    // write array identifier
                    chars = add_string(&target_buffer, &target_size, &target_offset, ", \""ARRAY"\": [\n");
                    if (chars < 0)
                    return chars;
                    for (i = 0; i < node->count; i++)
                    {
                        // set descriptor to initial node at the branch array
                        *desc = initial_node;
                        (*desc) += 1;
                        // indent to current level using tab
                        chars = add_indent(&target_buffer, &target_size, &target_offset, level + 1);
                        if (chars < 0)
                            return chars;
                        // write branch children start
                        chars = add_string(&target_buffer, &target_size, &target_offset, "[\n");
                        if (chars < 0)
                            return chars;
                        // write branch children
                        chars = serialize_node(desc, data, target_buffer, target_size, level + 1, get_name_param, get_name);
                        if (chars < 0)
                            return chars;
                        target_offset += chars;
                        target_buffer += chars;
                        target_size -= chars;
                        // indent to current level using tab
                        chars = add_indent(&target_buffer, &target_size, &target_offset, level + 1);
                        if (chars < 0)
                            return chars;
                        // write branch children end
                        chars = add_string(&target_buffer, &target_size, &target_offset, "]");
                        if (chars < 0)
                            return chars;
                        if (i < node->count - 1)
                        {
                            chars = add_string(&target_buffer, &target_size, &target_offset, ",\n");
                            if (chars < 0)
                                return chars;
                        }
                        else
                        {
                            chars = add_string(&target_buffer, &target_size, &target_offset, "\n");
                            if (chars < 0)
                                return chars;
                        }
                    }
                }
                else
                {
                    // write children identifier
                    chars = add_string(&target_buffer, &target_size, &target_offset, ", \""CHILDREN"\": [\n");
                    if (chars < 0)
                        return chars;
                    // write branch children
                    (*desc) += 1;
                    chars = serialize_node(desc, data, target_buffer, target_size, level + 1, get_name_param, get_name);
                    if (chars < 0)
                        return chars;
                    target_offset += chars;
                    target_buffer += chars;
                    target_size -= chars;
                }
                // indent to current level using tab
                chars = add_indent(&target_buffer, &target_size, &target_offset, level);
                if (chars < 0)
                    return chars;
                // write node end
                if (level == 0 || (*desc)[1].type == KOW_BRANCH_END)
                    node_end_str = "]}\n";
                else
                    node_end_str = "]},\n";
                chars = add_string(&target_buffer, &target_size, &target_offset, node_end_str);
                if (chars < 0)
                    return chars;
                break;
            }
            default:
            {
                int value_size = kowhai_get_node_type_size(node->type);
                // write header
                chars = add_header(&target_buffer, &target_size, &target_offset, node, get_name_param, get_name);
                if (chars < 0)
                    return chars;
                // write value identifier
                chars = add_string(&target_buffer, &target_size, &target_offset, ", \""VALUE"\": ");
                if (chars < 0)
                    return chars;
                // write value/s
                if (node->count > 1)
                {
                    chars = add_string(&target_buffer, &target_size, &target_offset, "[");
                    if (chars < 0)
                        return chars;
                    for (i = 0; i < node->count; i++)
                    {
                        // write leaf node array item value
                        chars = add_value(&target_buffer, &target_size, &target_offset, node->type, *data);
                        if (chars < 0)
                            return chars;
                        // increment data pointer
                        *data = (char*)*data + value_size;
                        // write comma if there is another array item
                        if (i < node->count - 1)
                        {
                            chars = add_string(&target_buffer, &target_size, &target_offset, ", ");
                            if (chars < 0)
                                return chars;
                        }
                    }
                    chars = add_string(&target_buffer, &target_size, &target_offset, "]");
                    if (chars < 0)
                        return chars;
                }
                else
                {
                    // write leaf node value
                    chars = add_value(&target_buffer, &target_size, &target_offset, node->type, *data);
                    if (chars < 0)
                        return chars;
                    // increment data pointer
                    *data = (char*)*data + value_size;
                }
                // write node end
                if (level == 0 || node[1].type == KOW_BRANCH_END)
                    node_end_str = " }\n";
                else
                    node_end_str = " },\n";
                chars = add_string(&target_buffer, &target_size, &target_offset, node_end_str);
                if (chars < 0)
                    return chars;
                break;
            }
        }

        if (level == 0)
            return target_offset;

        (*desc) += 1;
    }
}

int kowhai_serialize(struct kowhai_tree_t tree, char* target_buffer, int* target_size, void* get_name_param, kowhai_get_symbol_name_t get_name)
{
    int chars = serialize_node(&tree.desc, &tree.data, target_buffer, *target_size, 0, get_name_param, get_name);
    if (chars < 0)
        return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
    *target_size = chars;
    return KOW_STATUS_OK;
}

int copy_string_from_token(const char* js, jsmntok_t* tok, char* dest, int dest_size)
{
    int token_string_size = tok->end - tok->start;
    if (token_string_size < dest_size)
    {
        strncpy(dest, js + tok->start, token_string_size);
        dest[token_string_size] = 0;
        return 1;
    }
    return 0;
}

#define TEMP_SIZE 100

int get_token_uint8(jsmn_parser* parser, jsmntok_t* tok, uint8_t* value)
{
    char temp[TEMP_SIZE];
    if (copy_string_from_token(parser->js, tok, temp, TEMP_SIZE))
    {
        *value = atoi(temp);
        return KOW_STATUS_OK;
    }
    else
        return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
}

int get_token_uint16(jsmn_parser* parser, jsmntok_t* tok, uint16_t* value)
{
    char temp[TEMP_SIZE];
    if (copy_string_from_token(parser->js, tok, temp, TEMP_SIZE))
    {
        *value = atoi(temp);
        return KOW_STATUS_OK;
    }
    else
        return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
}

int get_token_uint32(jsmn_parser* parser, jsmntok_t* tok, uint32_t* value)
{
    char temp[TEMP_SIZE];
    if (copy_string_from_token(parser->js, tok, temp, TEMP_SIZE))
    {
        *value = atol(temp);
        return KOW_STATUS_OK;
    }
    else
        return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
}

int get_token_float(jsmn_parser* parser, jsmntok_t* tok, float* value)
{
    char temp[TEMP_SIZE];
    if (copy_string_from_token(parser->js, tok, temp, TEMP_SIZE))
    {
        *value = (float)atof(temp);
        return KOW_STATUS_OK;
    }
    else
        return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
}

int token_string_match(jsmn_parser* parser, jsmntok_t* tok, char* str)
{
    return tok->end - tok->start == strlen(str) &&
        strncmp(parser->js + tok->start, str, strlen(str)) == 0;
}

int process_token(jsmn_parser* parser, int token_index, struct kowhai_node_t* desc, int desc_size, int* desc_nodes_populated, void* data, int data_size, int* data_offset)
{
#define INC { i++; token_index++; continue; }
    int initial_token_index = token_index;
    struct kowhai_node_t* initial_desc = desc;
    int i;
    int res;
    *desc_nodes_populated = 0;
    *data_offset = 0;

    if (desc_size < 1)
        return -2;

    if (parser->tokens[token_index].type == JSMN_OBJECT)
    {
        int token_is_branch = 0;
        jsmntok_t* parent_tok = &parser->tokens[token_index];

        token_index = initial_token_index;
        desc = initial_desc;

        for (i = 0; i < parent_tok->size; i++)
        {
            jsmntok_t* tok;
            token_index++;
            tok = &parser->tokens[token_index];
            if (tok->type == JSMN_STRING)
            {
                if (token_string_match(parser, tok, TYPE))
                {
                    res = get_token_uint16(parser, tok + 1, &desc->type);
                    if (res != KOW_STATUS_OK)
                        return -1;
                    INC;
                }
                else if (token_string_match(parser, tok, SYMBOL))
                {
                    res = get_token_uint16(parser, tok + 1, &desc->symbol);
                    if (res != KOW_STATUS_OK)
                        return -1;
                    INC;
                }
                else if (token_string_match(parser, tok, COUNT))
                {
                    res = get_token_uint16(parser, tok + 1, &desc->count);
                    if (res != KOW_STATUS_OK)
                        return -1;
                    INC;
                }
                else if (token_string_match(parser, tok, TAG))
                {
                    res = get_token_uint16(parser, tok + 1, &desc->tag);
                    if (res != KOW_STATUS_OK)
                        return -1;
                    INC;
                }
                else if (token_string_match(parser, tok, VALUE))
                {
                    int k;
                    i++;
                    if (desc->count > 1)
                    {
                        token_index++;
                        tok++;
                    }
                    for (k = 0; k < desc->count; k++)
                    {
                        token_index++;
                        tok++;
                        switch (desc->type)
                        {
                            case KOW_UINT8:
                            case KOW_INT8:
                            case KOW_CHAR:
                            {
                                uint8_t value;
                                //TODO: could probably call get_token_uint32 instead and remove get_token_uint8 (needs testing)
                                res = get_token_uint8(parser, tok, &value);
                                if (res != KOW_STATUS_OK)
                                    return -1;
                                if ((*data_offset) + (int)sizeof(uint8_t) > data_size)
                                    return -3;
                                *((uint8_t*)data) = value;
                                data = (char*)data + sizeof(uint8_t);
                                data_size -= sizeof(uint8_t);
                                *data_offset += sizeof(uint8_t);
                                break;
                            }
                            case KOW_UINT16:
                            case KOW_INT16:
                            {
                                uint16_t value;
                                //TODO: could probably call get_token_uint32 instead and remove get_token_uint16 (needs testing)
                                res = get_token_uint16(parser, tok, &value);
                                if (res != KOW_STATUS_OK)
                                    return -1;
                                if ((*data_offset) + (int)sizeof(uint16_t) > data_size)
                                    return -3;
                                *((uint16_t*)data) = value;
                                data = (char*)data + sizeof(uint16_t);
                                data_size -= sizeof(uint16_t);
                                *data_offset += sizeof(uint16_t);
                                break;
                            }
                            case KOW_UINT32:
                            case KOW_INT32:
                            {
                                uint32_t value;
                                res = get_token_uint32(parser, tok, &value);
                                if (res != KOW_STATUS_OK)
                                    return -1;
                                if ((*data_offset) + (int)sizeof(uint32_t) > data_size)
                                    return -3;
                                *((uint32_t*)data) = value;
                                data = (char*)data + sizeof(uint32_t);
                                data_size -= sizeof(uint32_t);
                                *data_offset += sizeof(uint32_t);
                                break;
                            }
                            case KOW_FLOAT:
                            {
                                float value;
                                res = get_token_float(parser, tok, &value);
                                if (res != KOW_STATUS_OK)
                                    return -1;
                                if ((*data_offset) + (int)sizeof(float) > data_size)
                                    return -3;
                                *((float*)data) = value;
                                data = (char*)data + sizeof(float);
                                data_size -= sizeof(float);
                                *data_offset += sizeof(float);
                                break;
                            }
                        }
                    }
                    continue;
                }
                else if (token_string_match(parser, tok, CHILDREN) ||
                            token_string_match(parser, tok, ARRAY))
                {
                    int parent_array_index;
                    int parent_array_size = 1, start_nodes_populated = *desc_nodes_populated;
                    int k, nodes_populated, _data_offset;
                    jsmntok_t* array_tok = tok + 1;
                    i++;
                    token_index++;

                    if (token_string_match(parser, tok, ARRAY))
                    {
                        parent_array_size = array_tok->size;
                        array_tok = array_tok + 1;
                        token_index++;
                    }
                    for (parent_array_index = 0; parent_array_index < parent_array_size; parent_array_index++)
                    {
                        desc = initial_desc;
                        *desc_nodes_populated = start_nodes_populated;

                        if (array_tok->size > 0)
                        {
                            token_index++;
                            for (k = 0; k < array_tok->size; k++)
                            {
                                res = process_token(parser, token_index, desc + 1, desc_size - 1, &nodes_populated, data, data_size, &_data_offset);
                                if (res >= 0)
                                {
                                    token_index += res;
                                    if (k < array_tok->size - 1)
                                        token_index++;
                                    desc += nodes_populated;
                                    desc_size -= nodes_populated;
                                    *desc_nodes_populated += nodes_populated;
                                    data = (char*)data + _data_offset;
                                    data_size -= _data_offset;
                                    *data_offset += _data_offset;
                                }
                                else
                                    return res;
                            }
                        }
                        if (parent_array_index < parent_array_size - 1)
                            token_index++;
                    }
                }
            }
        }
        token_is_branch = initial_desc->type == KOW_BRANCH_START;
        if (token_is_branch)
        {
            desc++;
            desc_size--;

            if (desc_size < 1)
                return -2;

            desc->type = KOW_BRANCH_END;
            desc->symbol = desc->symbol;
            (*desc_nodes_populated)++;
        }
        (*desc_nodes_populated)++;
    }
    return token_index - initial_token_index;
}

int kowhai_deserialize(char* buffer, void* scratch, int scratch_size, struct kowhai_node_t* descriptor, int* descriptor_size, void* data, int* data_size)
{
    int desc_nodes_populated, data_offset, result;
    jsmn_parser parser;
    jsmntok_t* tokens = (jsmntok_t*)scratch;
    int token_count = scratch_size / sizeof(jsmntok_t);
    jsmnerr_t err;
    jsmn_init_parser(&parser, buffer, tokens, token_count);
    err = jsmn_parse(&parser);
    switch (err)
    {
        case JSMN_ERROR_INVAL:
        case JSMN_ERROR_PART:
            return KOW_STATUS_BUFFER_INVALID;
        case JSMN_ERROR_NOMEM:
            return KOW_STATUS_SCRATCH_TOO_SMALL;
    }

    result = process_token(&parser, 0, descriptor, *descriptor_size, &desc_nodes_populated, data, *data_size, &data_offset);
    switch (result)
    {
        case -1:
            return KOW_STATUS_BUFFER_INVALID;
        case -2:
        case -3:
            return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
    }
    *descriptor_size = desc_nodes_populated;
    *data_size = data_offset;

    return KOW_STATUS_OK;
}
