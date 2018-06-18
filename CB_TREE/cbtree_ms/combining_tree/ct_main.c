#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#include "ct_main.h"
#include "ct_tree.h"
#include "ct_node.h"

pthread_t _threads[N_THREADS_MAX];
unsigned long long max;

void *ct_main(void *arg){
    struct node_t *leaf = (struct node_t*)arg;
    unsigned long long iter = max/_n_threads;
    struct node_t **stack = (struct node_t **)malloc(sizeof(struct node_t *)*8);
    while(iter--){
        get_and_increment(stack, leaf);
    }
    return NULL;
}

int _result;
void *sync_main(void *arg){
    // int tid = *((int*)arg);
    unsigned long long iter = max/_n_threads;
    while(iter--){
        __sync_fetch_and_add(&_result, INC_VALUE);
    }
    return NULL;
}


pthread_mutex_t mutex_mutex;
bool lock;
void *mutex_main(void *arg){
    unsigned long long v = max/_n_threads;
    while(v--){
        // while(!__sync_lock_test_and_set(&lock, true));
        pthread_mutex_lock(&mutex_mutex);
        _result += INC_VALUE;
        pthread_mutex_unlock(&mutex_mutex);
        // __sync_lock_release(&lock);
    }
    return NULL;
}

void test(void *func(void *)){
    int i;
    struct timespec ts1, ts2;
    unsigned long long t1, t2;

    if(func == ct_main){
        struct node_t **leaves = tree_init(_n_threads);

        clock_gettime(CLOCK_REALTIME, &ts1);
        for(i=0; i<_n_threads; i++){    
            pthread_create(&_threads[i], NULL, func, (void *)leaves[i/2]);
        }
    } else {
        _result = 0;
        pthread_mutex_init(&mutex_mutex, NULL);
        clock_gettime(CLOCK_REALTIME, &ts1);
        for(i=0; i<_n_threads; i++){    
            pthread_create(&_threads[i], NULL, func, NULL);
        }
    }
    
    for(i=0; i<_n_threads; i++){
        pthread_join(_threads[i], NULL);
    }
    
    clock_gettime(CLOCK_REALTIME, &ts2);
    
    t1 = ts1.tv_sec * 1000000000 + ts1.tv_nsec;
    t2 = ts2.tv_sec * 1000000000 + ts2.tv_nsec;

    printf("%lf\t", (double)(t2 - t1)/1000000);
}

int main(int argc, char *argv[]){
    // if(argc == 2)
        max = atoi(argv[1]);
    // else
        // max = MAX;
    
    int i;
    _n_threads=atoi(argv[2]);
    
    // for(; i<=N_THREADS_MAX; i+=2){
        // _n_threads=i;
        // printf("\nct_main %d threads\n", i);
        test(ct_main);
        test(mutex_main);
        // printf("\n");
    // }
    
    return 0;
}