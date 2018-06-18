#include <stdio.h>
#include <stdlib.h>

#include "ct_main.h"
#include "ct_tree.h"
#include "ct_node.h"


int get_result(){
    return _nodes[0].result;
}

struct node_t **tree_init(int width){
    struct node_t **leaves;
    int len_nodes, len_leaves;
    int i;

    len_nodes = width - 1;
    _nodes = (struct node_t *)malloc(sizeof(struct node_t) * len_nodes);
    node_init(&_nodes[0], NULL, 0); // initialize root node 
    for(i=1; i<len_nodes; i++){
        node_init(&_nodes[i], &_nodes[(i-1)/2], i); // parent: position of parent in binary tree
    }

    len_leaves = (width+1)/2;
    leaves = (struct node_t **)malloc(sizeof(struct node_t *) * len_leaves);
    for(i=0; i<len_leaves; i++){
        leaves[i] = &_nodes[len_nodes - i - 1];
    }
    return leaves;
}

// synchronized operation
bool precombine(struct node_t *node){
    node_lock(node);

    while(node->locked){
        node_wait(node);
    }

    switch(node->status){
        case IDLE:
            node->status = FIRST;
            node_unlock(node);
            return true;
        case FIRST:
            node->locked = true;
            node->status = SECOND;
            node_unlock(node);
            return false;
        case ROOT:
            node_unlock(node);
            return false;
        default:
            printf("%s unexpected node state %d\n", __func__, node->status);
            exit(-1);
    }
}

//synchronized operation
int combine(struct node_t *node, int combined){
    int ret;
    node_lock(node);
    while(node->locked){
        node_wait(node);
    }
    node->locked = true;
    node->first_value = combined;

    switch(node->status){
        case FIRST:
            ret = node->first_value;
            node_unlock(node);
            return ret;
        case SECOND:
            ret = node->first_value + node->second_value;
            node_unlock(node);
            return ret;
        default:
            printf("%s unexpected node state %d\n", __func__, node->status);
            exit(-1);
    }
}

//sychronized operation
int op(struct node_t *node, int combined){
    int prior;
    node_lock(node);
    switch(node->status){
        case ROOT:
            prior = node->result;
            node->result += combined;
            node_unlock(node);
            return prior;
        case SECOND:
            node->second_value = combined;
            node->locked = false;
            node_wakeup(node);
            while(node->status != RESULT){ 
                node_wait(node);
            }
            node->locked = false;
            node_wakeup(node);
            node->status = IDLE;
            prior = node->result;
            node_unlock(node);
            return prior;
        default:
            printf("%s unexpected node state %d\n", __func__, node->status);
            exit(-1);
    }
}

//synchronized operation
void distribute(struct node_t *node, int prior){
    node_lock(node);

    switch(node->status){
        case FIRST:
            node->status = IDLE;
            node->locked = false;
            break;
        case SECOND:
            node->result = prior + node->first_value;
            node->status = RESULT;
            break;
        default:
            printf("%s unexpected node state %d\n", __func__, node->status);
            exit(-1);
    }
    node_wakeup(node);
    node_unlock(node);
}

unsigned long long get_and_increment(struct node_t **stack, struct node_t *myleaf){
    struct node_t *node, *stop;
    int combined, prior, top;

    node = myleaf;
    top = 0;

    /* precombining phase */
    while(precombine(node)){
        node = node->parent;
    }
    stop = node;

    /* combining phase */
    node = myleaf;
    combined = INC_VALUE;
    while(node != stop){
        combined = combine(node, combined);
        stack[top++] = node;
        node = node->parent;
    }

    /* operation phase */
    prior = op(stop, combined);
    
    /* distribution phase */
    while(top != 0){
        node = stack[--top];
        distribute(node, prior);
    }

    return prior;
}
