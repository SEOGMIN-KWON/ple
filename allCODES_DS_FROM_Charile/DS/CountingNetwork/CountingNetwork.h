/*************************************************************************
	> File Name: CountingNetwork.h
	> Author: ma6174
	> Mail: ma6174@163.com 
	> Created Time: 2012년 11월 26일 (월) 오전 01시 56분 38초
 ************************************************************************/
#include<stdio.h>
#include<malloc.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#define true 1
#define false 0
#define INFI 128
struct Balancer
{
	int toggle;
	pthread_mutex_t lock_b;
};
struct Merger
{
	struct Merger *half[2];
	struct Balancer *layer[INFI];
	int width;
};
struct Bitonic
{
	struct Bitonic *half[2];
	struct Merger *merger;
	int width;

};
int traverse_balancer(struct Balancer *balancer);

void init_balancer(struct Balancer *b);
void init_merger(struct Merger *m,int myWidth);

int traverse_merger(struct Merger * m,int input);
int init_bitonic(struct Bitonic *b,int myWidth);
int traverse_bitonic(struct Bitonic *b,int input);
