#include "common.h"
#include "cstl/cvector.h"

#define MAX_NAME_LEN (50)

typedef struct _NodeData
{
    int id;
    char name[MAX_NAME_LEN];
} NodeData;

typedef struct _TreeNode
{
    int index;
    NodeData data;
    struct _TreeNode *child;
    struct _TreeNode *sibling;
} TreeNode;

typedef struct _HierTree
{
    int num;
    vector_t *nodeIndexs;
    int rootIndex;
    TreeNode *root;
} HierTree;

HierTree *g_hiertree;

extern int NAME_TO_INDEX(HierNode *node, char *name);
extern void hiertree_init();
extern HierNode *set_root(NodeData data);
extern HierNode *set_child(int parentIndex, int index, NodeData data);
extern HierNode *get_node_by_index(int index);
extern HierNode *_get_node_by_index(HierNode *node, int index);
extern void traversal(HierNode *tree);
