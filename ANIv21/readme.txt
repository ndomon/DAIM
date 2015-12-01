'Stable' version of DAIM

To start the DAIM controller running with Mininet follow these steps:

Step 1: download the DAIM controller distribution and compile with the following commands:

~$ gcc -c ani.cpp com.cpp con.cpp linked.cpp

~$ gcc ani.o com.o con.o linked.o -o aniv21 -lstdc++ -lpthread

Step 2: change directory to the executable file and enter ./aniv21 -h to see the documented arguments. 

~$ cd ANIv21/dist/Debug/GNU-Linux-x86/

~/ANIv21/dist/Debug/GNU-Linux-x86$ ./aniv21 -help

Step 3: start a default Mininet topology with one switch and two hosts to talk to the DAIM controller:

~$ sudo mn --controller remote

This command will create a network with a connection to the remote controller at 127.0.0.1:6633

Step 4: run a simple learning switch using a DAIM controller framework:

~/ANIv21/dist/Debug/GNU-Linux-x86$ ./aniv21 -p 6633

***Note that every OpenFlow switch in the network must be connected and managed by at least one DAIM controller. 
