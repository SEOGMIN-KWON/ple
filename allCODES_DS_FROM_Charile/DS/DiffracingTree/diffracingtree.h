/*************************************************************************
	> File Name: diffracingtree.h
	> Author: charlie_chen
	> Mail: charliecqc@dcslab.snu.ac.kr 
	> Created Time: Wed 28 Nov 2012 12:45:44 PM KST
 ************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<time.h>
#include<sys/time.h>
#define INFI 100
#define true 1
#define false 0
#define interative 100
struct Slot
{
	pthread_t thread;
	int stamp;
	pthread_mutex_t lock;


};
struct Exchanger
{
	 int EMPTY;
	 int WAITING;
	 int BUSY;
	 struct Slot *slot;
};
struct Balancer
{
	pthread_mutex_t mutex;
	int toggle;
};
struct Prism
{
	struct Exchanger *exchanger[INFI];
};
struct DiffractingBalancer
{
	struct Prism *prism;
	struct Balancer balancer;
};
struct DiffractingTree
{
	int width;
	struct DiffractingBalancer *root;
	struct DiffractingTree *left;
	struct DiffractingTree *right;
};
struct Arg
{
	int width;
	int contension;
};
void init_diffractingtree(struct DiffractingTree *t,int mySize);
int traverse_diffractingtree(struct DiffractingTree *t,int widthi,long c);
int traverse_diffractingbalancer(struct DiffractingBalancer * d,int wi,long c);
int visit_prism(struct Prism *p,int WIDTH,long c);
void init_slot(struct Slot *s,pthread_t thread,int stamp);
void init_prism(struct Prism *p,int size);
struct Slot *get(struct Slot *get);
int set(struct Slot *s,pthread_t id,int n_stamp);
int CAS(struct Slot *s,struct Slot *oldval,struct Slot *newval);
void init_diffractingtreebalancer(struct DiffractingBalancer *db,int size);
void init_exchanger(struct Exchanger *e);
pthread_t exchange(struct Exchanger *e,pthread_t myItem,long timeout);
void init_balancer(struct Balancer *b);
int traverse_balancer(struct Balancer *b);
