#ifndef _KOWHAI_UTILS_H_
#define _KOWHAI_UTILS_H_

#include "kowhai.h" 

/**
 * @brief called when a difference is found between two tree's
 * @param param, application specific parameter passed through
 * @param left_node, points to the left node where the difference was found, or NULL if right node is unique
 * @param left_data, point to the start of the difference in the left node, or NULL if right node is unique
 * @param right_node, points to the right node where the difference was found, or NULL if left node is unique
 * @param right_data, point to the start of the difference in the right node, or NULL if left node is unique
 * @param index, the array index of this node where the difference starts
 * @param depth number of parent nodes from the root node that this difference was found
 */
typedef int (*kowhai_on_diff_t)(void* param, const struct kowhai_node_t *left_node, void *left_data, const struct kowhai_node_t *right_node, void *right_data, int index, int depth);

/**
 * @brief diff left and right tree
 * If a node is found in the left tree that is not in the right tree (ie symbol path and types/array size match) or visa versa, call on_diff
 * If a node is found in both left and right tree, but the values of the node items do not match call on_diff
 * @param left, diff this tree against right
 * @param right, diff this tree against left
 * @param on_diff_param, application specific parameter passed through the on_diff callback
 * @param on_diff, call this when a unique node or common nodes that have different values are found
 */
int kowhai_diff(struct kowhai_tree_t *left, struct kowhai_tree_t *right, void* on_diff_param, kowhai_on_diff_t on_diff);

/**
 * @brief merge nodes that are common to src and dst from src into dst leaving unique nodes unchanged
 * @brief dst, destination tree (this is updated from the source tree)
 * @brief src, source tree
 * @return KOW_STATUS_OK on success otherwise other KOW_STATUS code
 */
int kowhai_merge(struct kowhai_tree_t *dst, struct kowhai_tree_t *src);

/**
 * Create a symbol path using a tree descriptor and a node within that destriptor
 * @param descriptor, the tree descriptor that we are making a symbol path from
 * @param node, the node in the tree that we want our symbol path to point to
 * @param target, the target buffer for our created symbol path
 * @param size, the size of target buffer
 * @return KOW_STATUS_OK on success (also update size param to actual size of symbol path created)
 */
int kowhai_create_symbol_path(struct kowhai_node_t* descriptor, struct kowhai_node_t* node, union kowhai_symbol_t* target, int* target_size);

/**
 * Create a symbol path using a tree and a memory offset within that trees data
 * @param tree, the tree that we are making a symbol path from
 * @param target_location, the memory location in the tree data that we want our symbol path to point to
 * @param target, the target buffer for our created symbol path
 * @param size, the size of target buffer
 * @return KOW_STATUS_OK on success (also update size param to actual size of symbol path created)
 */
int kowhai_create_symbol_path2(struct kowhai_tree_t* tree, void* target_location, union kowhai_symbol_t* target, int* target_size);

#endif

