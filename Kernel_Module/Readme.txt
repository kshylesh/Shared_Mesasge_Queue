----------------------------------------------------------
DHEEMANTH BYKERE MALLIKARJUN-1213381271
KARTHIK SHYLESH KUMAR -1213169085
----------------------------------------------------------
List of files
1.driver.c 
2.driver_tester.c
3.driver.h 

----------------------------------------------------------------
Steps to run program on host machine
----------------------------------------------------------------
1.Change the mouse event file to appropriate event.
2.Change thhe cpu ticks in driver.h file to 1700000 (already changed in the queue_driver_board)
3.Source code for host is located in source code->queue_driver_host
4.Use makefile provided with "make all" command to create modules.
5.Install driver.ko module using the command insmod driver.ko
6.Create an exectuatble file using the command "gcc -o output_file_name ./driver_tester.c -lm -pthreads"
7.Run the executable file using command "sudo output_file_name"

----------------------------------------------------------------
Steps to run program on Board
----------------------------------------------------------------
1.Change the mouse event file to appropriate event (event2)
2.Change thhe cpu ticks in driver.h file to 400000 (already changed in the queue_driver_board).
3.Source code for host is located in source code->queue_driver_board.
4.Use the makefile (for machine) provided with "make all" command to create modules.
5.Copy the all files to galileo board using command "scp -r root@<IPaddress> folder_to_copy"
6.Install driver.ko module using the command insmod driver.ko
7.Run the module file "./driver.o"

----------------------------------------------------------------
Steps to debug ring buffer in host
----------------------------------------------------------------
1.After instering module open a new terminal.
2.Type sudo dmesg -c to clear ring buffer.
3.Type sudo dmesg -w to wait for debug messages.
4.Run the excecute file in another terminal.
