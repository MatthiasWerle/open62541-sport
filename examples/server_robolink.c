/*********************/
/* GLOBAL PARAMETERS */
/*********************/
#define BAUDRATE B115200
#define READ_TIMEOUT_MS 50	/* can't be used in header files for they're thenfor undeclared for the compiler */
#define SIZE_MOTORADDR 3
#define SIZE_TTYNAME 50
#define N 2 				/* max. number of motor controllers */
#define DEFAULT_TTYNAME
#define DEFAULT_MOTORADDR
//#define DEBUG_THREADTEST
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




/* 1: send messages in main; 2: threadTest */

/*********************************/
/* FUNCTIONS AND TYPEDEFINITIONS */
/*********************************/
/* server stop handler */
UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static void delay(int milli_seconds) 
{ 
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
	} thread_args;

/***********/
/* THREADS */
/***********/
static void *threadServer(void *vargp)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "thread threadServer started to run the server");
	thread_args *arg = (thread_args*)vargp;

	/* Run the server loop */
    UA_StatusCode status = UA_Server_run(arg->server, arg->running);
	*(arg->status) = status;
    UA_Server_delete(arg->server);
    UA_ServerConfig_delete(arg->config);
	return NULL;
}

static void *threadListen(void *vargp)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "thread threadListen started to supervise port communication");
	globalstructMC *glob = ((thread_args*)vargp)->global;
	printf("fd = %d\n", (glob+0)->fd);
#ifdef DEBUG_THREADTEST
	char* msg = "#1A\r";
	int j = 0;
	while (1){
		j++;
		delay(1000);
		printf("threadListen: %ds passed \n",j);
		printf("threadListen: sending msg = %s\n ... \n",msg);
		sport_send_msg(msg, (glob+0)->fd);
	}
#endif

#ifdef READ_CONT /* reads continuously on one filedescriptor */
	int rdlen = 0; 
	char buf[80];
	while(1){
		delay(100);
		memset(buf, '\0', sizeof(buf));
		rdlen = (int)read((glob+0)->fd , buf, sizeof(buf));
		printf("received msg: rdlen = %d \n",rdlen);
		printf("received msg: buf = %s \n\n", buf);
		strcpy(buf, "");
	}
#endif
	return NULL;
}

static void *threadTest(void *vargp)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "thread threadTest started to test stuff");
	
#ifdef DEBUG_THREADTEST
	globalstructMC *glob = ((thread_args*)vargp)->global;
	printf("ttyname = %s \n", (glob+0)->ttyname);
	printf("ttyname = %s \n", (glob+1)->ttyname);
	char* msg = "#1S\r";
	int j = 0;
	delay(1000);
	while (1){
		j++;
		delay(1000);
		printf("threadTest: %fs passed \n",j+0.5);
		printf("threadTest: sending msg = %s\n ... \n",msg);
		sport_send_msg(msg, (glob+0)->fd);
	}
#endif

	printf("closing threadTest ...\n");
	return NULL;
}
/*************/
/* MAIN LOOP */
/*************/
int main(int argc, char** argv)
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler); /* catch ctrl-c */

    /* Create a server listening on port 4840 */
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

	delay(1);
	
	printf("READ_TIMEOUT_MS = %d\n", READ_TIMEOUT_MS);

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

	printf("jo_test\n");
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

	/* WIP changing object properties, yet not working */
	UA_Variant fdId;
	UA_String fdvalue = UA_STRING("1337");
	UA_Variant_setScalar(&fdId, &fdvalue, &UA_TYPES[UA_TYPES_STRING]);

	/* arguments for multiple threads */
	thread_args arg;
		arg.config = config;
		arg.server = server;
		arg.running = &running;
		arg.global = global;
	void *vargp = (void*)&arg;	/* void argument pionter */

	/* create multiple threads */
	pthread_t threadServerId;
	pthread_t threadListenId;
	pthread_t threadTestId;
	pthread_create(&threadServerId, NULL, threadServer, vargp);
	pthread_create(&threadListenId, NULL, threadListen, vargp);
	pthread_create(&threadTestId, NULL, threadTest, vargp);
	pthread_exit(NULL);
	
	UA_StatusCode *statusreturn = arg.status;
	return (int)*statusreturn;

//	UA_StatusCode test = UA_Server_readValue(server, UA_NODEID_STRING(1, "MCId1"), &fdAttr.value);
//	printf("UA_Statuscode test = %d\n", (int)test);
//	UA_Server_writeObjectProperty(server, UA_NODEID_STRING(1,"MCId1"), UA_QUALIFIEDNAME(1, "fdId"), fdId);

}
