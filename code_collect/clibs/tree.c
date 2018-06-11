#include <stdio.h>
#include <string.h>

#include "common.h"
#include "tree.h"

static int check_node_index(const int index) {

    int i ;
    for (i = 0; i < vector_size(g_hiertree->nodeIndexs); ++i) {
        if (*(int*)vector_at(g_hiertree->nodeIndexs, i) == index) {
            // found node.
            return true;
        }
    }
    return false;
}

TreeNode *get_node(TreeNode *tree, int index) {

    TreeNode *nodeinfo;

    if (NULL != tree) {
        if (tree->index == index) {
            nodeinfo = tree;
            return nodeinfo;
        }
        //search child node
        nodeinfo = get_node(tree->child, index);
        //search slibling node
        if (nodeinfo == NULL)
            nodeinfo = get_node(tree->sibling, index);
    }

    return nodeinfo;
}

TreeNode *get_node_by_name(const char *name)
{}

TreeNode *get_node_by_index(const int index) {

    TreeNode *tmpNode = NULL;
    TreeNode *root = NULL;

    if (true != check_node_index(index)) {
        printf("invalied index\n");
        return NULL;
    }

    root = g_hiertree->root;
    tmpNode = get_node(root, index);
