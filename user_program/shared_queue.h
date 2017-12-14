#define def_length 10           /*defaultlength of the queue*/
#define base_period 1000        /*minimum base period for periodicity of threads*/
#include<pthread.h>

struct msg                      /*structure of message*/
{
	int msg_id;                 /*holds message ID*/
	int src_id;                 /*holds the source ID of sending thread*/
	double rand;                /*holds random PI estimation */
	unsigned long long enq_t;   /*holds the enqueue time of the message*/
	unsigned long long dq_t;    /*holds the difference of time between enqueue & dequeue of the message*/
};

struct queue                            /*queue structure*/ 
{
	struct msg *entry[def_length];      /*An array of message structure representing the queue with a default size*/   
	int front;                          /*represents the position where new message needs to be enqueued*/
	int rear;                           /*represents the position from where  message needs to be dequeued*/
	int full;                           /*Indicates whether the queue is full or not */
	pthread_mutex_t lock;               /*Mutex lock to make the serailized entry into the queue*/
	
};

double rand_num(double iter);                           /*function to generate random PI estimations*/
struct queue* sq_create();                              /*function to create a queue data structure*/
int sq_write(struct msg *m1,struct queue *q1);          /*function to enqueue a message to a particular data queue*/
int sq_read(struct msg **m1,struct queue *q1);          /*function to dequeue a message from a particular data queue*/
void sq_delete(struct queue*);                          /*function to delete a particular data queue and free the memory*/

