#define _GNU_SOURCE
#include<stdlib.h>
#include<time.h>
#include "driver.h"
#include<stdio.h>
#include<math.h>
#include<unistd.h>
#include<errno.h>
#include <linux/input.h>
#include<sys/types.h>
#include <fcntl.h>
#include<semaphore.h>
#include<sched.h>
#include<pthread.h>

#define MOUSEFILE "/dev/input/event2"                   /*Path to read to get the mouse events*/
#define MAX 1000                                        /*max no. of interval entries tocalculate mean and standard deviation*/
#define CPU_CLOCKS_PER_MILLISECOND 400000              /*Frequency of CPU of board in milliseconds*/

sem_t sem1,sem2;                                        /*semaphores to control execution of aperiodic threads*/
int global_msg_count=0;                                 /*To hold the unique message id*/
const int p_period_mul[]={12,32,18,28};                 /*periodicity of periodic sending threads*/
const int r_period_mul =40;                             /*periodicity of periodic receiver threads*/
pthread_mutex_t lock1=PTHREAD_MUTEX_INITIALIZER;        /*mutex to protect the increment of unique message id*/
pthread_mutex_t lock2=PTHREAD_MUTEX_INITIALIZER;        /*mutex to protect the double click event detection flag*/
double temp[2]={0,0};                                   /*to hold the time intervals for left and right double click detection*/
int double_click=0,rx_count=0;                          /*double click flag and receive count respectively*/
double sum=0,sum_d=0,average=0,standard_deviation=0;
double interval[MAX];                                   /*Array to hold the message's time of residence in queue*/
pthread_t thread_id[7];                                 /*7 threads*/
int fdq[2];                                            /*To hold file descriptors of two queues*/
int cpu=1;                                              /*To set affinity to cpu 1*/
cpu_set_t cpuset;                                       /*A buffer that holds masking value for cpu affinity*/


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

void* periodic_sender(void *arg)                        /*Thread function for periodic sending threads*/
{
    struct timespec next,period;
    clock_gettime(CLOCK_MONOTONIC,&next);               /*stores present time into 'next structure'*/
    int *src;
    int ret1;
    src=(int*)arg;                                      /*source id of thread is upacked*/
    period.tv_sec=0;
    period.tv_nsec=p_period_mul[*src]*base_period*1000; /*periodicity is stored in 'period' structure*/
    struct msg *m;
    m=(struct msg*)malloc(sizeof(struct msg));         /*allocates the memory for message*/
    
    while(!double_click)                                /*run until there is no double click*/
    {   if(rx_count>MAX)                                /*exit if maximum receive has been acheived*/
        break;
        
        m->src_id=*src;                                    /*puts the source id*/
        m->rand=rand_num(rand()%41+10);                     /*gets random PI value for iterations between 10-50*/
        m->enq_t=0;                                         
        pthread_mutex_lock(&lock1);
        m->msg_id=++global_msg_count;                       /*Message id is assigned uniquely*/
        pthread_mutex_unlock(&lock1);
        ret1=write(fdq[(*src)%2],m,sizeof(struct msg));                       /*message is written to their pertaining queue*/
        if(ret1<0)
            {
                errno=EINVAL;
                                           
            }
            else
        printf("Message enqueued by periodic with thread id : %lu \n",pthread_self());
    
    
    if((next.tv_nsec+period.tv_nsec)>=1000000000)       /*To avoid the overflow of field that contains the nanoseconds*/
            {
                next.tv_nsec=(next.tv_nsec+period.tv_nsec)%1000000000;
                next.tv_sec++;
             }
    else
        next.tv_nsec+=period.tv_nsec;
        clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&next,0); /*sleep till the absolute time period is reached*/
    }
    free(m);
    printf("periodic sender thread with thread id %lu exiting...\n",pthread_self());
    pthread_exit(0);
}        


 void* aperiodic_sender(void *arg)              /*Thread function for aperiodic sending threads*/
{
    int *src;
    int ret2;
    src=(int*)arg;                              /*source id of thread is upacked*/
    struct msg *m;
    m=(struct msg*)malloc(sizeof(struct msg));
    
    while(1)
    {   
        
        if(thread_id[0]==pthread_self())
        {
        sem_wait(&sem1);                        /*If thread 1 is running decrement it,s semaphore else the thread 2's.*/
       
        }
        else
        sem_wait(&sem2);
        if((double_click)|| (rx_count>MAX))         /*Run until double or max receive has been acheived*/
        break;
        
        m->src_id=*src;
        m->rand=rand_num(rand()%41+10);
        m->enq_t=0;
        pthread_mutex_lock(&lock1);
        m->msg_id=++global_msg_count;
        pthread_mutex_unlock(&lock1);
        ret2=write(fdq[(*src)%2],m,sizeof(struct msg)); 
        if(ret2<0)
         {       printf("%s click,Aperiodic Thread %lu Dropped message as queue was full \n",(pthread_self()==thread_id[0])?"right":"left",pthread_self());
                 errno=EINVAL;
                 
                 
        }
        else
        printf("Message enqueued by aperiodic with thread id : %lu \n",pthread_self());
        
    }
        
     
    free(m);
    printf("aperiodic sender thread with thread id %lu exiting...\n",pthread_self());
    pthread_exit(0);
 }


void* periodic_receiver(void *arg)                              /*Thread function for periodic receiving thread*/
{

    struct timespec next,period;
    clock_gettime(CLOCK_MONOTONIC,&next);
    period.tv_sec=0;
    period.tv_nsec=(*(( int*)arg))*base_period*1000;
    struct msg *m;
    m=(struct msg*)malloc(sizeof(struct msg));
    int ret3,i=0;
    while(!(double_click)||lseek(fdq[0],0,SEEK_CUR)||lseek(fdq[1],0,SEEK_CUR)) /*run until double click or queue 1 is empty or queue 2 is empty so that all messages are dequeued even after double click*/
    {
            ret3=read(fdq[i++%2],m,sizeof(struct msg)); /*read from both the queues alternatively*/
            if(ret3<0)
            
                errno=EINVAL; /*return error value if queue is empty*/
            
            else
            {
            rx_count++; /* keep count of no. of messages received*/
            interval[rx_count-1]=(double)(m->dq_t)/CPU_CLOCKS_PER_MILLISECOND; /*store the interim period of messages*/
            printf("msg dequeued by periodic receiver thread with msg_id: %d random_num: %lf and interval: %lf ms\n",m->msg_id,m->rand,interval[rx_count-1]);
           
            
            }
           
            if((next.tv_nsec+period.tv_nsec)>=1000000000)
    {
        next.tv_nsec=(next.tv_nsec+period.tv_nsec)%1000000000;
        next.tv_sec++;
    }
    else
        next.tv_nsec+=period.tv_nsec;
         
        clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&next,0);
         if(i>5)
         i=1;   
    }
     free(m); /*free the message from the queue after dequeuing*/
     printf("periodic receiver thread with thread id %lu exiting...\n",pthread_self());
    pthread_exit(0);
    
  }
  
  
  int main(void)
{
    
    pthread_attr_t attr[7];  /*attributes of threads*/
    struct sched_param par[7];
    int thread_prior[]={70,71,81,82,83,84,91}; /*priorities of 2 aperiodic,4 periodic sender and 1 periodic receiver thread respectively*/
    int src_id[]={1,2,3,4,5,6}; /*source id of all sending threads*/
    const int *src_id_addr[]={&src_id[0],&src_id[1],&src_id[2],&src_id[3],&src_id[4],&src_id[5]};
    int ret,i;
    int fd; /*file descriptor for opening mouse device file*/
    struct input_event ie; /*structure to retreive info about event*/
   
   /*use the below method to set affinity only if using dual booted or pure linux machine as vbox runs on virtual cpu at times*/ 
    //CPU_ZERO(&cpuset);     
    //CPU_SET(cpu,&cpuset);  /*sets the mask value of cpu affinity*/
   
   /*file descriptors of queues are set to operate on */ 
   printf("Opening files\n");
    fdq[0]=open("/dev/dataqueue0",O_RDWR);
    fdq[1]=open("/dev/dataqueue1",O_RDWR);
    
    /*Initializing the attribute,setting affinity,setting priority,setting policy and creating the periodic threads by passing their source id as parameter to thread function 'periodic_sender'*/
    
    for(i=0;i<4;i++)
    {
        ret=pthread_attr_init(&attr[2+i]);
        //ret=pthread_attr_setaffinity_np(&attr[2+i],sizeof(cpu_set_t),&cpuset); /*used only on dual boot or pure linux machine*/
        ret=pthread_attr_getschedparam(&attr[2+i],&par[2+i]);
        par[2+i].sched_priority=thread_prior[2+i];
        ret=pthread_attr_getschedparam(&attr[2+i],&par[2+i]);
        ret=pthread_attr_setschedpolicy(&attr[2+i],SCHED_FIFO);
        ret=pthread_create(&thread_id[2+i],&attr[2+i],periodic_sender,(void*)src_id_addr[2+i]);
        if(ret)
        {
            fprintf(stderr,"error creating thread : %d\n",ret);
            exit(EXIT_FAILURE);
        }
        
     }
     
     
      /*Initializing the attribute,setting affinity,setting priority,setting policy and creating the aperiodic threads by passing their source id as parameter to thread function 'aperiodic_sender'*/
     for(i=0;i<2;i++)
    {
        ret=pthread_attr_init(&attr[i]);
        //ret=pthread_attr_setaffinity_np(&attr[i],sizeof(cpu_set_t),&cpuset); /*used only on dual boot or pure linux machine*/
        ret=pthread_attr_getschedparam(&attr[i],&par[i]);
        par[i].sched_priority=thread_prior[i];
        ret=pthread_attr_getschedparam(&attr[i],&par[i]);
        ret=pthread_attr_setschedpolicy(&attr[i],SCHED_FIFO);
        
        ret=pthread_create(&thread_id[i],&attr[i],aperiodic_sender,(void*)src_id_addr[i]);
        if(ret)
        {
            fprintf(stderr,"error creating thread : %d\n",ret);
            exit(EXIT_FAILURE);
        }
        
     }  
     
        /*Initializing the attribute,setting affinity,setting priority,setting policy and creating the periodic receiver thread by passing their periodicity(as source id is known) as parameter to thread function 'periodic_receiver'*/
        ret=pthread_attr_init(&attr[6]);
        //ret=pthread_attr_setaffinity_np(&attr[6],sizeof(cpu_set_t),&cpuset); /*used only on dual boot or pure linux machine*/
        ret=pthread_attr_getschedparam(&attr[6],&par[6]);
        par[6].sched_priority=thread_prior[6];
        ret=pthread_attr_getschedparam(&attr[6],&par[6]);
        ret=pthread_attr_setschedpolicy(&attr[6],SCHED_FIFO);
        ret=pthread_create(&thread_id[6],&attr[6],periodic_receiver,(void*)&r_period_mul);
        if(ret)
        {
            fprintf(stderr,"error creating thread : %d\n",ret);
            exit(EXIT_FAILURE);
        }
        
    /*initializing the semaphores to handle right and left clicks control execution*/ 
     sem_init(&sem1,0,0);
     sem_init(&sem2,0,0);
     
  /*opening the mouse device file to read the event data*/
    if((fd = open(MOUSEFILE, O_RDONLY)) == -1) {
        perror("opening device");
        exit(EXIT_FAILURE);
    }

    while(read(fd, &ie, sizeof(struct input_event))) {
        if(ie.type==1&&ie.code==273&&ie.value==1) /*represents the data corresponding to right click*/
        
        {   
        
           if((ie.time.tv_sec+(ie.time.tv_usec/1000000)-temp[0])<=0.5) /*check if two clicks are made succesfully within 500ms*/
           {        pthread_mutex_lock(&lock2);
                    double_click=1; /*if condition is satisfied set the double click flag*/
                    pthread_mutex_unlock(&lock2);
                    /*post the semaphores so that it doesnt get blocked to check the double click flag in their execution context*/
                    sem_post(&sem1);
                    sem_post(&sem2);
                    
                    break; 
           }
           
                   
                    sem_post(&sem1); /*give permission to execute right click event handling code after the single click is detected*/
                    temp[0]=ie.time.tv_sec+(ie.time.tv_usec/1000000); /* store the click timing for comapring the next click timing to make the double click detection*/
           
             
        }
        else if(ie.type==1&&ie.code==272&&ie.value==1) /*represents the data corresponding to right click*/
        
        {   
          
           if((ie.time.tv_sec+(ie.time.tv_usec/1000000)-temp[1])<=0.5)
           {        pthread_mutex_lock(&lock2);
                    double_click=1;
                    pthread_mutex_unlock(&lock2);
                    sem_post(&sem2);
                    sem_post(&sem1);
                    break; 
           }
                  
                    sem_post(&sem2); /*give permission to execute right click event handling code after the single click is detected*/
                    temp[1]=ie.time.tv_sec+(ie.time.tv_usec/1000000);/* store the click timing for comapring the next click timing to make the double click detection*/
           
           
             
        }
        
            
}
if(double_click)
printf("Out from detection\n");
for(i=0;i<7;i++) /*waiting for all the threads to finish*/
    pthread_join(thread_id[i],NULL);
     
    
    for(i=0;i<rx_count;i++)
    sum+=interval[i];
    
    average=sum/rx_count; /*average*/
    
    for(i=0;i<rx_count;i++)
    sum_d+=pow(interval[i]-average,2);
    
    standard_deviation=sum_d/rx_count; /*standard deviation*/
    
    close(fd);
    close(fdq[0]);
    close(fdq[1]);
    
    printf("average : %lf ms\t ,std_deviation : %lf ms\n",average,standard_deviation);
    
        
        return 0;
    }
