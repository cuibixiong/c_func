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

static HierTree *g_hiertree;
