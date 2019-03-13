/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * OPC UA server implementation for control of multiple Nanotec motor controllers
 * ------------------------------------------------------------------------------
 *
 * This code is suitable for the following motor controller models
 * SMCI12, SMCI33, SMCI35, SMCI36, SMCI47-S, SMCP33, PD2-N, PD4-N and PD6-N
 *
 * The following code should be run in a command line terminal. The user will be given 
 * instructions and prompts for parameters to overwrite the default configurations of 
 * the motor controller.
 * 
 * Description: Code utilities
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * The following code will run an OPC UA server and establishes a communication with a 
 * predefined number of motor controllers either connected alltogether to a single serial
 * port or otherwise all connected to single serial ports.
 * 
 * Every motor controller is represented by an object instance which owns several callback methods
 * and datasources of the motor controllers parameters.
 * 
 * Notes on Compatibality  
 * ^^^^^^^^^^^^^^^^^^^^^^ 
 * The code was written on Linux (Ubuntu 18.04.2 LTS) and tested for Raspbian GNU/Linux 9.4 (stretch)
 * 
 * Warnings: Important notes for for initial use
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Connect everything properly according to the manufacturers technical manual. 
 * For inital use of a motor controller read and use motorcontroller_initial_setup.c 
 * to adjust the motorcontrollers configurations (e.g. phase current) to prevent damaging the hardware.*/

/**********************/
/* GLOBAL DEFINITIONS */
/**********************/

#define BAUDRATE B115200
#define READ_TIMEOUT_S 0		/* reading timeout in s */
#define READ_TIMEOUT_US 100000	/* reading timeout in us, should be greater than 100000 otherwise segmentation faults could happen with Read currently configured set callback method */
#define SIZE_MOTORADDR 3		/* 3 bytes, for the string reaches from "1" to "255" */
#define SIZE_TTYNAME 50
#define N_MOTORCONTROLLERS 3 	/* max. number of motor controllers */
#define N_PORTS 1				/* number of ports, either 1 or N_MOTORCONTROLLERS (1 if all motor controllers are connected to the same port or N if each controller is connected to a single port) */

/* following definitions are options which can be commented out */
#define DEFAULT_TTYNAME
#define DEFAULT_MOTORADDR
#define READ_RESPONSE
//#define READ_CONT				/* no purpose yet */
//#define WAIT_STATUS_READY		/* not implemented yet */
//#define NEW						/* for debugging to test new code snippets */

/**********************/
/* INCLUDED LIBRARIES */
/**********************/

#include <signal.h>
#include <ua_server.h>
#include <ua_config_default.h>
#include <ua_log_stdout.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h> 			/* lib for serial port communication */
#include <unistd.h>
#include <stdint.h>
#include <time.h>				/* clock() function */
#include <pthread.h>			/* allows multi-threading */
#include <sys/select.h>			/* defines select() */
#include <sys/types.h>	
#include <sys/time.h>			/* defines variable type timeval */

#include "sport.h" 				/* serial port communication */
#include "nanotec_smci_etc.h"	/* command list for step motor controllers from nanotec */

/*********************************/
/* FUNCTIONS AND TYPEDEFINITIONS */
/*********************************/

/* server stop handler */
UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

/* wait function */
static void delay(int milli_seconds) { 
    clock_t start_time = clock(); 						/* Stroing start time */
  
    while (clock() < start_time + milli_seconds*1000) 	/*looping till required time is not acheived */
        ; 
} 

	typedef struct {
		UA_ServerConfig *config;
		UA_Server *server ;
		UA_Boolean *running;
		UA_StatusCode *status;
		globalstructMC *global;
		fd_set *readfds;
		struct timeval *tv;
	} thread_args;

/***********/
/* THREADS */
/***********/
static void *threadServer(void *vargp){
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "thread threadServer started to run the server");
	thread_args *argp = (thread_args*)vargp;

	/* Run the server loop */
    UA_StatusCode status = UA_Server_run(argp->server, argp->running);
	*(argp->status) = status;
    UA_Server_delete(argp->server);
    UA_ServerConfig_delete(argp->config);
	return NULL;
}

static void *threadListen(void *vargp){
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "thread threadListen started to supervise port communication");
#ifdef READ_CONT 										/* reads continuously on one filedescriptor set WIP */
	thread_args *argp = (thread_args*)vargp;
	globalstructMC *glob = argp->global;
	char msg[80];
	int ready;
	printf("threadListen: fd = %d\n", (glob+0)->fd);
	while(1){
		ready = select(N_PORTS+1, argp->readfds, NULL, NULL, argp->tv);
		argp->tv->tv_sec = READ_TIMEOUT_S;
		if (ready > 0){
			pthread_mutex_lock((glob+0)->lock);
			printf("threadListen: mutex locked \n");
			sport_read_msg(msg, (glob+0)->fd);	/* read message */
			pthread_mutex_unlock((glob+0)->lock);
			printf("threadListen: mutex unlocked \n");
		}
		else if(ready == -1)
			printf("Error from select(): %s \n", strerror(errno));
//		else /* if ready == 0 */
//			printf("Timeout from select() after %d s\n", READ_TIMEOUT_S);
	}
#endif
	return NULL;
}

/*************/
/* MAIN LOOP */
/*************/
int main(int argc, char** argv){
	delay(1);

    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler); 								/* catch ctrl-c */

    /* Create a server listening on port 4840 */
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

	/* DECLARATIONS AND DEFINITIONS OF  GLOBAL VARIABLES */
	int i;														/* index for loops */
	char **nameid = NULL;
	char **namebrowse = NULL;
	char **namenr = NULL;

	UA_NodeId nodeIdMC[N_MOTORCONTROLLERS];						/* Array of NodeId's for every Motor Controller Object Instance */
	globalstructMC global[N_MOTORCONTROLLERS];					/* array of structs where each element is assigned to one object instace, defined in nanotec_smci_etc.h */

	/* declare thread IDs */
	pthread_t threadServerId;
	pthread_t threadListenId;
	pthread_mutex_t lock[N_PORTS];							/* one lock for every port */

	/* declare variables for select() to monitor multiple filedescriptors and prevent infinite loop during read() */
	fd_set readfds;
	fd_set readfds_single[N_MOTORCONTROLLERS];					/* array with all fds from readfds as single fd sets in an array */
	struct timeval tv;											/* set timeout for monitoring filedescriptors */
		tv.tv_sec = READ_TIMEOUT_S;								/* timeout in s */
		tv.tv_usec = READ_TIMEOUT_US;							/* timeout in microseconds */
	printf("tv_sec = %ld, tv_usec = %ld \n", tv.tv_sec, tv.tv_usec);

	/* define tty names for every object instance */
	for (i=0; i<N_MOTORCONTROLLERS; i=i+1){
		#ifdef DEFAULT_TTYNAME
			#if (N_PORTS == 1)									/* each motor controller connected to same port */
				strcpy(global[i].ttyname, "/dev/ttyUSB0");
			#endif
			#if (N_PORTS == N_MOTORCONTROLLERS) 				/* each motor controller connected to own port */
				char ttyname_tmp[SIZE_TTYNAME];
				sprintf(ttyname_tmp, "/dev/ttyUSB%d", i);
				strcpy(global[i].ttyname, ttyname_tmp);
			#endif
			#if (N_PORTS != N_MOTORCONTROLLERS && N_PORTS != 1)
				printf("N_PORTS = %d\n", N_PORTS);
				printf("irregular number of motorcontrollers (N_MOTORCONTROLLERS) or ports (N_PORTS)\n");
			#endif
		#endif
		#ifndef DEFAULT_TTYNAME
				printf("Enter device file name (e.g. \"/dev/ttyUSB0\" for MotorController%d: ", i+1);
				fgets(global[i].ttyname, SIZE_TTYNAME, stdin);
				global[i].ttyname[strcspn(global[i].ttyname, "\n")] = 0; 	/* delete the "\n" from fgets command at the end of the string */
		#endif
	}

	/* WIP declare lock for every ttyname */
	for (i=0; i<N_MOTORCONTROLLERS; i=i+1){
		#if (N_PORTS == 1)
			global[i].lock = &(lock[0]);
		#endif
		#if (N_PORTS == N_MOTORCONTROLLERS) 
			global[i].lock = &(lock[i]);
		#endif
		#if (N_PORTS != 1 & N_PORTS != N_MOTORCONTROLLERS)
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "defined global variables N_PORTS and N_MOTORCONTROLLERS don't fit together.");
			return UA_STATUSCODE_BADUNEXPECTEDERROR;
		#endif
	}
	

	/* Define motor addresses for every object instance */
	#ifndef DEFAULT_MOTORADDR
		printf("FYI: individual motor adresses for MotorController range from 1 to 254 \n");
		printf("FYI: broadcast motor adress is \"*\" \n");
	#endif
	for (i=0; i<N_MOTORCONTROLLERS; i=i+1){
		sprintf(global[i].motorAddr, "%d", i+1);
		#ifndef DEFAULT_MOTORADDR
			printf("FYI: individual motor adresses for MotorController range from 1 to 254 \n");
			printf("FYI: broadcast motor adress is \"*\" \n");
			printf("Enter motor adress for MotorController%d: ", i+1);
			fgets(global[i].motorAddr, SIZE_MOTORADDR, stdin);
			global[i].motorAddr[strcspn(global[i].motorAddr, "\n")] = 0; 	/* delete the "\n" from fgets command at the end of the string */
		#endif
	}
	printf("\n");

	/* clear variables */
	nameid = (char**)malloc(sizeof(**nameid)*N_MOTORCONTROLLERS);			/* pointer to string with name of later used motor controller obj instance nodeId's */
	namebrowse = (char**)malloc(sizeof(**nameid)*N_MOTORCONTROLLERS);		/* pointer to string with name of later used browsenames */
	namenr = (char**)malloc(sizeof(**nameid)*N_MOTORCONTROLLERS);			/* pointer to string with name of later used number to generate nodeId's name */
	FD_ZERO(&readfds);														/* clear file descriptor set */


	/* set filedescriptors, nodeIds for obj. instances, serial port connection configurations, */
	for (i=0; i<N_MOTORCONTROLLERS; i=i+1){
		/* set filedescriptors in fd-set and in array struct variable global */
		get_fd(global[i].ttyname, &(global[i].fd));							/* in future probably obsolet because it'll be also contained in global[i].readfds WIP */

		FD_ZERO(&readfds_single[i]);										/* WIP: clear file descriptor set */
		if(global[i].fd != -1){
			FD_SET(global[i].fd, &readfds_single[i]);						/* WIP: fd set of single fd for global struct assigend to a single motor controller object instance */
			FD_SET(global[i].fd, &readfds);									/* WIP: fd set of all fds, to be used in continously on all fds listening thread WIP!*/
		}
		global[i].readfds = &readfds_single[i];								/* WIP: */
		global[i].tv = &tv;													/* WIP: */

		/* declare NodeId's for every object instance of a motor controller */
		nameid[i] = (char*)malloc(sizeof(char*) * 10);
		namenr[i] = (char*)malloc(sizeof(char*) * 10);
		strcpy(*(nameid+i), "MCId");
		sprintf(*(namenr+i), "%d", i+1);
		strcat(*(nameid+i), *(namenr+i));
		nodeIdMC[i] = UA_NODEID_STRING(1,*(nameid+i));

		/* define browsenames for every object instance of a motor controller */
		namebrowse[i] = (char*)malloc(sizeof(char*) * 50);
		strcpy(*(namebrowse+i), "motorController");
		sprintf(*(namenr+i), "%d", i+1);
		strcat(*(namebrowse+i), *(namenr+i));

		/* set serial port connection settings for every file descriptor */
		set_interface_attribs(global[i].fd, BAUDRATE);						/*baudrate 115200, 8 bits, no parity, 1 stop bit */

		/* user info */
		printf("Idx: %d; fd: %d on %s for nodeIdMC = \"%s\" \n with browsename \"%s\" and motor adress \"%s\" \n\n",i, global[i].fd, global[i].ttyname, (char*)nodeIdMC[i].identifier.string.data, *(namebrowse+i), global[i].motorAddr);
	}

	/* Add Object Instances */
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Define object types and add object instances\n");
    defineObjectTypes(server);
#ifdef TYPECONSTRUCTOR
	addMotorControllerTypeConstructor(server);								/* need this? initializes lifecycle?! */
#endif
	for (i=0; i<N_MOTORCONTROLLERS; i=i+1){
		addMotorControllerObjectInstance(server, *(namebrowse+i), &nodeIdMC[i], &global[i]);
	}

	/* Define arguments for multiple threads */
	thread_args arg;
		arg.config 		= config;
		arg.server 		= server;
		arg.running 	= &running;
		arg.global 		= global;
		arg.readfds 	= &readfds;
		arg.tv			= &tv;

	/* create multiple threads */
	pthread_create(&threadServerId, NULL, threadServer, (void*)&arg);		/* Thread which runs server loop */
	pthread_create(&threadListenId, NULL, threadListen, (void*)&arg);		/* WIP: Thread which can continously read on a single port */
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Created all threads\n");

	pthread_mutex_destroy(lock);											/* WIP not yet implemented */
	pthread_exit(NULL);														/* WIP not yet implemented */

	UA_StatusCode *statusreturn = arg.status;
	return (int)*statusreturn;
}
