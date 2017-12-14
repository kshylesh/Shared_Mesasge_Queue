


all : 		shared_msg_queue_ops.c shared_msg_queue_tester.c
		gcc -Wall -o shared_msg_queue shared_msg_queue_ops.c shared_msg_queue_tester.c -lm -pthread

clean:
	rm  shared_msg_queue
