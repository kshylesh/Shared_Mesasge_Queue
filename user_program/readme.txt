TEAM26 ESP438
DHEEMANTH BYKERE MALLIKARJUN-1213381271
KARTHIK SHYSLESH KUMAR -     1213169085


List of Files and description

Module1:squeue.h : Header file consisting of message structure,queue structure, base period,length of ring buffer, and list of function header

Module2:shared_msg_queue_ops.c : C file implementing shared message queue functions (sq_create,sq_write, sq_read, sq_delete)along with random number generation.

Main Module:shared_msg_queue_tester.c :C file consisiting of main function, periodic thread creation function,  aperiodic thread creation function,receiver thread function, pi calculation and standard deviation calcuations.

------------------------------------------------------------
Steps for running the program on virutal machine
------------------------------------------------------------
1.Change #define MOUSEFILE  "/dev/input/event3" to appropriate event.
2.Get the executable file by running the makefile.
	a.make all :to build all files
	b.make clean:to clean all files.
3.Make the exexutable file run on single core cpu by setting the affinity to run on single core by using command sudo taskset -c 0 "outputfile/executable file". 

------------------------------------------------------------
Steps for running the program on dual boot system
------------------------------------------------------------
1.Change #define MOUSEFILE  "/dev/input/event3" to appropriate event.
2.Uncomment the following line of code in the Main Module:
  line 174://CPU_ZERO(&cpuset);     
  line 175://CPU_SET(cpu,&cpuset);  /*sets the mask value of cpu affinity*/
  line 186://ret=pthread_attr_setaffinity_np(&attr[2+i],sizeof(cpu_set_t),&cpuset); /*used only on dual boot or pure linux machine*/
  line 205://ret=pthread_attr_setaffinity_np(&attr[i],sizeof(cpu_set_t),&cpuset); /*used only on dual boot or pure linux machine*/
  line 222://ret=pthread_attr_setaffinity_np(&attr[6],sizeof(cpu_set_t),&cpuset); /*used only on dual boot or pure linux machine*/
3.Get the executable file by running the makefile.
	a.make all :to build all files
	b.make clean:to clean all files.
4.Run the executable file without setting the affinity.
