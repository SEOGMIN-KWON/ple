#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "ct_node.h"
#include "ct_main.h"
#include "ct_tree.h"

void node_init(struct node_t *node, struct node_t *parent, int no)
{
    if(parent){
        node->parent = parent;
        node->status = IDLE;
    } else {
        node->parent = NULL;
        node->status = ROOT;
    }
    node->first_value = 0;
    node->second_value = 0;
    node->result = 0;
    node->locked = false;
    pthread_cond_init(&(node->cond), NULL);
    pthread_mutex_init(&(node->mutex), NULL);
}
