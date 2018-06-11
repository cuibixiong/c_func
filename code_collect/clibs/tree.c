#include <stdio.h>
#include <string.h>

#include "common.h"
#include "tree.h"

int NAME_TO_INDEX(HierNode *parent_node, char *name)
{
	unsigned int i ;

	for (i = 0; i < vector_size(g_hiertree->nodeIndexs); ++i) {
		int index = *(int*)vector_at(g_hiertree->nodeIndexs, i);
		HierNode *node = _get_node_by_index(parent_node, index);
		if ((node != NULL) && strcmp(node->data.name, name) == 0)
			return index;
	}

	return -1;
}

static int check_node_index(const int index)
{
	unsigned int i ;

	if (index == -1)
		return false;

	for (i = 0; i < vector_size(g_hiertree->nodeIndexs); ++i) {
		if (*(int*)vector_at(g_hiertree->nodeIndexs, i) == index) {
			return true;
		}
	}

	return false;
}

HierNode *_get_node_by_index(HierNode *node, int index)
{
	HierNode *nodeinfo = NULL;
	if (index == -1)
		return NULL;

	if (NULL != node) {
		if (node->index == index) {
			nodeinfo = node;
			return nodeinfo;
		}

		//search child node
		nodeinfo = _get_node_by_index(node->child, index);
		//search slibling node
		if (nodeinfo == NULL)
			nodeinfo = _get_node_by_index(node->sibling, index);
	}

	return nodeinfo;
}

HierNode *get_node_by_index(int index)
{
	HierNode *tmpNode = NULL;
	HierNode *root = NULL;

	if (true != check_node_index(index)) {
		printf("invalied index\n");
		return NULL;
	}

	root = g_hiertree->root;
	tmpNode = _get_node_by_index(root, index);

	return tmpNode;
}

HierNode* set_child(int parentIndex, int index, NodeData data)
{
	HierNode *parent_node = NULL;
	HierNode *new_node = NULL;
	HierNode *head = NULL;
	HierNode *last_child = NULL;

	if (true != check_node_index(parentIndex)) {
		printf("%s:%d invalied parent index %d \r\n", __func__, __LINE__, parentIndex);
		return NULL;
	}

	parent_node = get_node_by_index(parentIndex);

	if (NULL == parent_node) {
		printf("%s:%d can't get node by Index :%d \r\n", __func__, __LINE__, parentIndex);
		return NULL;
	}

	new_node = (HierNode *)malloc(sizeof(HierNode));

	if (NULL == new_node) {
		printf("%s:%d malloc new Node failed \r\n", __func__, __LINE__);
		return NULL;
	}

	memset(new_node, 0, sizeof(HierNode));
	new_node->index = index;
	new_node->data.id = data.id;
	new_node->data.reg = data.reg;
	strcpy(new_node->data.name, data.name);

	vector_push_back(g_hiertree->nodeIndexs, index);
	g_hiertree->num++;

	if (NULL == parent_node->child) {
		parent_node->child = new_node;
		return new_node;
	}

	if (NULL == parent_node->sibling) {
		parent_node->sibling = new_node;
		return new_node;
	}

	head = parent_node->sibling;

	while (head) {
		last_child = head;
		head = head->sibling;
	}

	last_child->sibling = new_node;

	return new_node;
}

HierNode* set_root(NodeData data)
{
	HierNode *new_node = NULL;
	new_node = (HierNode *)malloc(sizeof(HierNode));

	if (NULL == new_node) {
		printf("%s:%d malloc new Node failed \r\n", __func__, __LINE__);
		return NULL;
	}

	memset(new_node, 0, sizeof(HierNode));
	new_node->index = 0;
	new_node->child = NULL;
	new_node->sibling = NULL;
	new_node->data.id = data.id;
	strcpy(new_node->data.name, data.name);

	vector_push_back(g_hiertree->nodeIndexs, 0);

	g_hiertree->num++;
	g_hiertree->root = new_node;

	return new_node;
}

void hiertree_init()
{
	g_hiertree = CALLOC(HierTree);
	g_hiertree->num = 0;
	g_hiertree->rootIndex = 0;
	g_hiertree->root = NULL;
	g_hiertree->nodeIndexs = create_vector(int);
	return;
}

void traversal(HierNode *node)
{
	if (NULL != node) {
		printf("TreeNode data id name :%s\n", node->data.name);
		traversal(node->child);
		traversal(node->sibling);
	}

	return;
}

void dump() {
	traversal(g_hiertree->root);
	return;
}

#if 0
void create() {

	NodeData data;
	memset(&data, 0, sizeof(NodeData));

	init();

	data.id = 0;
	strncpy(data.name, "root", sizeof(data.name) - 1);

	set_root(data);

	memset(&data, 0, sizeof(NodeData));
	data.id = 1;
	strncpy(data.name, "root-child-1", sizeof(data.name) - 1);

	set_child(0, 1, data);

	memset(&data, 0, sizeof(NodeData));
	data.id = 2;
	strncpy(data.name, "root-child-2", sizeof(data.name) - 1);

	set_child(0, 2, data);

	memset(&data, 0, sizeof(NodeData));
	data.id = 3;
	strncpy(data.name, "root-child-3", sizeof(data.name) - 1);

	set_child(0, 3, data);

	memset(&data, 0, sizeof(NodeData));
	data.id = 4;
	strncpy(data.name, "root-child-1-1", sizeof(data.name) - 1);

	set_child(1, 4, data);

	memset(&data, 0, sizeof(NodeData));
	data.id = 5;
	strncpy(data.name, "root-child-1-2", sizeof(data.name) - 1);

	set_child(1, 5, data);

	return;
}

void main()
{
	create();
	dump();
}
#endif
