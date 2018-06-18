#include <stdlib.h>
#include <pthread.h>
#include <iostream.h>

#define SIZE 256

struct Argus{
	int id;
}

enum CStatus {ROOT IDLE, FIRST, SECOND, RESULT};

struct Node{
	CStatus cStatus;

	bool lokced;
	pthread_mutext_t nodeLock;
	pthread_cond_t cond;
	ptrhead_cond_t cond_result;

	pthread_mutex_t 

	int fistValue, secondValue;
	int result;
	int id;

	Node* parent;
	
	struct Node* (*crate_root_node) (void);
	struct Node* (*create_noraml_node) (Node*);

	bool (*precombine) (void);
	int (*combine) (int);
	int (*op) (int);
	void (*distribute) (int);

	void (*wait) (pthread_cond_t*, ptrhead_mutext_t*);
	void (*notify_all) (pthread_cond_t* cond);
	void (*node_lock) (void);
	void (*node_unlock) (void);
};

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

struct CombiningTree{
	Node* nodes[SIZE]; // Head of internal nodes
	Node* leaf[SIZE]; // Head of leaf nodes

	struct CombiningTree* (*make_combiningTree) (int);
	
	int (*getAndIncrement) (int);
};

/* make CombiningTree */
struct CombiningTree* make_combiningTree(int num_of_nodes);

/* execute combiningTree*/
int (*getAndIncrement) (int id);
