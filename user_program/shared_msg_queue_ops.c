#include"shared_queue.h"
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<pthread.h>
#include<math.h>
double rand_num(double iter) /*Function to generate random PI values */
{
    double  i, j;     
    double f;           
    double pi = 1;
      
     
    
   for(i = iter; i > 1; i--)
    {
   
        f = 2;
        for(j = 1; j < i; j++)
            {
                f = 2 + sqrt(f);
            }
      f = sqrt(f);
      pi = pi * f / 2;
   }
   pi *= sqrt(2) / 2;
   pi = 2 / pi;
   return pi;
}



static __inline__ unsigned long long rdtsc(void)                    /*inline assembly function to return time-stamp counter value (64bit)*/
{                                                                   /*gives no.of cycles since boot-time till the triggered point*/
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

struct queue* sq_create()
{
	struct queue *q=(struct queue*)malloc(sizeof(struct queue));    /*allocates the memory from heap for data queue */ 
	q->front=0;                                                     /*The front,rear positions are assigned zero*/
	q->rear=0;
	q->full=0;                                                      /*since queue is just created the full indication is unset*/
	pthread_mutex_init(&(q->lock),NULL);                            /*lock initialization to make serialized entry into queue*/
	return q;                                                       /*returning the reference to the created queue*/
}

int sq_write(struct msg *m1,struct queue *q1)
{
	pthread_mutex_lock(&(q1->lock));                                /*locking the queue before any message is enqueued*/
	if(q1->front==q1->rear && q1->full)                             /*condition to check if the queue is full*/
	{   
	    
		pthread_mutex_unlock(&(q1->lock));                          /*release the lock before returning if the queue is full*/
		return -1;
	}
	
	q1->entry[q1->front]=m1;                                        /*message is placed to the data queue*/
	q1->entry[q1->front]->enq_t=rdtsc();                            /*enqueued time(in terms of TSC) is placed to the pertaining message*/
	q1->front++;                                                    /*data enqueue position is incremented for new message*/
	q1->front %= def_length;                                        /*Ensures the queue wraps around*/
	
	if(q1->front==q1->rear)
	{
	    q1->full=1;                                                 /*a conditioned  is reached where queue is full*/
	    
	}
	pthread_mutex_unlock(&(q1->lock));                              /*releases the lock after successfull enqueue operation*/
	return 0;

}

int sq_read(struct msg **m1,struct queue *q1)               
{
	pthread_mutex_lock(&(q1->lock));                        /*locking the queue before any message is dequeued*/
	if(q1->front==q1->rear && !q1->full)                    /*condition to check if the queue is empty*/
	{   
	    *m1=NULL;                                                                                                       
		pthread_mutex_unlock(&(q1->lock));                  /*release the lock before returning if the queue is empty*/
		
		return -1;
	}
	
	q1->entry[q1->rear]->dq_t=rdtsc()-q1->entry[q1->rear]->enq_t; /*The amount of time the message was in data queue is set here*/
	*m1=q1->entry[q1->rear];                                       /*The reference of the message is stored in a pointer provided by user*/
	q1->rear++;                                                    /*data dequeue position is incremented */
	q1->rear %= def_length;                                        /*Ensures the queue wraps around*/
	if(q1->front==q1->rear)
	q1->full=0;
	pthread_mutex_unlock(&(q1->lock));                             /*releases the lock after successfull dequeue operation*/
	return 0;

}

void sq_delete(struct queue *q1)
{
	free(q1);                                                   /*freeing the entire queue data structure*/
}
