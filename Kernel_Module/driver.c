#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/device.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<asm/uaccess.h>
#include<linux/init.h>
#include<linux/types.h>
#include<linux/slab.h>
#include<linux/time.h>
#include<linux/string.h>
#include<linux/jiffies.h>
#include<linux/semaphore.h>
#include"driver.h"
#define DEVICE_NAME "squeue" /*Device name that appears in /proc/devices with major number assigned*/
#define CPU_CLOCKS_PER_MILLISECOND 400000 /*Board CPU is 400MHz */
static dev_t dev_num;                      /*holds the major and minor numbers assigned */
static struct class *que_class;            /*holds a reference to the class to which the shared queue belongs to */ 

/*Structure of the device to be created*/
static struct shared_msg_queue              
{
    struct queue que;
    struct cdev que_cdev;     /*cdev embedded to make it operate as character device*/
    struct semaphore que_sem; /*semaphore to make a serialized entry to the queue*/
    char *que_name;            /*holds the name of the device, just for displaying purpose and debugging*/
}*que_device[2];              /*holds the reference to the two device queues*/

static __inline__ unsigned long long tsc(void) /*function to get thhe time stamp counter value*/
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

int que_open(struct inode *que_inode,struct file *que_file) /*invoked when the device file is opened*/
{
    struct shared_msg_queue *que_dev;
    que_dev=container_of(que_inode->i_cdev,struct shared_msg_queue,que_cdev); /*gets the parent structure*/
    que_file->private_data=que_dev;     /*Puts the reference of the device structure to private field so that it can be retreived later*/
    printk(KERN_INFO "%s called  %s function\n",que_dev->que_name,__FUNCTION__);
    return 0;
}

int que_close(struct inode *que_inode,struct file *que_file) /*invoked when the device file is closed*/
{
   struct shared_msg_queue *que_dev=que_file->private_data; /*gets the reference of the device structure from its specific file structure*/
   printk(KERN_INFO "%s called  %s function\n",que_dev->que_name,__FUNCTION__);
   return 0;
}

/*function invoked when a write is called on device file */
ssize_t que_write(struct file *que_file,const char __user *m,size_t length,loff_t *pos) 
{
    struct shared_msg_queue *que_dev=que_file->private_data;
    printk(KERN_INFO "Inside %s function of %s\n",__FUNCTION__,que_dev->que_name);
    down(&(que_dev->que_sem));                           /*get possesion of critical region by decrementing the binary semaphore*/
    if((que_dev->que.full)&&(que_dev->que.front==que_dev->que.rear)) /*condition to check if queue is full*/
    {   
        printk(KERN_INFO " %s is full\n",que_dev->que_name);
        up(&(que_dev->que_sem)); /*release the semaphore before returning error value so that nothing is blocked*/
        return -1;
    }
    
    que_dev->que.entry[que_dev->que.rear]=(struct msg*)kmalloc(sizeof(struct msg),GFP_KERNEL); /*allocation of memory to message at the rear end of the queue*/
    if(copy_from_user(que_dev->que.entry[que_dev->que.rear],m,sizeof(struct msg))!=0)/*To copy the buffer from user space to kernel space so that if user space buffer is swapped or invalid, this interface gives a return value to handle the situation properly*/
    {
        printk(KERN_INFO "Bad address while copying to kernel space\n"); 
        up(&(que_dev->que_sem));/*release the semaphore before returning error value so that nothing is blocked*/
        return -1;
     }
    que_dev->que.entry[que_dev->que.rear]->enq_t=tsc(); /*load the enqueue time stamp value*/
    
    que_dev->que.rear++;  /*increment the rear end*/
    que_dev->que.rear%=def_length; /*does the wrapping of the queue back to the first position after the max length is reached*/
    if(que_dev->que.front==que_dev->que.rear) 
        que_dev->que.full=1; /*set the flag after the queue is detected as full*/
    up(&(que_dev->que_sem)); /*release the semaphore before returning ,so that nothing is blocked*/
    printk(KERN_INFO "written to %s successfully\n",que_dev->que_name);
    return 0;
}    

/*function invoked when a read is called on device file */
ssize_t que_read(struct file *que_file,char __user *m,size_t length,loff_t *pos)
{
    struct shared_msg_queue *que_dev=que_file->private_data;
    printk(KERN_INFO "Inside %s function of %s\n",__FUNCTION__,que_dev->que_name);
    down(&(que_dev->que_sem));/*get possesion of critical region by decrementing the binary semaphore*/
    if(!(que_dev->que.full)&&(que_dev->que.front==que_dev->que.rear))/*condition to check if queue is empty*/
    {   
        printk(KERN_INFO " %s is empty\n",que_dev->que_name);
        up(&(que_dev->que_sem));/*release the semaphore before returning error value so that nothing is blocked*/
        return -1;
    }
    
    que_dev->que.entry[que_dev->que.front]->dq_t=tsc()-(que_dev->que.entry[que_dev->que.front]->enq_t);/*load the interval, the message was in queue before it is being dequeued in terms of time stamp counter value*/
    if(copy_to_user(m,que_dev->que.entry[que_dev->que.front],sizeof(struct msg))!=0)/*To copy the buffer from kernel space to user space so that if user space buffer is swapped or invalid ,this interface gives a return value to handle the situation properly*/
    
        {
        printk(KERN_INFO "Bad address while copying to user space\n"); 
        up(&(que_dev->que_sem));/*release the semaphore before returning error value so that nothing is blocked*/
        return -1;
     }    
    kfree(que_dev->que.entry[que_dev->que.front]); /*free the memory after dequeuing*/
    que_dev->que.front++;/*increment the front end*/
    que_dev->que.front%=def_length;/*does the wrapping of the queue back to the first position after the max length is reached*/
    if(que_dev->que.front==que_dev->que.rear)
        que_dev->que.full=0;/*unset the flag after the queue is detected as empty */
    up(&(que_dev->que_sem)); /*release the semaphore before returning ,so that nothing is blocked*/
    printk(KERN_INFO "read  from %s successfully\n",que_dev->que_name);
    return sizeof(struct msg);/*returns the number of bytes read*/
}   

/*function invoked to check if the queue is empty or not so that receiver thread will dequeue it till it becomes empty*/
loff_t que_seek(struct file *que_file,loff_t off,int seek)
{
     struct shared_msg_queue *que_dev=que_file->private_data;
     printk(KERN_INFO "Inside %s function:\n",__FUNCTION__);
     return (!((que_dev->que.front==que_dev->que.rear)&&!(que_dev->que.full)));/*returns true if queue is non-empty else false*/
}     

/*linking the device operations to file_operations structure by assigning the function pointers*/   
static struct file_operations queue_ops=
{
    .owner=THIS_MODULE,
    .open=que_open,
    .release=que_close,
    .write=que_write,
    .read=que_read,
    .llseek=que_seek,
};

/*function invoked when the module is loaded*/
int que_init(void)
{
    int i;
    printk(KERN_INFO "Inside %s function\n",__FUNCTION__);
    if(alloc_chrdev_region(&dev_num,0,2,DEVICE_NAME)<0) /*allocates the major and minor number dynamically*/
    {
        printk(KERN_INFO "unsuccessful major number allocation\n");
        return -1;
     }
   if((que_class=class_create(THIS_MODULE,DEVICE_NAME))==NULL) /*creates the class for device files*/
   {    
        printk(KERN_INFO "Unable to create class\n");
        unregister_chrdev_region(dev_num,2);
        return -1;   
   }
   for(i=0;i<2;i++)
   {
        que_device[i]=(struct shared_msg_queue*)kmalloc(sizeof(struct shared_msg_queue),GFP_KERNEL);/*allocates memory to our device structure*/
        if(!que_device[i])
        {
            printk(KERN_INFO "Cant allocate memory to data structure\n");
            return -ENOMEM;
         }
         /*initialzes the front,rear, full and semaphore associated with corresponding queues*/
         que_device[i]->que.front=0; 
         que_device[i]->que.rear=0;
         que_device[i]->que.full=0; 
         sema_init(&(que_device[i]->que_sem),1);
    }
    /*assignes a name to the queue for debugging purpose*/
    que_device[0]->que_name="shared_msg_queue0";
    que_device[1]->que_name="shared_msg_queue1";
    for(i=0;i<2;i++)
    {
        if(device_create(que_class,NULL,MKDEV(MAJOR(dev_num),MINOR(dev_num)+i),NULL,"dataqueue%d",i)==NULL) /*creates the info for udev so that it creates a device node with name  "dataqueue0" and "dataqueue1"*/
        {
            class_destroy(que_class);
            unregister_chrdev_region(dev_num,2);
            printk(KERN_INFO "Registration error\n");
            return -1;
         }
     }
     for(i=0;i<2;i++)
     {
        cdev_init(&(que_device[i]->que_cdev),&queue_ops);/*links the file operations with the character device*/
        que_device[i]->que_cdev.owner=THIS_MODULE; /*sets the owner of the device*/
        if(cdev_add(&(que_device[i]->que_cdev),MKDEV(MAJOR(dev_num),MINOR(dev_num)+i),1)==-1) /*adds the character device to the linked list maintained by kernel to traverse the character device*/
        {
            device_destroy(que_class,MKDEV(MAJOR(dev_num),MINOR(dev_num)+i));
            class_destroy(que_class);
            unregister_chrdev_region(dev_num,2);
            return -1;
         }
      }
      printk(KERN_INFO "Devices created\n");
      return 0;
  }

/*invoked when the module is unloaded*/  
void que_exit(void)
{
    int i;
    printk(KERN_INFO "Inside %s function\n",__FUNCTION__);
    for(i=0;i<2;i++)
    {
        cdev_del(&(que_device[i]->que_cdev));/*delete the character device from linked list*/
        device_destroy(que_class,MKDEV(MAJOR(dev_num),MINOR(dev_num)+i));/*destroy the node*/
        kfree(que_device[i]);/*free the memory allocated to the device structure*/
    }
    class_destroy(que_class);/*remove the class to which the queue belong*/
    unregister_chrdev_region(dev_num,2); /*unregister the major and minor numbers allocated*/  
 }
 module_init(que_init);/*registers the init function*/
 module_exit(que_exit);/*resgisters the exit function*/
 MODULE_LICENSE("GPL");/*sets the license to GPL so that all exported symbol of GPL compliant can be used by this module*/
        
                
              
