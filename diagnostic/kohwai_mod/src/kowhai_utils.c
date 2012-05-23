#include "kowhai_utils.h"

#include <string.h>

#ifdef KOWHAI_DBG
#include <stdio.h>
#define KOWHAI_UTILS_ERR "KOWHAI_UTILS ERROR:"
#define KOWHAI_UTILS_INFO "KOWHAI_UTILS INFO: "
#define KOWHAI_TABS "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
#endif

// forward decalre this to bring it in from kowhai.c as we need it
///@todo there should be a way to either use the public api or get at internal kowhia stuff more easily
int get_node(const struct kowhai_node_t *node, int num_symbols, const union kowhai_symbol_t *symbols, uint16_t *offset, struct kowhai_node_t **target_node, int initial_branch);

/**
 * @brief diff_l2r diff left tree against right tree
 * If a node is found in the left tree that is not in the right tree (ie symbol path and types/array size match) call on_unique 
 * If a node is found in both left and right tree, but the values of the node items do not match call on_diff
 * @note unique items on the right tree are ignored
 * @param left, diff this tree against right
 * @param left_node_index, the index of the left node in the original descriptor (used in recursive calls, original caller should pass 0)
 * @param right, diff this tree against left
 * @param right_node_index, the index of the right node in the original descriptor (used in recursive calls, original caller should pass 0)
 * @param on_diff_param, application specific parameter passed through the on_diff callback
 * @param on_unique, call this when a unique node is found in the left tree
 * @param on_diff, call this when a common node is found in both left and right trees and the values do not match
 * @param swap_cb_param, change the order of swap_cb_param so right if first then left, otherwise it is left then right
 * @param depth, how deep in the tree are we (0 root, 1 first branch, etc)
 */
static int diff_l2r(struct kowhai_tree_t *left, struct kowhai_tree_t *right, void* on_diff_param, kowhai_on_diff_t on_unique, kowhai_on_diff_t on_diff, int swap_cb_param, int depth)
{
    int ret;
    uint16_t offset;
    struct kowhai_node_t *right_node;
    union kowhai_symbol_t symbol[1];
    int size;
    int i;
    unsigned int skip_nodes;

    // go through all the left nodes and look for matches in right 
    while (1)
    {
        // are we at the end of this branch
        if (left->desc->type == KOW_BRANCH_END)
        {
            #ifdef KOWHAI_DBG
            printf(KOWHAI_UTILS_INFO "(%d)%.*s pop\n", depth, depth, KOWHAI_TABS);
            #endif
            return KOW_STATUS_OK;
        }

        // does the left path exist in the right path
        skip_nodes = 0;
        i = 0;
        symbol[0].parts.name = left->desc->symbol;
        symbol[0].parts.array_index = 0;
        ret = get_node(right->desc, 1, symbol, &offset, &right_node, 0);
        switch (ret)
        {
            case KOW_STATUS_OK:
                // found matching node in left and right tree's so diff all the common array items
                if (left->desc->type == KOW_BRANCH_START)
                {
                    // diff all the common array items one by one
                    struct kowhai_tree_t __left = *left;
                    struct kowhai_tree_t __right;
                    for (i = 0; i < left->desc->count && i < right_node->count; i++)
                    {
                        // get the offset into right for the branch array item to update
                        symbol[0].parts.array_index = i;
                        ret = get_node(right->desc, 1, symbol, &offset, NULL, 0);
                        if (ret != KOW_STATUS_OK)
                            return ret;
                        __right.desc = &right_node[1];
                        __right.data = ((uint8_t *)right->data + offset);
                        __left.desc = left->desc + 1;

                        // diff this branch array item (drill). NB recursive call to diff_l2r increments __left.data for each array item
                        #ifdef KOWHAI_DBG
                        printf(KOWHAI_UTILS_INFO "(%d)%.*s drill\n", depth, depth, KOWHAI_TABS, left->desc->symbol);
                        #endif
                        ret = diff_l2r(&__left, &__right, on_diff_param, on_unique, on_diff, swap_cb_param, depth + 1);
                        if (ret != KOW_STATUS_OK)
                            return ret;
                    }
                }
                else
                {
                    // get the size of each element in the array (not the whole node size)
                    int left_size, right_size;
                    ret = kowhai_get_node_size(left->desc, &left_size);
                    if (ret != KOW_STATUS_OK)
                        return ret;
                    ret = kowhai_get_node_size(right_node, &right_size);
                    if (ret != KOW_STATUS_OK)
                        return ret;

                    // diff all the common elements in the array and call on_diff if the values do not match
                    for (i = 0; i < left->desc->count && i < right_node->count; i++)
                    {
                        uint16_t left_offset, right_offset;
                        uint8_t *left_data, *right_data;
                        int run_on_diff = 0;

                        // get offsets into the left and right arrays for this array item
                        symbol[0].parts.array_index = i;
                        ret = get_node(left->desc, 1, symbol, &left_offset, NULL, 0);
                        if (ret != KOW_STATUS_OK)
                            return ret;
                        left_data = (uint8_t*)left->data + left_offset;
                        ret = get_node(right_node, 1, symbol, &right_offset, NULL, 0);
                        if (ret != KOW_STATUS_OK)
                            return ret;
                        right_data = (uint8_t*)right->data + offset + right_offset;
                        
                        // these array elements differ is the sizes dont match or the values dont match
                        if (left_size != right_size)
                            run_on_diff = 1;
                        if (memcmp(left_data, right_data, left_size / left->desc->count) != 0)
                            run_on_diff = 1;

                        // if a difference is detected then run on_diff
                        if (run_on_diff && (on_diff != NULL))
                        {
                            struct kowhai_node_t left_node = *left->desc;

                            if (!swap_cb_param)
                                ret = on_diff(on_diff_param, &left_node, left_data, right_node, right_data, i, depth);
                            else
                                ret = on_diff(on_diff_param, right_node, right_data, &left_node, left_data, i, depth);
                            if (ret != KOW_STATUS_OK)
                                return ret;
                        }
                    }
                }
                // allow fall through to call on_unqiue for any remaining nodes in the left but not right trees
            case KOW_STATUS_INVALID_SYMBOL_PATH:
                // we got here either becuase we could not find the left node in the right tree, or we fell through from above after 
                // processing all the common array elements for nodes that appear in the left and right trees; now we need to call
                // on unique for all unprocessed nodes (unqiue nodes in the left tree)
                for (; i < left->desc->count; i++)
                {
                    void* left_data;
                    // get the offset where array item i starts in the left branch
                    symbol[0].parts.array_index = i;
                    ret = get_node(left->desc, 1, symbol, &offset, NULL, 0);
                    if (ret != KOW_STATUS_OK)
                        return ret;
                    left_data = (uint8_t*)left->data + offset;

                    // call on unique
                    if (on_unique != NULL)
                    {
                        if (!swap_cb_param)
                            ret = on_unique(on_diff_param, left->desc, left_data, NULL, NULL, i, depth);
                        else
                            ret = on_unique(on_diff_param, NULL, NULL, left->desc, left_data, i, depth);
                        if (ret != KOW_STATUS_OK)
                            return ret;
                    }
                }
                break;
            default:
                // fail boat
                return ret;
        }

        // done with this node increment our position in the left tree
        // note if this was a branch skip_nodes will != 0 and contain the number of nodes and
        // kowhai_get_node_size will give the size of the whole branch
        ret = kowhai_get_node_size(left->desc, &size);
        if (ret != KOW_STATUS_OK)
            return ret;
        ret = kowhai_get_node_count(left->desc, &skip_nodes);
        if (ret != KOW_STATUS_OK)
            return ret;
        left->data = (uint8_t *)left->data + size;
        left->desc += skip_nodes;

        // if this tree is not nicely formed (ie wrapped in a branch start/end) then the next item may not be a 
        // branch end, instead we might just run off the end of the buffer so force a stop
        if (depth == 0)
            return KOW_STATUS_OK;

    }
}

/**
 * @brief diff left and right tree
 * If a node is found in the left tree that is not in the right tree (ie symbol path and types/array size match) or visa versa, call on_diff
 * If a node is found in both left and right tree, but the values of the node items do not match call on_diff
 * @param left, diff this tree against right
 * @param right, diff this tree against left
 * @param on_diff_param, application specific parameter passed through the on_diff callback
 * @param on_diff, call this when a unique node or common nodes that have different values are found
 */
int kowhai_diff(struct kowhai_tree_t *left, struct kowhai_tree_t *right, void* on_diff_param, kowhai_on_diff_t on_diff)
{
    struct kowhai_tree_t _left, _right;
    int ret;

    // we use diff_l2r to find nodes that are unique in the left tree, or nodes that differ in value between left and right first
    #ifdef KOWHAI_DBG
    printf(KOWHAI_UTILS_INFO "diff left against right\n");
    #endif
    _left = *left;
    _right = *right;
    ret = diff_l2r(&_left, &_right, on_diff_param, on_diff, on_diff, 0, 0);
    if (ret != KOW_STATUS_OK)
        return ret;

    // we just have to find nodes that are unique in the right tree. to do this we reuse diff_l2r with left and right swapped
    // and ask diff_l2r to reverse the params to the callbacks
    #ifdef KOWHAI_DBG
    printf(KOWHAI_UTILS_INFO "diff right against left\n");
    #endif
    _left = *left;
    _right = *right;
    ret = diff_l2r(&_right, &_left, on_diff_param, on_diff, NULL, 1, 0);

    return ret;
}

/**
 * @brief called by diff when merging
 * @param param unused parameter
 * @param dst this is the destination node to merge common source nodes into, or NULL if node is unique to src
 * @param src this is the source node to merge into common destination nodes, or NULL if node is unique to dst
 * @param depth, how deep in the tree are we (0 root, 1 first branch, etc)
 */
static int on_diff_merge(void* param, const struct kowhai_node_t *dst_node, void *dst_data, const struct kowhai_node_t *src_node, void *src_data, int index, int depth)
{
    int size;

    //
    // sanity checks for merge
    //

    if (dst_node == NULL)
    {
        #ifdef KOWHAI_DBG
        printf(KOWHAI_UTILS_INFO "(%d)%.*s unique node %d in src[%d]\n", depth, depth, KOWHAI_TABS, src_node->symbol, index);
        #endif
        return KOW_STATUS_OK;
    }

    if (src_node == NULL)
    {
        #ifdef KOWHAI_DBG
        printf(KOWHAI_UTILS_INFO "(%d)%.*s unique node %d in dst[%d]\n", depth, depth, KOWHAI_TABS, dst_node->symbol, index);
        #endif
        return KOW_STATUS_OK;
    }

    // if types are not the same then do not merge them
    ///@todo maybe we should allow upcasting ?
    if (dst_node->type != src_node->type)
    {
        #ifdef KOWHAI_DBG
        printf(KOWHAI_UTILS_INFO "(%d)%.*s cannot merge dst type %d in to src type %d\n", depth, depth, KOWHAI_TABS, dst_node->type, src_node->type);
        #endif
        return KOW_STATUS_OK;
    }

    // if array size's are not the same then do not merge
    ///@todo maybe we should merge as much as possible
    if (dst_node->count != src_node->count)
    {
        #ifdef KOWHAI_DBG
        printf(KOWHAI_UTILS_INFO "(%d)%.*s cannot merge arrays off different sizes [dst.count = %d, src.count = %d]\n", depth, depth, KOWHAI_TABS, dst_node->count, src_node->count);
        #endif
        return KOW_STATUS_OK;
    }

    //
    // both nodes exist but differ so copy src into dst
    //

    size = kowhai_get_node_type_size(dst_node->type);
    if (size == -1)
        ///@todo need a better error code here !!
        return KOW_STATUS_OK;

    #ifdef KOWHAI_DBG
    printf(KOWHAI_UTILS_INFO "(%d)%.*s merging %d bytes of %d[%d] from src into dst\n", depth, depth, KOWHAI_TABS, size, dst_node->symbol, index);
    #endif
    memcpy(dst_data, src_data, size);
    
    return KOW_STATUS_OK;
}

// merge nodes that are common to src and dst from src into dst leaving unique nodes unchanged
int kowhai_merge(struct kowhai_tree_t *dst, struct kowhai_tree_t *src)
{
    struct kowhai_tree_t *_dst = dst;

    #ifdef KOWHAI_DBG
    printf("\n");
    #endif
    // we only support merging of well formed tree's (both src and dst)
    if (dst->desc->type != KOW_BRANCH_START ||
        src->desc->type != KOW_BRANCH_START)
        return KOW_STATUS_INVALID_DESCRIPTOR;
    
    // update all the notes in dst that are common to dst and src
    return kowhai_diff(_dst, src, NULL, on_diff_merge);
}

int kowhai_create_symbol_path(struct kowhai_node_t* descriptor, struct kowhai_node_t* node, union kowhai_symbol_t* target, int* target_size)
{
    int symbol_path_index = 0;
    while (descriptor <= node)
    {
        if (symbol_path_index >= *target_size)
            return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
        target[symbol_path_index].symbol = descriptor->symbol;
        switch (descriptor->type)
        {
            case KOW_BRANCH_START:
                symbol_path_index++;
                break;
            case KOW_BRANCH_END:
                symbol_path_index--;
                break;
            default:
                break;
        }
        descriptor++;
    }
    *target_size = symbol_path_index + 1;
    return KOW_STATUS_OK;
}

int _create_symbol_path2(struct kowhai_tree_t* tree, void* target_location, union kowhai_symbol_t* target, int* target_size, int symbol_path_length)
{
    if (*target_size < symbol_path_length)
        return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
    while (tree->data <= target_location)
    {
        int ret;
        switch (tree->desc->type)
        {
            case KOW_BRANCH_START:
            {
                int i;
                struct kowhai_node_t* node = tree->desc;
                for (i = 0; i < node->count; i++)
                {
                    target[symbol_path_length - 1].symbol = KOWHAI_SYMBOL(node->symbol, i);
                    tree->desc = node + 1;
                    ret = _create_symbol_path2(tree, target_location, target, target_size, symbol_path_length + 1);
                    if (ret == KOW_STATUS_OK)
                        return ret;
                    if (ret != KOW_STATUS_NOT_FOUND)
                        return ret;
                }
                break;
            }
            case KOW_BRANCH_END:
                return KOW_STATUS_NOT_FOUND;
            default:
            {
                int i;
                ret = kowhai_get_node_type_size(tree->desc->type);
                for (i = 0; i < tree->desc->count; i++)
                {
                    target[symbol_path_length - 1].symbol = KOWHAI_SYMBOL(tree->desc->symbol, i);
                    tree->data = (char*)tree->data + ret;
                    if (tree->data > target_location)
                        break;
                }
                break;
            }
        }
        tree->desc++;
    }
    *target_size = symbol_path_length;
    return KOW_STATUS_OK;
}

int kowhai_create_symbol_path2(struct kowhai_tree_t* tree, void* target_location, union kowhai_symbol_t* target, int* target_size)
{
    struct kowhai_tree_t tmp_tree = *tree;
    int branch_size = 0;
    // first node will be a branch
    if (*target_size < 1)
        return KOW_STATUS_TARGET_BUFFER_TOO_SMALL;
    target->symbol = tmp_tree.desc->symbol;
    tmp_tree.desc++;
    return _create_symbol_path2(&tmp_tree, target_location, target, target_size, 2);
}

