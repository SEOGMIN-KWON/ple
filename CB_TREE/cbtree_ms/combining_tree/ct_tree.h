#ifndef CT_TREE
#define CT_TREE

#include "ct_main.h"

struct node_t *_nodes;
struct node_t **tree_init(int width);
unsigned long long get_and_increment(struct node_t **stack, struct node_t *leaf);
int get_result();
#endif