/*********************/
/* GLOBAL PARAMETERS */
/*********************/
#define BAUDRATE B115200
#define READ_TIMEOUT_MS 50	/* can't be used in header files for they're thenfor undeclared for the compiler */
#define SIZE_MOTORADDR 3
#define SIZE_TTYNAME 50
#define N 2 				/* max. number of motor controllers */
#define N_PORTS 1			/* number of ports, either 1 or N (1 if all motor controllers are connected to the same port or N if each controller is connected to a single port) */
#define DEFAULT_TTYNAME
#define DEFAULT_MOTORADDR
//#define READ_RESPONSE
#define READ_CONT

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
#include <termios.h>
#include <unistd.h>

#include <string.h>
#include <stdint.h>
#include <time.h>				/* clock() function */
#include <unistd.h>
#include <pthread.h>			/* allows multi-threading */

// #include "func.h"
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
    clock_t start_time = clock(); /* Stroing start time */
  
    while (clock() < start_time + milli_seconds*1000) /*looping till required time is not acheived */
        ; 
} 

	typedef struct {
		UA_ServerConfig *config;
		UA_Server *server ;
		UA_Boolean *running;
		UA_StatusCode *status;
		globalstructMC *global;
		pthread_t threadReadMsgId;
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

static void *threadReadMsg(void *vargp){
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "thread threadReadMsg started to read buffered input message");
	thread_args *argp = (thread_args*)vargp;
	globalstructMC *glob = argp->global;
    if (pthread_mutex_init((glob+0)->lock, NULL) == 0){
		pthread_mutex_lock((glob+0)->lock);
		printf("threadReadMsg: mutex locked\n");
		printf("threadReadMsg: fd = %d\n", (glob+0)->fd);
		int rdlen = 0; 
		char buf[80];
		memset(buf, '\0', sizeof(buf));
		rdlen = (int)read((glob+0)->fd , buf, sizeof(buf));
		printf("received msg: rdlen = %d \n",rdlen);
		printf("received msg: buf = %s \n\n", buf);
		pthread_mutex_unlock((glob+0)->lock);
		printf("threadReadMsg: mutex unlocked \n");
	}
	else{
		printf("\n mutex init failed\n");
	}
	return NULL;
}

static void *threadListen(void *vargp1)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "thread threadListen started to supervise port communication");
#ifdef READ_CONT 										/* reads continuously on one filedescriptor */
	thread_args *argp1 = (thread_args*)vargp1;
	globalstructMC *glob1 = argp1->global;
	printf("threadListen: fd = %d\n", (glob1+0)->fd);
	int rdlen1 = 0; 
	char buf1[80];
	while(1){
		delay(1000);
		if (pthread_mutex_trylock((glob1+0)->lock) != 0){
			pthread_mutex_lock((glob1+0)->lock);
			printf("threadListen: mutex locked \n");
			memset(buf1, '\0', sizeof(buf1));
			rdlen1 = (int)read((glob1+0)->fd , buf1, sizeof(buf1));
			printf("received msg: rdlen1 = %d \n",rdlen1);
			printf("received msg: buf1 = %s \n\n", buf1);
			pthread_mutex_unlock((glob1+0)->lock);
			printf("threadListen: mutex unlocked \n");
		}
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
    signal(SIGTERM, stopHandler); /* catch ctrl-c */

    /* Create a server listening on port 4840 */
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

	/* declare global variables */
	int i;										/* index for loops */
	char **nameid = NULL;
	char **namebrowse = NULL;
	char **namenr = NULL;
	nameid = (char**)malloc(sizeof(**nameid)*N);		/* pointer to string with name of later used motor controller obj instance nodeId's */
	namebrowse = (char**)malloc(sizeof(**nameid)*N);	/* pointer to string with name of later used browsenames */
	namenr = (char**)malloc(sizeof(**nameid)*N);		/* pointer to string with name of later used number to generate nodeId's name */

	UA_NodeId nodeIdMC[N];								/* Array of NodeId's for every Motor Controller Object Instance */
	globalstructMC global[N];							/* array of structs where each element is assigned to one object instace, defined in nanotec_smci_etc.h */

	/* thread IDs */
	pthread_t threadServerId;
	pthread_t threadReadMsgId;
	pthread_t threadListenId;
	pthread_mutex_t lock[N_PORTS];

	/* declare tty names for every object instance */
	for (i=0; i<N; i=i+1){
		char ttyname_tmp[SIZE_TTYNAME];
		sprintf(ttyname_tmp, "/dev/ttyUSB%d", i);
		strcpy(global[i].ttyname, ttyname_tmp);
	}
#ifndef DEFAULT_TTYNAME
	for (i=0; i<N; i=i+1){
		printf("Enter device file name (e.g. \"/dev/ttyUSB0\" for MotorController%d: ", i+1);
		fgets(global[i].ttyname, SIZE_TTYNAME, stdin);
		global[i].ttyname[strcspn(global[i].ttyname, "\n")] = 0; 	/* delete the "\n" from fgets command at the end of the string */
	}
#endif

	/* declare lock for every ttyname */
	for (i=0; i<N; i=i+1){
#if (N_PORTS == 1)
		global[i].lock = &(lock[0]);
#else 
		global[i].lock = &(lock[i]);
#endif
	}


	/* declare motor addresses for every object instance */
	for (i=0; i<N; i=i+1){
		sprintf(global[i].motorAddr, "%d", i+1);
	}
#ifndef DEFAULT_MOTORADDR
	printf("FYI: individual motor adresses for MotorController range from 1 to 254 \n");
	printf("FYI: broadcast motor adress is \"*\" \n");
	for (i=0; i<N; i=i+1){
		printf("Enter motor adress for MotorController%d: ", i+1);
		fgets(global[i].motorAddr, SIZE_MOTORADDR, stdin);
		global[i].motorAddr[strcspn(global[i].motorAddr, "\n")] = 0; 	/* delete the "\n" from fgets command at the end of the string */
	}
	printf("\n");
#endif

	/* set global variables */
	for (i=0; i<N; i=i+1){
		/* set attributes of array struct variable global */
		global[i].fd = set_fd(global[i].ttyname);
		printf("fyi Idx: %d; fd: %d for %s \n", i, global[i].fd, global[i].ttyname);
		
		/* declare NodeId's for every object instance of a motor controller */
		nameid[i] = (char*)malloc(sizeof(char*) * 10);
		namenr[i] = (char*)malloc(sizeof(char*) * 10);
		strcpy(*(nameid+i), "MCId");
		sprintf(*(namenr+i), "%d", i+1);
		strcat(*(nameid+i), *(namenr+i));
		printf("fyi *(nameid+1) = %s \n", *(nameid+i));
		nodeIdMC[i] = UA_NODEID_STRING(1,*(nameid+i));
		printf("(char*)nodeIdMC[i].identifier.string.data = %s \n",(char*)nodeIdMC[i].identifier.string.data);
		
		/* set connection settings for serial port */
		printf("Timeout for listing on %s is set to %d ms\n", global[i].ttyname, READ_TIMEOUT_MS);
		set_interface_attribs(global[i].fd, BAUDRATE);		/*baudrate 115200, 8 bits, no parity, 1 stop bit */
	}
	
	/* arguments for multiple threads */
	thread_args arg;
		arg.config = config;
		arg.server = server;
		arg.running = &running;
		arg.global = global;
	void *vargp = (void*)&arg;	/* void argument pointer */
	
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

	/* create multiple threads */
	pthread_create(&threadServerId, NULL, threadServer, vargp);
	sport_send_msg("#*$\r",3);
	printf("create threadReadMsg\n");
	pthread_create(&threadReadMsgId, NULL, threadReadMsg, vargp);
	delay(1000);
	sport_send_msg("#*Z|\r",3);
	delay(1000);
	printf("create threadListen\n");
	pthread_create(&threadListenId, NULL, threadListen, vargp);
	pthread_mutex_destroy(lock);
	pthread_exit(NULL);

	printf("created all threads");
	UA_StatusCode *statusreturn = arg.status;
	return (int)*statusreturn;

//	UA_StatusCode test = UA_Server_readValue(server, UA_NODEID_STRING(1, "MCId1"), &fdAttr.value);
//	printf("UA_Statuscode test = %d\n", (int)test);
//	UA_Server_writeObjectProperty(server, UA_NODEID_STRING(1,"MCId1"), UA_QUALIFIEDNAME(1, "fdId"), fdId);

}
