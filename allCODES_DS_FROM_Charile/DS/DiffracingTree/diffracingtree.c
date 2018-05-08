/*************************************************************************
	> File Name: diffracingtree.c
	> Author: charlie_chen
	> Mail: charliecqc@dcslab.snu.ac.kr 
	> Created Time: Wed 28 Nov 2012 12:46:11 PM KST
 ************************************************************************/

#include"diffracingtree.h"
struct DiffractingTree dtree;
int wire[INFI];
struct Arg arg[INFI];
void *Func(void *a);
int main(int argc,char *argv[])
{
	if(argc!=4)
	{	
		printf("Usage:./drt [thread_num] [width] [contension]\n");
		return 1;
	}
	int cnt_thread=atoi(argv[1]);
//	int cnt_thread=8;
	int WIDTH=atoi(argv[2]);
//	int WIDTH=8;
	int contension=atoi(argv[3]);
//	int contension=1000;
	pthread_t thread[cnt_thread];
	int i,sum;
	init_diffractingtree(&dtree,WIDTH);
//	init_exchanger(&exchanger);
//	printf("in init_slots->thread:%u\n",exchanger.slot.thread);
	for(i=0;i<cnt_thread;i++)
	{
		arg[i].width=WIDTH;
		arg[i].contension=contension;
		pthread_create(&thread[i],NULL,Func,&arg[i]);
	}
	for(i=0;i<cnt_thread;i++)
		pthread_join(thread[i],NULL);
	for(i=0;i<WIDTH;i++)
	{
		printf("wire[%d]:sum:%d\n",i,wire[i]);
		sum+=wire[i];
	}
	printf("sum of wires:%d\n",sum);
	printf("thread_cnt:%d,interative:%d,sum%d\n",cnt_thread,interative,cnt_thread*interative);
	return 0;
}
void *Func(void *arg)
{
	int i,r,width;
	long c;
	struct Arg *a=(struct Arg *)arg;
	width=a->width;
	c=a->contension;
	for(i=0;i<interative;i++)
	{
		r=traverse_diffractingtree(&dtree,width,c);
		__sync_fetch_and_add(&wire[r],1);
	}
}
void init_slot(struct Slot *s,pthread_t t,int stamp)
{
	s->thread=t;
	s->stamp=stamp;
	pthread_mutex_init(&(s->lock),NULL);

}
struct Slot* get(struct Slot *s)
{
		pthread_mutex_lock(&(s->lock));
//	    printf("in get\n");
		struct Slot *x=(struct Slot *)malloc(sizeof(struct Slot));
		init_slot(x,s->thread,s->stamp);
		pthread_mutex_unlock(&(s->lock));
//		printf("x:%x\n",x);
		return x;
}
int set(struct Slot *s,pthread_t x,int nstamp)
{
	pthread_mutex_lock(&(s->lock));
	s->thread=x;
	s->stamp=nstamp;
	pthread_mutex_unlock(&(s->lock));
	return 0;
}
int CAS(struct Slot *s,struct Slot *new,struct Slot *old)
{
	pthread_mutex_lock(&(s->lock));
	int r;
//	printf("in cas\n");
//	printf("new->thread:%d,stamp:%ld\n",new->thread,new->stamp);
//	printf("old->thread:%d,stamp:%ld\n",old->thread,old->stamp);
	if(s->thread==old->thread && s->stamp==old->stamp)
	{
		s->thread=new->thread;
		r=true;
	}
	else
	{
		r=false;
	}
	pthread_mutex_unlock(&(s->lock));
	return r;
}
void init_exchanger(struct Exchanger *e)
{
	e->EMPTY=0;
	e->WAITING=1;
	e->BUSY=2;
	e->slot=(struct Slot *)malloc(sizeof(struct Slot));
	init_slot(e->slot,0,0);
}
pthread_t exchange(struct Exchanger *e,pthread_t myItem,long timeout)
{
	struct timeval tv;
	long timebound;
	gettimeofday(&tv,NULL);
	timebound=tv.tv_usec+timeout;
//	printf("timebound:%ld\n",timebound);
	while(1)
	{
		gettimeofday(&tv,NULL);
		if(tv.tv_usec>timebound)
		{
			return 0;//time out
		}
//		printf("hello worldaaaaa\n");
		struct Slot *atom=get(e->slot);
	//	printf("atomi->thread:%x\n",atom->thread);
		pthread_t init=atom->thread;
		int init_stamp=atom->stamp;
		if(init_stamp == 0)
		{
//			printf("doing init\n");
			struct Slot *ot=(struct Slot *)malloc(sizeof(struct Slot));
			init_slot(ot,myItem,1);
			int result=CAS(e->slot,atom,ot);
//			printf("result in line 138:%d\n",result);
			if(result)
			{
				gettimeofday(&tv,NULL);
				while(tv.tv_usec<timebound)
				{
		//			printf("hello world2\n");
					struct Slot *youItem=get(e->slot);
					if(youItem->stamp==2)
					{
						set(e->slot,0,0);
						return youItem->thread;
					}
				}
			//	printf("hello world3\n");
				int r2=CAS(e->slot,ot,atom);
				if(r2)
				{
					return 0;
				}
				else
				{
					struct Slot *wa=get(e->slot);
					pthread_t t=wa->thread;
					set(e->slot,0,0);
				//	printf("hello world4\n");
					return t;
				}
			}
			else
			{

			}
		}
		else if(init_stamp==1)
		{
			struct Slot *s=(struct Slot *)malloc(sizeof(struct Slot));
			init_slot(s,myItem,2);
			int r3=CAS(e->slot,atom,s);
			if(r3)
			{
			//	printf("hello world5\n");
				return atom->thread;
			}
			else
			{
			}

		}
		else if(init_stamp==2)
		{
		}
		else
		{
			printf("error\n");
			return -1;
		}
	}
//	printf("time out\n");
		return 0;	
}
void init_balancer(struct Balancer *b)
{
	pthread_mutex_init(&(b->mutex),NULL);
	b->toggle=true;
}
int traverse_balancer(struct Balancer *b)
{
	int r;
	pthread_mutex_lock(&(b->mutex));
	if(b->toggle==true)
		r=false;
	else
		r=true;
	b->toggle=r;
	pthread_mutex_unlock(&(b->mutex));
	return r;
}
void init_prism(struct Prism *p,int width)
{
	int i;
	for(i=0;i<width;i++)
	{
		p->exchanger[i]=(struct Exchanger *)malloc(sizeof(struct Exchanger));
		init_exchanger(p->exchanger[i]);
	}
}
int visit_prism(struct Prism *p,int WIDTH,long contension)
{
	pthread_t thread=pthread_self();
	int index=rand()%WIDTH;
//	printf("thread:%d in visit_prism\n");
	pthread_t ot=exchange(p->exchanger[index],thread,contension);
	if(ot==0)
	{
		return -1;
	}
	else
	{
		return (thread<ot);
	}
}
int traverse_diffractingbalancer(struct DiffractingBalancer * d,int w,long c)
{
	int r=visit_prism(d->prism,w,c);
//	printf("result of visit_prism:%d\n",r);	
	if(r==-1)
		return traverse_balancer(&(d->balancer));
	else
		return r;
}

void init_diffractingtree(struct DiffractingTree *t,int mySize)
{
	t->width=mySize;
	t->root=(struct DiffractingBalancer *)malloc(sizeof(struct DiffractingBalancer));
	init_diffractingtreebalancer(t->root,mySize);
	if(mySize>2)
	{
		struct DiffractingTree *a=(struct DiffractingTree *)malloc(sizeof(struct DiffractingTree));
		init_diffractingtree(a,mySize/2);
		t->left=a;
		struct DiffractingTree *b=(struct DiffractingTree *)malloc(sizeof(struct DiffractingTree));
		init_diffractingtree(b,mySize/2);
		t->right=b;
	}
	else
	{
		t->left=NULL;
		t->right=NULL;
	}
}
void init_diffractingtreebalancer(struct DiffractingBalancer *db,int size)
{
	db->prism=(struct Prism *)malloc(sizeof(struct Prism));
	init_prism(db->prism,size);

}
int traverse_diffractingtree(struct DiffractingTree *t,int width,long contension)
{
	int r=traverse_diffractingbalancer(t->root,width,contension);
//	printf("result of traverse_diffractingroot:%d\n",r);
	if(t->width<=2)
		return r;
	else
	{
//		printf("damn world\n");
		if(r==0)
			r=2*(traverse_diffractingtree((t->left),width/2,contension));
		else
			r=2*(traverse_diffractingtree((t->right),width/2,contension))+1;
	}
	return r;
}
