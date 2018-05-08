/*************************************************************************
	> File Name: CountingNetwork.c
	> Author: ma6174
	> Mail: ma6174@163.com 
	> Created Time: 2012년 11월 26일 (월) 오전 01시 42분 42초
 ************************************************************************/

#include"CountingNetwork.h"
#define interative 10000
#define net_width 8
void *Func();
int wire[1024];
struct Bitonic bitonic;
int main(int argc,char *argv[])
{
	int thread_cnt=atoi(argv[1]);
	int i,sum;
//	int thread_cnt=1;
	pthread_t thread[thread_cnt];
	init_bitonic(&bitonic,net_width);
	for(i=0;i<thread_cnt;i++)
	{
		pthread_create(&thread[i],NULL,Func,NULL);
	}
	for(i=0;i<thread_cnt;i++)
	{
		pthread_join(thread[i],NULL);
	}
	for(i=0;i<net_width;i++)
	{
		sum+=wire[i];
		printf("wire[%d]:%d\n",i,wire[i]);
	}
	printf("sum of wires\n",sum);
	printf("sum:%d*%d=%d\n",thread_cnt,interative,thread_cnt*interative);
	return 0;
}
void *Func()
{	
	int i,input,output;
	for(i=0;i<interative;i++)
	{
		input=rand()%net_width;
		output=traverse_bitonic(&bitonic,input);
		__sync_fetch_and_add(&wire[output],1);	
	}
/*	struct Merger m;
	init_merger(&m,8);
	output=traverse_merger(&m,0);
	printf("output of Func:%d\n",output);
	output=traverse_merger(&m,1);
	printf("output of Func:%d\n",output);
	output=traverse_merger(&m,2);

	printf("output of Func:%d\n",output);
	output=traverse_merger(&m,3);
	printf("output of Func:%d\n",output);
	output=traverse_merger(&m,4);
	printf("outout of Func:%d\n",output);
	output=traverse_merger(&m,5);
	printf("output of Func:%d\n",output);
	output=traverse_merger(&m,6);
	printf("output of Func:%d\n",output);

*/}
int traverse_balancer(struct Balancer *balancer)
{
	pthread_mutex_lock(&(balancer->lock_b));
	if(balancer->toggle==true)
	{
		balancer->toggle=false;
		pthread_mutex_unlock(&(balancer->lock_b));
		return 0;
	}
	else
	{
		balancer->toggle=true;
		pthread_mutex_unlock(&(balancer->lock_b));
		return 1;
	}
}
void init_balancer(struct Balancer *b)
{
	b->toggle=true;
}
void init_merger(struct Merger *m,int myWidth)
{
	int i,j;
	m->width=myWidth;
    struct Balancer balancer[INFI];
	for(i=0;i<(m->width)/2;i++)
	{
		init_balancer(&balancer[i]);
		m->layer[i]=&(balancer[i]);
	}
	if(m->width>2)
	{
		 struct Merger merger[2];
		for(j=0;j<2;j++)
		{ 
			m->half[j]=&merger[j];
			init_merger(m->half[j],myWidth/2);
		}
	}
}
int traverse_merger(struct Merger * m,int input)
{
	int output=0;
	if(m->width==2)
	{
		output=traverse_balancer(m->layer[0]);
		return output;
	}
	else
	{
		if(input<(m->width)/2)
		{
			output=traverse_merger(m->half[input%2],input/2);

		}
		else
		{
			output=traverse_merger(m->half[1-input%2],input/2);

		}
	}
			return (2*output)+traverse_balancer(m->layer[output]);
}
int init_bitonic(struct Bitonic *b,int myWidth)
{
	int i;
	 struct Merger m;
	b->width=myWidth;
	b->merger=&m;
	init_merger(b->merger,b->width);
	if(b->width>2)
	{
	    struct Bitonic bt[2];
		for(i=0;i<2;i++)
		{
			b->half[i]=&bt[i];
			init_bitonic(b->half[i],(b->width)/2);
		}
	}

}
int traverse_bitonic(struct Bitonic *b,int input)
{
	int output=0;
	int input_merger=0;
	if(b->width>2)
	{
		output=traverse_bitonic(b->half[input/((b->width)/2)],input/2);
	}
//	input_merger=(input>=(b->width)/2?(b->width)/2: 0)+output;
	input_merger=(input>=output/2?output/2: 0)+output;
	return traverse_merger(b->merger,input_merger);
}
