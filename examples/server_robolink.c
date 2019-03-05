/*********************/
/* GLOBAL PARAMETERS */
/*********************/
#define BAUDRATE B115200
#define READ_TIMEOUT_S 10		/* reading timeout in s */
#define READ_TIMEOUT_US 0		/* reading timeout in us */
#define SIZE_MOTORADDR 3		/* 3 bytes, for the string reaches from "1" to "255" */
#define SIZE_TTYNAME 50
#define N 3 					/* max. number of motor controllers */
#define N_PORTS 1				/* number of ports, either 1 or N (1 if all motor controllers are connected to the same port or N if each controller is connected to a single port) */
#define DEFAULT_TTYNAME
#define DEFAULT_MOTORADDR		/* ATTENTION: SERVER WILL HANG ITSELF UP IF THE WRONG MOTOR ADRESS IS CONFIGURED BECAUSE READ BLOCKS*/
#define READ_RESPONSE
//#define READ_CONT

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

#include <string.h>
#include <stdint.h>
#include <time.h>				/* clock() function */
#include <unistd.h>
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
		pthread_t threadReadMsgId;
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

static void *threadReadMsg(void *vargp){											/* at the moment totally useless thread */
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "thread threadReadMsg started to read buffered input message");
#ifdef READ_RESPONSE
	thread_args *argp = (thread_args*)vargp;
	globalstructMC *glob = argp->global;
    if (pthread_mutex_init((glob+0)->lock, NULL) == 0){
		pthread_mutex_lock((glob+0)->lock);
		printf("threadReadMsg: mutex locked\n");
		printf("threadReadMsg: fd = %d\n", (glob+0)->fd);
		char msg[80];
		int rdlen = sport_read_msg(msg, (glob+0)->fd);								/* read message */
		printf("received msg: %d bytes in buf = %s \n\n", rdlen, msg);
		pthread_mutex_unlock((glob+0)->lock);
		printf("threadReadMsg: mutex unlocked \n");
	}
	else{
		printf("\n mutex init failed\n");
	}
#endif
	return NULL;
}

static void *threadListen(void *vargp)
{
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
			if (pthread_mutex_trylock((glob+0)->lock) != 0){
				pthread_mutex_lock((glob+0)->lock);
				printf("threadListen: mutex locked \n");
				int rdlen = sport_read_msg(msg, (glob+0)->fd);	/* read message */
				printf("received msg: %d bytes in buf = %s \n\n", rdlen, msg);
				pthread_mutex_unlock((glob+0)->lock);
				printf("threadListen: mutex unlocked \n");
			}
		}
		else if(ready == -1)
			printf("Error from select(): %s \n", strerror(errno));
		else /* ready == 0 */
			printf("Timeout from select() after %d s\n", READ_TIMEOUT_S);
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
    signal(SIGTERM, stopHandler); 						/* catch ctrl-c */

    /* Create a server listening on port 4840 */
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

	/* DECLARE GLOBAL VARIABLES */
	int i;												/* index for loops */
	char **nameid = NULL;
	char **namebrowse = NULL;
	char **namenr = NULL;

	UA_NodeId nodeIdMC[N];								/* Array of NodeId's for every Motor Controller Object Instance */
	globalstructMC global[N];							/* array of structs where each element is assigned to one object instace, defined in nanotec_smci_etc.h */

	/* thread IDs */
	pthread_t threadServerId;
	pthread_t threadReadMsgId;
	pthread_t threadListenId;
	pthread_mutex_t lock[N_PORTS];

	/* variables for select() to monitor multiple filedescriptors */
	fd_set readfds;
	fd_set readfds_single[N];							/* array with all fds from readfds as single fd sets in an array */
	struct timeval tv;									/* set timeout for monitoring filedescriptors */
		tv.tv_sec = READ_TIMEOUT_S;						/* timeout in s */
		tv.tv_usec = READ_TIMEOUT_US;					/* timeout in microseconds */
		printf("tv_sec = %ld, tv_usec = %ld \n", tv.tv_sec, tv.tv_usec);
	
	/* declare tty names for every object instance */
	for (i=0; i<N; i=i+1){
		#ifdef DEFAULT_TTYNAME
			#if (N_PORTS == 1)							/* each motor controller connected to same port */
				strcpy(global[i].ttyname, "/dev/ttyUSB0");
			#endif
			#if (N_PORTS == N) 							/* each motor controller connected to own port */
				char ttyname_tmp[SIZE_TTYNAME];
				sprintf(ttyname_tmp, "/dev/ttyUSB%d", i);
				strcpy(global[i].ttyname, ttyname_tmp);
			#endif
			#if (N_PORTS != N && N_PORTS != 1)
				printf("N_PORTS = %d\n", N_PORTS);
				printf("irregular number of motorcontrollers (N) or ports (N_PORTS)\n");
			#endif
		#endif
		#ifndef DEFAULT_TTYNAME
				printf("Enter device file name (e.g. \"/dev/ttyUSB0\" for MotorController%d: ", i+1);
				fgets(global[i].ttyname, SIZE_TTYNAME, stdin);
				global[i].ttyname[strcspn(global[i].ttyname, "\n")] = 0; 	/* delete the "\n" from fgets command at the end of the string */
		#endif
	}

	/* WIP declare lock for every ttyname */
	for (i=0; i<N; i=i+1){
		#if (N_PORTS == 1)
			global[i].lock = &(lock[0]);
		#endif
		#if (N_PORTS == N) 
			global[i].lock = &(lock[i]);
		#endif
	}

	/* declare motor addresses for every object instance */
	#ifndef DEFAULT_MOTORADDR
		printf("FYI: individual motor adresses for MotorController range from 1 to 254 \n");
		printf("FYI: broadcast motor adress is \"*\" \n");
	#endif
	for (i=0; i<N; i=i+1){
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
	nameid = (char**)malloc(sizeof(**nameid)*N);			/* pointer to string with name of later used motor controller obj instance nodeId's */
	namebrowse = (char**)malloc(sizeof(**nameid)*N);		/* pointer to string with name of later used browsenames */
	namenr = (char**)malloc(sizeof(**nameid)*N);			/* pointer to string with name of later used number to generate nodeId's name */

	FD_ZERO(&readfds);										/* clear file descriptor set */
	/* set filedescriptors, nodeIds for obj. instances, serial port connection configurations, */
	for (i=0; i<N; i=i+1){
		/* set filedescriptors in fd-set and in array struct variable global */
		global[i].fd = set_fd(global[i].ttyname);			/* actually redundant because it'll be also contained in global[i].readfds WIP */
		printf("global[i].fd = %d\n", global[i].fd);
		FD_ZERO(&readfds_single[i]);							/* ATENTION! clear file descriptor set */
		if(global[i].fd != -1){
			FD_SET(global[i].fd, &readfds_single[i]);			/* ATTENTION! fd set of single fd for global struct assigend to a single motor controller object instance */
			FD_SET(global[i].fd, &readfds);						/* fd set of all fds, to be used in continously on all fds listening thread WIP!*/
		}
		global[i].readfds = &readfds_single[i];
		global[i].tv = &tv;

		/* declare NodeId's for every object instance of a motor controller */
		nameid[i] = (char*)malloc(sizeof(char*) * 10);
		namenr[i] = (char*)malloc(sizeof(char*) * 10);
		strcpy(*(nameid+i), "MCId");
		sprintf(*(namenr+i), "%d", i+1);
		strcat(*(nameid+i), *(namenr+i));
		nodeIdMC[i] = UA_NODEID_STRING(1,*(nameid+i));

		/* user info */
		printf("Idx: %d; fd: %d on %s for nodeIdMC = \"%s\" with motor adress \"%s\" \n",i, global[i].fd, global[i].ttyname, (char*)nodeIdMC[i].identifier.string.data, global[i].motorAddr);
		/* set connection settings for serial port */
		set_interface_attribs(global[i].fd, BAUDRATE);		/*baudrate 115200, 8 bits, no parity, 1 stop bit */
	}

	/* Add Object Instances */
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "define object types and add object instances");
    defineObjectTypes(server);
	addMotorControllerTypeConstructor(server);	/* need this? initializes lifecycle?! */
	for (i=0; i<N; i=i+1){
		namebrowse[i] = (char*)malloc(sizeof(char*) * 50);
		strcpy(*(namebrowse+i), "motorController");
		sprintf(*(namenr+i), "%d", i+1);
		strcat(*(namebrowse+i), *(namenr+i));		
		printf("fyi *(namebrowse+i) = %s \n", *(namebrowse+i));
		printf("(char*)nodeIdMC[i].identifier.string.data = %s \n",(char*)nodeIdMC[i].identifier.string.data);
		addMotorControllerObjectInstance(server, *(namebrowse+i), &nodeIdMC[i], &global[i]);
	}

	/* arguments for multiple threads */
	thread_args arg;
		arg.config 		= config;
		arg.server 		= server;
		arg.running 	= &running;
		arg.global 		= global;
		arg.readfds 	= &readfds;
		arg.tv			= &tv;
	void *vargp = (void*)&arg;	/* void argument pointer */

	/* create multiple threads */
	pthread_create(&threadServerId, NULL, threadServer, vargp);
	sport_send_msg("#*$\r",3);
	printf("create threadReadMsg\n");
	pthread_create(&threadReadMsgId, NULL, threadReadMsg, vargp);
	sport_send_msg("#*Z|\r",3);
	printf("create threadListen\n");
	pthread_create(&threadListenId, NULL, threadListen, vargp);
	printf("created all threads\n");

	pthread_mutex_destroy(lock);
	pthread_exit(NULL);

	UA_StatusCode *statusreturn = arg.status;
	return (int)*statusreturn;
}
