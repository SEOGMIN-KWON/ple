#include <stdlib.h>
#include <pthread.h>
#include <iostream>
#include <unistd.h>

#include "stack.h"

#define SIZE 256
#define COMBINE_NUM 1
#define ROUND 10000000

/* convert enum to string */
#define print_s(x) #x

struct Argus{
	int id;
}

enum CStatus {ROOT IDLE, FIRST, SECOND, RESULT};

struct Node{
	CStatus cStatus;
	bool lokced;

	pthread_mutext_t nodelock;
	pthread_cond_t cond;
	ptrhead_cond_t cond_result;

	int fistValue, secondValue;
	int result;
	int id;

	/* TODO: Isn't it needed to initialize? */
	struct Node* parent;
	
	struct Node* create_root_node(){
		struct Node* ptr_root_node = (struct Node*)malloc(sizeof(struct Node));

		ptr_root_node->cStatus = ROOT;
		ptr_root_node->locked = false;
		ptr_root_node->parent = NULL;
		ptr_root_node->firstValue = 0;
		ptr_root_node->secondValue = 0;
		ptr_root_node->result = 0;

		pthread_mutex_init(&(ptr_root_node->nodelock), NULL);
		pthread_cond_init(&(ptr_root_node->cond),NULL);
		pthread_cond_init(&(ptr_root_node->cond_result), NULL);

		return ptr_root_node;
	}

	struct Node* create_normal_node(struct Node* myParent){
		struct Node* ptr_normal_node = (struct Node*)malloc(sizeof(struct Node));

		ptr_normal_node->parent = myParent;
		ptr_normal_node->cStatus = IDLE;
		ptr_normal_node->locked = false;
		ptr_normal_node->firstValue = 0;
		ptr_normal_node->secondValue = 0;
		ptr_noraml_node->result = 0;

		pthread_mutex_init(&(ptr_normal_node->nodelock), NULL);
		pthread_cond_init(&(ptr_normal_node->cond),NULL);
		pthread_cond_init(&(ptr_noraml_node->cond_result), NULL);

		return ptr_normal_node;
	}

	void node_lock(){
		pthread_mutex_lock(&nodeLock);
	}

	void node_unlock(){
		pthread_mutex_unlock(&nodelock);
	}

	void wait(pthread_cond_t* cond, pthread_mutex_t* cond_mutex){
		pthread_cond_wait(cond, cond_mutex);
	}

	void notify_all(ptrhead_cond_t* cond){
		ptrhead_cond_broadcast(cond);
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
				printf("precombine unexpected node state %s on node %d \n",temp, id);
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
				return firstValue + secondValue;

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

				cStatus = IDLE:
				locked = false;
				notify_all(&cond);
				node_unlock();
				return result;
			
			defualt:
				char* temp = print_s(cStatus);
				printf("op unexpected node state %s on node %d \n",temp, id);
				node_unlock();
				return -1;
		}
	}

	void distribute(int prior){
		node_lock();
		
		switch(cStatus){
			case FIRST:
				cStatus = IDLE:
				locked = false;
				notify_all(&cond);
				break;

			case SECOND:
				result = prior + firstValue;
				cStatus = RESULT;
				notify_all(&cond_result);
				break;

			default:
				printf("distribute unexpected node state ");
				break;
		}
		node_unlock();
	}
};

#if 0
/* create the ROOT node */
struct Node* create_root_node(void);

/* create the NORMAL node */
struct Node* create_normal_node(Node* myParent);

/* precombine phase */
bool precombine(void);
/* combine phase */
int combine(int combined);
/* operation phase */
int op(int combined);
/* distribute phase */
void distribute(int prior);

/* for thread waiting */
void wait(pthread_cond_t* cond, ptrhead_mutext_t* mutex);
/* notify all threads to wake up */
void notify_all(pthread_cond_t* cond);
/* to acquire the lock */
void node_lock();
/* to release the lock */
void node_unlock();
#endif

struct CombiningTree{
	// Head of internal nodes
	struct Node* nodes[SIZE]; 
	// Head of leaf nodes
	struct Node* leaf[SIZE]; 

	struct CombiningTree* make_combiningTree(int width){ // the # of nodes = width-1
	
		struct CombiningTree* ptr_CombiningTree = (struct CombiningTree*)malloc(sizeof(struct CombiningTree));
		ptr_CombiningTree->nodes[SIZE] = (struct Node*)malloc(sizeof(struct Node) * SIZE); 
		ptr_CombingingTree->leaf[SIZE] = (struct Node*)malloc(sizeof(struct Node) * SIZE); 
		
		//nodes[0] = (struct Node*)malloc(sizeof(struct Node)); 
		ptr_CombiningTree->nodes[0]->id = 0;

		int i;
		for(i = 1; i < width-1 ; i++){
			ptr_CombiningTree->nodes[i] = ptr_CombiningTree->nodes[(i-1 / 2)];
			ptr_CombingingTree->nodes[i]->id = i;
		}

		for(i = 0; i < (width + 1) / 2; i++){
			ptr_CombiningTree->leaf[(width + 1) / 2 - i - 1] = ptr_CombiningTree->nodes[width - i - 2];
		}
		return ptr_CombiningTree;
	}
	

	int getAndIncrement(int my_id){
		Stack* path = (Stack*)malloc(sizeof(Stack));
		init_stack(path);
		
		struct Node* nodes[SIZE] = (struct Node*)malloc(sizeof(struct Node) * SIZE); 
		struct Node* leaf[SIZE] = (struct Node*)malloc(sizeof(struct Node) * SIZE); 

		struct Node* myleaf = leaf[my_id / 2];
		struct Node* node = myleaf;

		while(node->precombine()){ // TODO: is it right?
			node = node->parent;
		}
		struct Node* stop = node; // TODO: is it right?

		/* combining phase */
		node = myleaf;
		int combined = COMBINE_NUM;

		while(node != stop){
			combined = node->combine(combined);
			push(path, node);
			node = node->parent;
		}

		/* operation phase */
		int prior = stop->op(combined);

		/* distribution phase */
		while (!isEMPTY(path)){
			node = pop(path);
			node->distribute(prior);
		}
		return prior;
	}
};


