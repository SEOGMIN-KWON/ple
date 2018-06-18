/* TODO: whether all exists */
#include <pthread.h>
//#include <atomic>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>

#include "stack.h"
#include "combiningtree.h"

#define COMBINE_NUM 1
#define ROUND 10000000

/* convert enum to string */
#define print_s(x) #x

int thread_num;

/* TODO: is it right? */
struct CombiningTree* temp_ctree = NULL:

struct CombiningTree* ctree1 = temp_ctree.make_combiningTree(1);
struct CombiningTree* ctree2 = temp_ctree.make_combiningTree(2);
struct CombiningTree* ctree4 = temp_ctree.make_combiningTree(4);
struct CombiningTree* ctree8 = temp_ctree.make_combiningTree(8);
struct CombiningTree* ctree16 = temp_ctree.make_combiningTree(16);


/* TODO: measure the executing time */
struct timeval start;
struct timeval finish;

/* Main function of Thread */
// TODO: need to interpret
void* getAndInc_thread(void* ptr){
	
	struct Args* arg = (struct Args*) ptr;
	int result;

	int i;
	for(i = 0; i < ROUND / thread_num; i++){
		if(thread_num == 1)
			result = ctree1.getAndIncrement(arg->id);
		else if(thread_num == 2)
			result = ctree2.getAndIncrement(arg->id);
		else if(thread_num == 4)
			result = ctree4.getAndIncrement(arg->id);
		else if(thread_num == 8)
			result = ctree8.getAndIncrement(arg->id);
		else if(thread_num == 16)
			result = ctree16.getAndIncrement(arg->id);
	}
	return NULL:
}

int main(){
	double runtime;

	pthread_t threads[16];
	struct Args arg[16];
	unsigned long diff;

	printf("Thread num		TIME		ROUND		Combined_Num		RESULT \n");

	for(thread_num = 1; thread_num < 17; thread_num = thread_num *2){
		printf("	 %d		 \n", thread_num);

		int i;
		for(i = 0; i < thread_num; i++){
			arg[i].id = i;
		}
		// TODO: is it okay?
		gettimeofday(&start, NULL); //start time
		int t;
		for(t = 0; t < thread_num; t++){
			int ret = pthrad_create(&threads[t], NULL, &getAndInc_thread, (void *)&arg[t]);
			if(ret){
				printf("create thread error\n");
			}
		}
		int k;
		for(k = 0; k < thread_num; k++){
			int ret = ptrhead_join(threads[k], NULL);
		}

		/* TODO: is it okay? */
		gettimeofday(&finish, NULL); //finish time
		diff = 1000000 * (finish.tv_sec - start.tv_sec) + finish.tv_usec - start.tv_usec;

		printf("%dus	%d		%d		",diff, ROUND, COMBINE_NUM);

		if(thread_num == 1)
			printf("%d\n",ctree1.nodes[0]->result);
		if(thread_num == 2)
			printf("%d\n",ctree2.nodes[0]->result);
		if(thread_num == 4)
			printf("%d\n",ctree4.nodes[0]->result);
		if(thread_num == 8)
			printf("%d\n",ctree8.nodes[0]->result);
		if(thread_num == 16)
			printf("%d\n",ctree16.nodes[0]->result);
	}
	return 0;
}


