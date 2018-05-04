/* TODO: whether all exists */
#include <pthread.h>
//#include <atomic>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "stack.h"
//#include "combiningtree.h"

#define COMBINE_NUM 1
#define ROUND 100000000 /* convert enum to string */
#define print_s(x) #x

int thread_num;
struct CombiningTree* tree = NULL;

struct timeval start;
struct timeval finish;

/* execute the combiningTree */
int getAndIncrement(int my_id){

//	Stack* path = (Stack *)malloc(sizeof(Stack));
	Stack path;
	init_stack(&path);
	struct Node* myleaf = tree->leaf[my_id/2];
	struct Node* node = myleaf;

	while(precombine(node)){
		node = node->parent;
	}
	struct Node* stop = node;

	/* combinie phase */
	node = myleaf;
	int combined = COMBINE_NUM;

	while(node != stop){
		combined = combine(node,combined);
		push(&path,node);
		node = node->parent;
	}

	/* operation phase */
	int prior = op(stop,combined);

	/* distribution phase */
	while(!isEMPTY(&path)){
		node = pop(&path);
		distribute(node,prior);
	}
	return prior;
}

/* Main function of Thread */
// TODO: need to interpret
void* getAndInc_thread(void* ptr){
	
	struct Args* arg = (struct Args*) ptr;
	int result;
	//printf("arg->id %d\n", arg->id);
	int i;
	for(i = 0; i < ROUND / thread_num; i++){
		getAndIncrement(arg->id);
		/*
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
		*/
	}
	return NULL;
}

int main(){
	double runtime;

	pthread_t threads[64];
	struct Args arg[64];
	unsigned long diff;

	printf("Thread num		TIME		ROUND		Combined_Num		RESULT \n");


	for(thread_num = 4; thread_num < 5; thread_num = thread_num *2){
		printf("	 %d		 ", thread_num);

		int i;
		for(i = 0; i < thread_num; i++){
			arg[i].id = i;
		}
	//	printf("finish\n");
		tree = make_combiningTree(thread_num);
		// TODO: is it okay?
		gettimeofday(&start, NULL); //start time
		int t;
		for(t = 0; t < thread_num; t++){
			int ret = pthread_create(&threads[t], NULL, &getAndInc_thread, (void *)&arg[t]);
			if(ret){
				printf("create thread error\n");
			}
		}
		int k;
		for(k = 0; k < thread_num; k++){
			int ret = pthread_join(threads[k], NULL);
		}

		/* TODO: is it okay? */
		gettimeofday(&finish, NULL); //finish time
		diff = 1000000 * (finish.tv_sec - start.tv_sec) + finish.tv_usec - start.tv_usec;

		printf("%dus	%d		%d		",diff, ROUND, COMBINE_NUM);

		/*
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
		*/
		
		printf("%d\n",tree->nodes[0]->result);
		for(t = 0 ; t < thread_num-1; t++){
			free(tree->nodes[t]);
		}
		free(tree);
	}
	return 0;
}


