/* TO DO: whether all exists */
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



void node_lock(){
	pthread_mutex_lock(&nodelock);
}

void node_unlock(){
	ptrhead_mutex_unlock(&nodelock);
}

void wait(pthread_cond_t* cond, pthread_mutex_t* cond_mutex){
	ptrhead_cond_wait(cond, cond_mutex);
}

void notify_all(pthread_cond_t* cond){
	pthread_cond_broadcast(cond);
}


int getAndIncrement(int my_id){

	Stack* path = (Stack *)malloc(sizeof(Stack));
	init_stack(path);

	Node* myleaf = leadf[my_id / 2];
	Node* node = myleaf;

	node.precombine = precombine();

	while(node.precombine){
		node = node.parent;
	}
	Node* stop = node;

	/* combinie phase */
	node = myleaf;
	int combined = COMBINE_NUM;
	node.combine = combine();

	while(node != stop){
		combined = node.combine(combined);
		path.push(node);
		node = node.parent;
	}

	/* operation phase */
	stop.op = op();
	int prior = stop.op(combined);

	/* distribution phase */
	node.distribute = distribute();
	while(!isEMPTY(path)){
		node = pop(path);
		node.distribute(prior);
	}
	return prior;
}


bool precombine(){
	node_lock();
	while(locked){
		wait(&cond, &nodelock);
	}

	while(cStatus == RESULT){}

	switch(cStatus){
		case IDLE:
			cStatus = FIRST;
			node_unlock();
			return true;

		case FIRST:
			locked = true;
			cStatus = SECOND;
			node_unlock();
			return false;

		case ROOT:
			node_unlock();
			return false;

		default: 
			char* temp = print_s(cStatus);
			printf("precombine unexpected node state %s on node %d \n",temp ,node.id);
			node_unlock();
			return false;
	}
}

int node combine(int combined){

	node_lock();
	while(locked){
		wait(&cond, &nodelock);
	}
	locked = true;

	firstValue = combined;

	switch(cStatus){
		case FIRST:
			node_unlock();
			return fistValue + secondValue;

		default:
			printf("combine unexpected Node state \n");
			node_unlock();
			return -1;
	}

}

int op(int combined){
	node_lock();
	switch(cStatus){
		case ROOT:
			int prior = result;
			result += combined;
			node_unlock();
			return prior;

		case SECOND:
			secondValue = combined;
			locked = false;
			notify_all(&cond);
			while(cStatus != RESULT){
				wait(&cond_result, &nodelock);
			}

			cStatus = IDLE;
			locked = false;
			notify_all(&cond);
			node_unlock();
			return result;

		default:
			char* temp = print_s(cStatus);
			printf("op unexpected node state %s on node %d \n",temp, node.id);
			node_unlock();
			return -1;
	}
}
