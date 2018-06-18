#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include <atomic>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>
#include <stack>
#include "combiningtree.h"

using namespace std;

#define COMBINE_NUM 1
#define ROUND 10000000
int threadnum;


CombiningTree ctree1(2);
CombiningTree ctree2(4);
CombiningTree ctree4(8);
CombiningTree ctree8(16);
CombiningTree ctree16(32);


struct timeval start;
struct timeval finish;

void Node::node_lock()
{
	pthread_mutex_lock(&nodelock);
}

void Node::node_unlock()
{
	pthread_mutex_unlock(&nodelock);
}

void Node::wait(pthread_cond_t *cond, pthread_mutex_t *cond_mutex)
{
	pthread_cond_wait(cond, cond_mutex);
}

void Node::notify_all(pthread_cond_t *cond)
{
	pthread_cond_broadcast(cond);
}

int CombiningTree::getAndIncrement(int my_id)
{
	stack<Node *> path;
	Node *myleaf = leaf[my_id / 2];
	Node *node = myleaf;
	while (node->precombine()) {
		node = node->Parent;
	}
	Node *stop = node;

	// combine phase
	node = myleaf;
	int combined = COMBINE_NUM;

	while (node != stop) {
		combined = node->combine(combined);
		path.push(node);
		node = node->Parent;
	}

	// operation phase
	int prior = stop->op(combined);

	//distribution phase
	while (!path.empty()) {
		node = path.top();
		path.pop();
		node->distribute(prior);
	}
	return prior;
}

//p265
bool Node::precombine()
{
	node_lock();
	while (locked) {
		wait(&cond, &nodelock);
	}

	while (cStatus == RESULT) {}

	switch (cStatus) {
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
		cout << " precombine unexpected Node state " << cStatus << "on node " << this->id << endl;
		node_unlock();
		return false;
	}
}

//p266
int Node::combine(int combined)
{
	node_lock();
	while (locked) {
		wait(&cond, &nodelock);
	}
	locked = true;
	firstValue = combined;
	switch (cStatus) {
	case FIRST:
		node_unlock();
		return firstValue;

	case SECOND:
		node_unlock();
		return firstValue + secondValue;

	default:
		cout << "combine unexpected Node state " << endl;
		node_unlock();
		return -1;
	}
}

//p266
int Node::op(int combined)
{
	node_lock();
	switch (cStatus) {
	case ROOT: {
		int prior = result;
		result += combined;
		node_unlock();
		return prior;
	}

	case SECOND: {
		secondValue = combined;
		locked = false;
		notify_all(&cond);
		while (cStatus != RESULT) {
			wait(&cond_result, &nodelock);
		}

		cStatus = IDLE;
		locked = false;
		notify_all(&cond);
		node_unlock();
		return result;
	}

	default: {
		cout << "op unexpected Node state " << cStatus << " on node " << this->id << endl;
		node_unlock();
		return -1;
	}
	}
}

//p267
void Node::distribute(int prior)
{
	node_lock();
	switch (cStatus) {
	case FIRST:
		cStatus = IDLE;
		locked = false;
		notify_all(&cond);
		break;

	case SECOND:
		result = prior + firstValue;
		cStatus = RESULT;
		notify_all(&cond_result);
		break;

	default:
		cout << "distribute unexpected node state " << endl;
		break;
	}
	node_unlock();
}


void *GetandInc_wapper(void * ptr)
{
	struct Args *arg = (struct Args *)ptr;
	int ret;
	for (int i = 0; i < ROUND / threadnum; i++) {
		if(threadnum == 1)
			ret = ctree1.getAndIncrement(arg->id);
		else if(threadnum == 2)
			ret = ctree2.getAndIncrement(arg->id);
		else if (threadnum == 4)
			ret = ctree4.getAndIncrement(arg->id);
		else if (threadnum == 8)
			ret = ctree8.getAndIncrement(arg->id);
		else if (threadnum == 16)
			ret = ctree16.getAndIncrement(arg->id);
	}
	return NULL;
}

int main()
{
	double runtime;

	pthread_t threads[16];
	struct Args arg[16];
	unsigned long diff;

	cout << "Thread num     Time      Round      Combined_Num    Result " <<endl;
	for (threadnum = 1; threadnum < 17; threadnum = threadnum * 2)
	{
		
		cout << "     " << threadnum << "       ";
		for (int i = 0; i < threadnum; i++) {
			arg[i].id = i;
		}
		gettimeofday(&start, NULL);        //start time
		for (int t = 0; t < threadnum; t++) {
			int ret = pthread_create(&threads[t], NULL, &GetandInc_wapper, (void *)&arg[t]);
			if (ret) {
				cout << "create thread error" << endl;
			}
		}
		for (int k = 0; k < threadnum; k++) {
			int ret = pthread_join(threads[k], NULL);
		}

		gettimeofday(&finish, NULL);        //finish time
		diff = 1000000 * (finish.tv_sec - start.tv_sec) + finish.tv_usec - start.tv_usec;

		cout << diff  << "us" << "    " << ROUND << "          " << COMBINE_NUM << "         ";
		if (threadnum == 1)
			cout << ctree1.nodes[0]->result << endl;
		if (threadnum == 2)
			cout << ctree2.nodes[0]->result << endl;
		if (threadnum == 4)
			cout << ctree4.nodes[0]->result << endl;
		if (threadnum == 8)
			cout << ctree8.nodes[0]->result << endl;
		if (threadnum == 16)
			cout << ctree16.nodes[0]->result << endl;
	}
	return 0;
}
