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

#define DISPLAY_STRING /* for debugging sport.h */
#include "sport.h" /* serial port communication */

/******************/
/* PROGRAMM START */
/******************/
char *ttyname1 = "/dev/ttyUSB0";						/* TODO: implement ttyname.c to find ttyname with deviceID */
char msg[] = "Output Message for test purposes"; 	/* output message */

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main(int argc, char** argv)
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler); /* catch ctrl-c */

    /* Create a server listening on port 4840 */
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    /* 00) Add a variable node */
	UA_NodeId nodeIdMC1 = UA_NODEID_STRING(1,"MCId1");
	UA_NodeId nodeIdMC2 = UA_NODEID_STRING(1,"MCId2");
	UA_NodeId nodeIdMC3 = UA_NODEID_STRING(1,"MCId3");
	UA_NodeId nodeIdMC4 = UA_NODEID_STRING(1,"MCId4");

    printf("fyi adding object instances ... \n");
    defineObjectTypes(server);

    addMotorControllerObjectInstance(server, "motorController1", &nodeIdMC1);
		setttyname(server, &nodeIdMC1, ttyname1);

	addMotorControllerObjectInstance(server, "motorController2", &nodeIdMC2);
		UA_String valStr = UA_STRING("/dev/ttyUSB1");
		UA_Variant valVar;
		UA_Variant_setScalar(&valVar, &valStr, &UA_TYPES[UA_TYPES_STRING]);
		setChildAttrVal(server, &nodeIdMC2, "TTYname", valVar, UA_TYPES[UA_TYPES_STRING]);

	addMotorControllerTypeConstructor(server);	/* need this? sets status to on and propably initializes lifecycle*/
	addMotorControllerObjectInstance(server, "motorController3", &nodeIdMC3);
	addMotorControllerObjectInstance(server, "motorController4", &nodeIdMC4);

	printf("fyi adding methods ... \n");
	addSportSendMsgMethod(server);
	addHellWorldMethod(server);
//	addFdMethod(server);

    /* 10) set connection settings for serial port */
    int fd = set_fd(ttyname1);					/* file descriptor for serial port */
	printf(" ttyname1: %s \n fd: %d \n", ttyname1, fd);
    printf("fyi start setting interface attributes... \n");
	set_interface_attribs(fd, B115200);         /*baudrate 115200, 8 bits, no parity, 1 stop bit */
	set_blocking(fd, 0);						/* set blocking for read off */

    /* 11) simple output on serial port */
	printf("fyi start sending a message... \n");
//    sport_send_msg(msg, fd);

    /* 12) simple noncanonical input */
	printf("fyi start to listen... \n");
	sport_listen(msg, fd);
	printf("fyi end of listening... \n");

	/* TESTING SITE START */
	UA_String teststring;
	teststring = UA_STRING("#Important Message");
	printf("teststring.data = %s\n", teststring.data);
	printf("teststring.length = %zu\n", teststring.length);
	printf("teststring.data+1= %s\n", teststring.data+1);
	printf("teststring.length-1 = %zu\n", teststring.length-1);
	printf("sizeof(teststring.length) = %zu \n", sizeof(teststring.length));

	int testint = 5;
	printf("testint = 5\n");
	printf("sizeof(testint) = %zu \n", sizeof(testint));
	/* TESTING SITE END */
	

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
