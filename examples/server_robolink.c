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

//#include "func.h"				/* helpful library eg. set/get nodeId or attributes */

	typedef struct {
		char* ttyname;
		int fd;
		int motorAddr;
	} globalstructMC;

#include "sport.h" 				/* serial port communication */
#include "nanotec_smci_etc.h"	/* command list for step motor controllers from nanotec */

/*********************/
/* GLOBAL PARAMETERS */
/*********************/
#define N 4 /* max. number of motor controllers */

static void delay(int milli_seconds) 
{ 
    // Stroing start time 
    clock_t start_time = clock(); 
  
    // looping till required time is not acheived 
    while (clock() < start_time + milli_seconds*1000) 
        ; 
} 

/******************/
/* PROGRAMM START */
/******************/
/* server stop handler */
UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

/*************/
/* MAIN LOOP */
/************/
int main(int argc, char** argv)
{
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
	nameid = (char**)malloc(sizeof(**nameid)*N);
	namebrowse = (char**)malloc(sizeof(**nameid)*N);
	namenr = (char**)malloc(sizeof(**nameid)*N);

	UA_NodeId nodeIdMC[N];						/* Array of NodeId's for every Motor Controller Object Instance */
	globalstructMC global[N];
	globalstructMC *globalpointer[N];
		global[0].ttyname = "/dev/ttyUSB0";
		global[1].ttyname = "/dev/ttyUSB1";
		global[2].ttyname = "/dev/ttyUSB2";
		global[3].ttyname = "/dev/ttyUSB3";
		global[0].motorAddr = 1;
		global[1].motorAddr = 1;
		global[2].motorAddr = 3;
		global[3].motorAddr = 4;
		
	/* set global variables */
	for (i=0; i<N; i=i+1){
	/* set attributes of array struct variable global */
		global[i].fd = set_fd(global[i].ttyname);
		global[i].fd = 3; 																		/* DEBUG muss später weg */
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





	/* Adding Methods */
//	printf("fyi adding methods ... \n");
//	addSportSendMsgMethod(server);


	/* DEBUG Test Variables */
	char msg[] = "#*A\r"; 	/* test command, Motor starten */
	printf("sending msg = %s\n ... \n",msg);
	sport_send_msg(msg, global[0].fd);
	delay(3000);
	strcpy(msg, "#*s-40000\r");
	printf("sending msg = %s\n ... \n",msg);
    sport_send_msg(msg, global[0].fd);
	delay(3000);
	strcpy(msg, "#*A\r");
	printf("sending msg = %s\n ... \n",msg);
    sport_send_msg(msg, global[0].fd);
	delay(3000);
	strcpy(msg, "#*s40001\r");
	printf("sending msg = %s\n ... \n",msg);
    sport_send_msg(msg, global[0].fd);
	delay(3000);
	strcpy(msg, "#*A\r");
	printf("sending msg = %s\n ... \n",msg);
    sport_send_msg(msg, global[0].fd);
	
    /* simple noncanonical input */
	printf("fyi start to listen... \n");
	sport_listen(msg, global[0].fd);
	printf("fyi end of listening... \n");

	/* WIP changing object properties, yet not working */
	UA_Variant fdId;
	UA_String fdvalue = UA_STRING("1337");
	UA_Variant_setScalar(&fdId, &fdvalue, &UA_TYPES[UA_TYPES_STRING]);
	
//	UA_StatusCode test = UA_Server_readValue(server, UA_NODEID_STRING(1, "MCId1"), &fdAttr.value);
//	printf("UA_Statuscode test = %d\n", (int)test);
//	UA_Server_writeObjectProperty(server, UA_NODEID_STRING(1,"MCId1"), UA_QUALIFIEDNAME(1, "fdId"), fdId);

    /* 20) Run the server loop */
    UA_StatusCode status = UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return (int)status; /* probably implicitly conversion to int */
}
