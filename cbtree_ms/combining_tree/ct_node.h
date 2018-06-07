#ifndef CT_NODE
#define CT_NODE

#include <stdbool.h>
#include <pthread.h>
#include "ct_main.h"

#define node_lock(node)         pthread_mutex_lock(&(node->mutex));
#define node_unlock(node)       pthread_mutex_unlock(&(node->mutex));
#define node_wait(node)         pthread_cond_wait(&(node->cond), &(node->mutex));
#define node_wakeup(node)       pthread_cond_signal(&(node->cond));

#define CACHE_LINE_SIZE         64

enum cstatus_t {
    IDLE,       // 아무도 없음
    FIRST,      // 한 쓰레드 도착
    SECOND,     // 두 쓰레드 도착
    RESULT,     // combine이 끝났을때의 상태, distribute까지 끝나면 idle로
    ROOT        // 
};

struct node_t{
    bool locked;
    char locked_pad[CACHE_LINE_SIZE - sizeof(bool)];
    enum cstatus_t status;
    char status_pad[CACHE_LINE_SIZE - sizeof(enum cstatus_t)];
    int first_value;
    char first_value_pad[CACHE_LINE_SIZE - sizeof(int)];
    int second_value;
    char second_value_pad[CACHE_LINE_SIZE - sizeof(int)];
    int result;
    char result_pad[CACHE_LINE_SIZE - sizeof(int)];
    struct node_t *parent;
    char parent_pad[CACHE_LINE_SIZE - sizeof(struct node_t *)];
    pthread_cond_t cond;
    char cond_pad[CACHE_LINE_SIZE - sizeof(pthread_cond_t)];
    pthread_mutex_t mutex;
    char mutex_pad[CACHE_LINE_SIZE - sizeof(pthread_mutex_t)];
};

void node_init(struct node_t *node, struct node_t *parent, int no);

#endif