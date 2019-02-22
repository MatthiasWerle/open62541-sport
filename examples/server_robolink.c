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

#include "sport.h" 				/* serial port communication */
#include "func.h"				/* helpful library eg. set/get nodeId or attributes */
#include "nanotec_smci_etc.h"	/* command list for step motor controllers from nanotec */

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
	UA_String ttyname1 = UA_STRING("/dev/ttyUSB0");	
	UA_String ttyname2 = UA_STRING("/dev/ttyUSB1");	
	UA_String ttyname3 = UA_STRING("/dev/ttyUSB2");	
	UA_String ttyname4 = UA_STRING("/dev/ttyUSB3");	

	int fd1 = set_fd((char*)ttyname1.data);					/* file descriptor for serial port */
	int fd2 = set_fd((char*)ttyname2.data);					/* file descriptor for serial port */
	int fd3 = set_fd((char*)ttyname3.data);					/* file descriptor for serial port */
	int fd4 = set_fd((char*)ttyname4.data);					/* file descriptor for serial port */

	UA_Variant valVar;

    /* set connection settings for serial port */
    printf("fyi start setting interface attributes... \n");
	set_interface_attribs(fd1, B115200);		/*baudrate 115200, 8 bits, no parity, 1 stop bit */
	set_interface_attribs(fd2, B115200);		/*baudrate 115200, 8 bits, no parity, 1 stop bit */
	set_interface_attribs(fd3, B115200);		/*baudrate 115200, 8 bits, no parity, 1 stop bit */
	set_interface_attribs(fd4, B115200);		/*baudrate 115200, 8 bits, no parity, 1 stop bit */
	set_blocking(fd1, 0);						/* set blocking for read off */
	set_blocking(fd2, 0);						/* set blocking for read off */
	set_blocking(fd3, 0);						/* set blocking for read off */
	set_blocking(fd4, 0);						/* set blocking for read off */

	/* Add nodeIds for object instances created later on */
	UA_NodeId nodeIdMC1 = UA_NODEID_STRING(1,"MCId1");
	UA_NodeId nodeIdMC2 = UA_NODEID_STRING(1,"MCId2");
	UA_NodeId nodeIdMC3 = UA_NODEID_STRING(1,"MCId3");
	UA_NodeId nodeIdMC4 = UA_NODEID_STRING(1,"MCId4");

	/* Add Object Instances */
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "define object types and add object instances");
    defineObjectTypes(server);

    addMotorControllerObjectInstance(server, "motorController1", &nodeIdMC1, &fd1);
		UA_Variant_setScalar(&valVar, &ttyname1, &UA_TYPES[UA_TYPES_STRING]);
		setChildAttrVal(server, &nodeIdMC1, "TTYname", valVar, UA_TYPES[UA_TYPES_STRING]);



	addMotorControllerObjectInstance(server, "motorController2", &nodeIdMC2, &fd2);
		UA_Variant_setScalar(&valVar, &ttyname2, &UA_TYPES[UA_TYPES_STRING]);
		setChildAttrVal(server, &nodeIdMC2, "TTYname", valVar, UA_TYPES[UA_TYPES_STRING]);

	addMotorControllerTypeConstructor(server);	/* need this? initializes lifecycle?! */
	addMotorControllerObjectInstance(server, "motorController3", &nodeIdMC3, &fd3);
	addMotorControllerObjectInstance(server, "motorController4", &nodeIdMC4, &fd4);


	/* DEBUG Test Variables */
	char* msg = "#*A\r"; 	/* test command, Motor starten */

	/* DEBUG EXTRA INFO */
	printf(" ttyname1: %s \n fd1: %d \n", (char*)ttyname1.data, fd1);
	printf(" ttyname1: %s \n fd2: %d \n", (char*)ttyname2.data, fd2);
	printf(" ttyname1: %s \n fd3: %d \n", (char*)ttyname3.data, fd3);
	printf(" ttyname1: %s \n fd4: %d \n", (char*)ttyname4.data, fd4);
	printf("msg = %s\n",msg);

	/* DEBUG TESTING SITE: GET NODEID*/
	UA_NodeId nodeIdMC3tty;
	nodeIdMC3tty.identifierType = UA_NODEIDTYPE_NUMERIC;
	getChildNodeId(server, &nodeIdMC3, "TTYname", &nodeIdMC3tty);
	printf("child nodeId tty MC3: %u \n", nodeIdMC3tty.identifier.numeric);

	/* Adding Methods */
//	printf("fyi adding methods ... \n");
//	addSportSendMsgMethod(server);

    /* simple output on serial port */
	printf("fyi start sending a message... \n");
    sport_send_msg(msg, fd1);

    /* simple noncanonical input */
	printf("fyi start to listen... \n");
	sport_listen(msg, fd1);
	printf("fyi end of listening... \n");

	/* TESTING SITE DATATYPES */
	UA_String teststring;
	teststring = UA_STRING("#Important Message");
	printf("teststring.data = %s\n", teststring.data);
	printf("teststring.length = %zu\n", teststring.length);
	printf("strlen((char*)teststring.data) = %zu\n", strlen((char*) teststring.data));
	teststring.data += 1; printf("teststring.data +=1;\n");
	printf("teststring.data = %s\n", teststring.data);
	printf("teststring.length = %zu\n", teststring.length);
	printf("strlen((char*)teststring.data) = %zu\n", strlen((char*) teststring.data));
	printf("sizeof(teststring.length) = %zu \n", sizeof(teststring.length));
	int testint = 5;
	printf("testint = 5\n");
	printf("sizeof(testint) = %zu \n", sizeof(testint));

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
