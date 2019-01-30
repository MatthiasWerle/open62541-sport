#include <signal.h>


//#include "open62541.h"

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

#include "sport.h" /* serial port communication */

//#define DISPLAY_STRING

static void
addVariable(UA_Server *server)
{
    /* Define the node attributes */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US", "the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "the answer");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    /* Define where the node shall be added with which browsename */
    UA_NodeId newNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableType = UA_NODEID_NULL; /* take the default variable type */
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "the answer");
    UA_Server_addVariableNode(server, newNodeId, parentNodeId, parentReferenceNodeId,
                              browseName, variableType, attr, NULL, NULL);
}

/******************/
/* PROGRAMM START */
/******************/
char *portname = "/dev/ttyUSB0";
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
    printf("fyi adding variables ... \n");
	addVariable(server);
	addVariable_fdSPort(server);
    printf("fyi adding object instances ... \n");
    defineObjectTypes(server);
    addMotorControllerObjectInstance(server, "motorController1");
    addMotorControllerObjectInstance(server, "motorController2");
    addMotorControllerTypeConstructor(server);								/* What is this for? */
    addMotorControllerObjectInstance(server, "motorController3");
    addMotorControllerObjectInstance(server, "motorController4");
    printf("fyi adding methods ... \n");
	addSportSendMsgMethod(server);


    /* 10) set connection settings for serial port */
    int fd = set_fd(portname);					/* file descriptor for serial port */
    printf("fyi start setting interface attributes... \n");
	set_interface_attribs(fd, B115200);         /*baudrate 115200, 8 bits, no parity, 1 stop bit */

    /* 11) simple output on serial port */
	printf("fyi start sending a message... \n");
//    sport_send_msg(msg, fd);

    /* 12) simple noncanonical input */
	printf("fyi start to listen... \n");
	set_blocking(fd, 0);
	sport_listen(msg, fd);
	printf("fyi end of listening... \n");

    /* 20) Run the server loop */
    UA_StatusCode status = UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return (int)status; /* probably implicitly conversion to int */
}
