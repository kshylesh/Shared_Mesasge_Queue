#define def_length 10 /*defaultnumber of entries in queue*/
#define base_period 1000 /*base periodicity*/

/*structure of message that needs to be enqueued inside queue*/
struct msg
{
	int msg_id; /*message ID of message*/
	int src_id; /*source id of thread that enqueues the message*/
	double rand;/*holds the random PI value estimation*/
	unsigned long long enq_t;/*The time at which the enqueuing is done*/
	unsigned long long dq_t; /*holds the time interval for which the message was in queue*/
};

struct queue
{
	struct msg *entry[def_length]; /*list of messages*/
	int front;/*front position from where the message is dequeued*/
	int rear;/*rear position from where the message is enqueued*/
	int full;/*flag to indicate whether the queue is full or not*/
	
	
};

double rand_num(double iter);/*function to generate a random PI value*/


