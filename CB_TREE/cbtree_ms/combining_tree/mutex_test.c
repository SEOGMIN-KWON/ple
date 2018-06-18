#include <pthread.h>
#include <stdio.h>
#include <time.h>

pthread_mutex_t mutex;
pthread_cond_t cond;
pthread_t th;

void *thmain(void *arg){
    /* before lock */
    printf("before lock: %d\n", mutex.__data.__lock);
    
    pthread_mutex_lock(&mutex);

    /* after lock */
    printf("after lock: %d\n", mutex.__data.__lock);

    printf("lock sleep 2s\n");
    sleep(2);
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond);

}

int main(int argc, char *argv[]){
    // unsigned long long int t;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    // pthread_create(&th, NULL, thmain, NULL);
    
    printf("%d\n", sizeof(pthread_cond_t));
    printf("%d\n", sizeof(pthread_mutex_t));
    printf("%d\n", sizeof(pthread_t));
    // sleep(1);
    // t = time(NULL);
    // printf("%llu\n", t);
    // pthread_cond_wait(&cond, &mutex);
    // t = time(NULL) - t;
    /* after unlock */
    printf("after unlock: %d\n", mutex.__data.__lock);
    
    
    return 0;
}
