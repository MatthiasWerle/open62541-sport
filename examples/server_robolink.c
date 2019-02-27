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

/*********************/
/* GLOBAL PARAMETERS */
/*********************/
#define N 4 /* max. number of motor controllers */
#define DEFAULT_TTYNAME
//#define DEFAULT_MOTORADDR
//#define DEBUG_THREADTEST


/* 1: send messages in main; 2: threadTest */

/*********************************/
/* FUNCTIONS AND TYPEDEFINITIONS */
/*********************************/
static void delay(int milli_seconds) 
{ 
    clock_t start_time = clock(); /* Stroing start time */
  
    while (clock() < start_time + milli_seconds*1000) /*looping till required time is not acheived */
        ; 
} 

/* server stop handler */
UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
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

static void *threadComm(void *vargp)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "thread threadComm started to supervise port communication");

#ifdef DEBUG_THREADTEST
	globalstructMC *glob = ((thread_args*)vargp)->global;
	char* msg = "#1A\r";
	int j = 0;
	while (1){
		j++;
		delay(1000);
		printf("threadComm: %ds passed \n",j);
		printf("threadComm: sending msg = %s\n ... \n",msg);
		sport_send_msg(msg, (glob+0)->fd);
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

	/* declare global variables */
	int i;										/* index for loops */
	char **nameid = NULL;
	char **namebrowse = NULL;
	char **namenr = NULL;
	nameid = (char**)malloc(sizeof(**nameid)*N);
	namebrowse = (char**)malloc(sizeof(**nameid)*N);
	namenr = (char**)malloc(sizeof(**nameid)*N);

	UA_NodeId nodeIdMC[N];						/* Array of NodeId's for every Motor Controller Object Instance */
	globalstructMC global[N];
	globalstructMC *globalpointer[N];
#ifdef DEFAULT_TTYNAME
	global[0].ttyname = "/dev/ttyUSB0";
	global[1].ttyname = "/dev/ttyUSB1";
	global[2].ttyname = "/dev/ttyUSB2";
	global[3].ttyname = "/dev/ttyUSB3";
#else
	global[0].ttyname = "/dev/ttyUSB0";
	global[1].ttyname = "/dev/ttyUSB1";
	global[2].ttyname = "/dev/ttyUSB2";
	global[3].ttyname = "/dev/ttyUSB3";
	for (i=0; i<N; i=i+1){
		printf("Enter device file name (e.g. \"/dev/ttyUSB0\" for MotorController%d: ", i+1);
		fgets(global[i].ttyname, 50, stdin);
	}
#endif
#ifdef DEFAULT_MOTORADDR
	for (i=0; i<N; i=i+1){
		global[i].motorAddr = i+1;
	}
#else
	printf("FYI: individual motor adresses for MotorController range from 1 to 254 \n");
	printf("FYI: broadcast motor adress is \"*\" \n");
	for (i=0; i<N; i=i+1){
		printf("Enter motor adress for MotorController%d: ", i+1);
		scanf("%d", &(global[i].motorAddr));
	}
	printf("\n");
#endif

	/* set global variables */
	for (i=0; i<N; i=i+1){
		/* set attributes of array struct variable global */
		global[i].fd = set_fd(global[i].ttyname);
		printf("fyi Idx: %d; fd: %d for %s \n", i, global[i].fd, global[i].ttyname);
		
		/* set array of pointers to array elements of struct variable global */
		globalpointer[i] = (globalstructMC*)&(global[i]);
		printf("fyi globalpointer[%d]->fd = %d \n", i, globalpointer[i]->fd);
		
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
		set_interface_attribs(global[i].fd, B115200);		/*baudrate 115200, 8 bits, no parity, 1 stop bit */
		set_blocking(global[i].fd, 0);						/* set blocking for read off */
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

	/* start thread to run server */
//	UA_ServerConfig *config, UA_Server *server, UA_Boolean running


	/* DEBUG Test Variables */
	char msg[] = "#*A\r"; 	/* test command, Motor starten */


    /* simple noncanonical input */
	printf("fyi start to listen... \n");
	sport_listen(msg, global[0].fd);
	printf("fyi end of listening... \n");

	/* arguments for multiple threads */
	thread_args arg;
		arg.config = config;
		arg.server = server;
		arg.running = &running;
		arg.global = global;
	void *vargp = (void*)&arg;	/* void argument pionter */

	/* create multiple threads */
	pthread_t threadServerId;
	pthread_t threadCommId;
	pthread_t threadTestId;
	pthread_create(&threadServerId, NULL, threadServer, vargp);
	pthread_create(&threadCommId, NULL, threadComm, vargp);
	pthread_create(&threadTestId, NULL, threadTest, vargp);

	pthread_exit(NULL);
	
	UA_StatusCode *statusreturn = arg.status;
	return (int)*statusreturn;

//	UA_StatusCode test = UA_Server_readValue(server, UA_NODEID_STRING(1, "MCId1"), &fdAttr.value);
//	printf("UA_Statuscode test = %d\n", (int)test);
//	UA_Server_writeObjectProperty(server, UA_NODEID_STRING(1,"MCId1"), UA_QUALIFIEDNAME(1, "fdId"), fdId);

}
