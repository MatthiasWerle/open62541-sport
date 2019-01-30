#define DISPLAY_STRING


/*****************************************/
/* Add Motor Controller Object Instances */
/*****************************************/

/* predefined identifier for later use */
UA_NodeId motorControllerTypeId = {1, UA_NODEIDTYPE_NUMERIC, {1001}};
// UA_String myString = UA_STRING("test"); /* from example */

static void
defineObjectTypes(UA_Server *server) {
	UA_Int32 defInt = 161;
	UA_String defStr = UA_STRING("Alerta");
    /* Define the object type for "Device" */
    UA_NodeId deviceTypeId; /* get the nodeid assigned by the server */
    UA_ObjectTypeAttributes dtAttr = UA_ObjectTypeAttributes_default;
    dtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "DeviceType");
    UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "DeviceType"), dtAttr,
                                NULL, &deviceTypeId);

	/* Manufacturer Name */
    UA_VariableAttributes mnAttr = UA_VariableAttributes_default;
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ManufacturerName");
    UA_NodeId manufacturerNameId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "ManufacturerName"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), mnAttr, NULL, &manufacturerNameId);
    /* Make the manufacturer name mandatory */
    UA_Server_addReference(server, manufacturerNameId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);

	/* Model Name */
    UA_VariableAttributes modelAttr = UA_VariableAttributes_default;
    modelAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ModelName");
	UA_NodeId modelNameId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "ModelName"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), modelAttr, NULL, &modelNameId);
    /* Make the model name mandatory */
    UA_Server_addReference(server, modelNameId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);

	/* Object Type */
    /* Define the object type for "Motor Controller" */
    UA_ObjectTypeAttributes ptAttr = UA_ObjectTypeAttributes_default;
    ptAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MotorControllerType");
    UA_Server_addObjectTypeNode(server, motorControllerTypeId,
                                deviceTypeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "MotorControllerType"), ptAttr,
                                NULL, NULL);

	/* Status */
    UA_VariableAttributes statusAttr = UA_VariableAttributes_default;
    statusAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Status");
    statusAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_NodeId statusId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, motorControllerTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Status"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr, NULL, &statusId);
    /* Make the status variable mandatory */
    UA_Server_addReference(server, statusId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);

	/* Step Resolution */
    UA_VariableAttributes stepResAttr = UA_VariableAttributes_default;
    stepResAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MotorStepResolution");
//    stepResAttr.valueRank = UA_VALUERANK_SCALAR;
	stepResAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    stepResAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&stepResAttr.value, &defInt, &UA_TYPES[UA_TYPES_INT32]);
	UA_NodeId stepResId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, motorControllerTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "MotorStepResolutions"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), stepResAttr, NULL, &stepResId);
    /* Make the stepRes variable mandatory */
    UA_Server_addReference(server, stepResId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);

	/* Step Frequency */
    UA_VariableAttributes stepFreqAttr = UA_VariableAttributes_default;
    stepFreqAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MotorStepFrequency");
//    stepFreqAttr.valueRank = UA_VALUERANK_SCALAR;
	stepFreqAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    stepFreqAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&stepFreqAttr.value, &defInt, &UA_TYPES[UA_TYPES_INT32]);
	UA_NodeId stepFreqId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, motorControllerTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "MotorStepFrequency"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), stepFreqAttr, NULL, &stepFreqId);
    /* Make the stepFreq variable mandatory */
    UA_Server_addReference(server, stepFreqId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);

	/* Portname */
    UA_VariableAttributes portnameAttr = UA_VariableAttributes_default;
    portnameAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Portname");
    portnameAttr.valueRank = UA_VALUERANK_SCALAR;
	portnameAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    portnameAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&portnameAttr.value, &defStr, &UA_TYPES[UA_TYPES_STRING]);
	UA_NodeId portnameId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, motorControllerTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "portnameId"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), portnameAttr, NULL, &portnameId);
    /* Make the stepFreq variable mandatory */
    UA_Server_addReference(server, portnameId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);

}

static void
addMotorControllerObjectInstance(UA_Server *server, char *name) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", name);
//    oAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, name),
                            motorControllerTypeId, /* this refers to the object type
                                           identifier */
                            oAttr, NULL, NULL);
}

static UA_StatusCode
motorControllerTypeConstructor(UA_Server *server,
                    const UA_NodeId *sessionId, void *sessionContext,
                    const UA_NodeId *typeId, void *typeContext,
                    const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "New motor controller created");

    /* Find the NodeId of the status child variable */
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = UA_QUALIFIEDNAME(1, "Status");

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = *nodeId;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    UA_BrowsePathResult bpr =
        UA_Server_translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD ||
       bpr.targetsSize < 1)
        return bpr.statusCode;

    /* Set the status value */
    UA_Boolean status = true;
    UA_Variant value;
    UA_Variant_setScalar(&value, &status, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_BrowsePathResult_clear(&bpr);

    /* At this point we could replace the node context .. */

    return UA_STATUSCODE_GOOD;
}

static void
addMotorControllerTypeConstructor(UA_Server *server) {
    UA_NodeTypeLifecycle lifecycle;
    lifecycle.constructor = motorControllerTypeConstructor;
    lifecycle.destructor = NULL;
    UA_Server_setNodeTypeLifecycle(server, motorControllerTypeId, lifecycle);
}

/**************************/
/* serial port connection */
/**************************/

static UA_StatusCode
addVariable_fdSPort(UA_Server *server) 
{
    /* Define the attribute of the fdSPort variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 fdSPort = 1234;
    UA_Variant_setScalar(&attr.value, &fdSPort, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","file descriptor port");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","file descriptor port");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId fdSPortNodeId = UA_NODEID_STRING(1, "fd.Port");
    UA_QualifiedName fdSPortName = UA_QUALIFIEDNAME(1, "fd port");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, fdSPortNodeId, parentNodeId,
                              parentReferenceNodeId, fdSPortName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "addVariable_fdSPort was called");
	return UA_STATUSCODE_GOOD;
}


/* set file descriptor for a serial port */
static int 
set_fd(char *portname)
{
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC); /* read and write, no controlling terminal of process,  */
    if (fd < 0){
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }
    return fd;
}

// static int
static UA_INLINE UA_StatusCode 
set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        //return -1;
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

//    cfsetospeed(&tty, (speed_t)speed);
//    cfsetispeed(&tty, (speed_t)speed);

	/* set baud rates to 19200 */
	cfsetospeed(&tty, B19200);
	cfsetispeed(&tty, B19200);

    tty.c_cflag |= (unsigned int)(CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~(unsigned int)CSIZE;
    tty.c_cflag |= (unsigned int)CS8;         /* 8-bit characters */
    tty.c_cflag &= ~(unsigned int)PARENB;     /* no parity bit */
    tty.c_cflag &= ~(unsigned int)CSTOPB;     /* only need 1 stop bit */
    // tty.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(unsigned int)(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(unsigned int)(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~(unsigned int)OPOST;

	printf("tty c_cflag %d \n",tty.c_cflag);
    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
		// return -1;
    }
    return UA_STATUSCODE_GOOD;
	// return 0
}

static void
set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tggetattr", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 10;            // 1 second read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                printf ("error %d setting term attributes", errno);
}

/* callable only by server */
//static int 
//sport_send_msg(char msg[], int fd)
//{
//    int wlen = (int)write(fd, msg, strlen(msg)); /* number of bytes written */
//    if (wlen != (int)strlen(msg)) {
//        printf("Error from write: %d, %d\n", wlen, errno);
//    }
//    tcdrain(fd);    /* delay for output */
//}
//    return wlen;

/* callable by server and client */
static UA_StatusCode 
sportSendMsgMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
						 size_t inputSize, const UA_Variant *input,
//						 size_t inputSize1, const UA_Variant *inputFd,
//						 size_t inputSize2, const UA_Variant *inputMsg,
                         size_t outputSize, UA_Variant *output)
//						 const char msg[], const int fd
{
	/* debug stuff*/
    UA_Int32 *inputInt = (UA_Int32*)input[0].data;
	UA_String *inputStr = (UA_String*)input[1].data;
//	UA_String *inputStr1 = (UA_String*)input[1]->data;

	printf("stuff to check: \n");
	printf("(int)*inputInt = %d \n",(int)*inputInt);
	printf("*(UA_Int32*)input[0].data = %d \n", *(UA_Int32*)input[0].data);
	printf("(char*)input[1].data = %.*s \n", (int)inputStr->length, (char*)inputStr->data);
//	printf("(char*)inputStr1 = %s \n", (char*)inputStr1);

//	UA_Int32 wlen;
//	char *inputStr = (char*)inputMsg->data;
//	int32_t *inputInt = (int32_t*)inputFd->data;

//    UA_Int32 wlen = (UA_Int32)write((int)inputFd->data, inputMsg->data, inputMsg->arrayLength); /* number of bytes written */
//    if (wlen != (UA_Int32)inputMsg->arrayLength) {
//        printf("Error from write: %d, %d\n", wlen, errno);
//    }
//    tcdrain((int)inputFd->data);    /* delay for output */
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Send Msg was called");
    return UA_STATUSCODE_GOOD;
}

static void
addSportSendMsgMethod(UA_Server *server){
	UA_Argument inputArguments[2];
	UA_Argument_init(&inputArguments[0]);
	inputArguments[0].description = UA_LOCALIZEDTEXT("en-US", "an int, filepointer to serial port");
	inputArguments[0].name = UA_STRING("FD");
	inputArguments[0].dataType = UA_TYPES[UA_TYPES_INT32].typeId;
	inputArguments[0].valueRank = UA_VALUERANK_SCALAR; /* what is this? */
	
	UA_Argument_init(&inputArguments[1]);
	inputArguments[1].description = UA_LOCALIZEDTEXT("en-US", "A string, message to be sent");
	inputArguments[1].name = UA_STRING("InputMessage");
//	inputArguments[1].dataType = UA_TYPES[UA_TYPES_INT32].typeId;	
	inputArguments[1].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
	inputArguments[1].valueRank = UA_VALUERANK_SCALAR; /* what is this? */
	
	UA_Argument outputArgument;
	UA_Argument_init(&outputArgument);
	outputArgument.description = UA_LOCALIZEDTEXT("en-US", "A string, message that was sent");
	outputArgument.name = UA_STRING("OutputMessage");
	outputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
	outputArgument.valueRank = UA_VALUERANK_SCALAR; /* what is this? */
	
	UA_MethodAttributes sendAttr = UA_MethodAttributes_default;
	sendAttr.description = UA_LOCALIZEDTEXT("en-US","Send a string via serial port");
	sendAttr.displayName = UA_LOCALIZEDTEXT("en-US","Send a message");
	sendAttr.executable = true;
	sendAttr.userExecutable = true;
	UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "SportSendMsg"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "SportSendMsg"),
                            sendAttr, &sportSendMsgMethodCallback,
                            2, inputArguments, 1, &outputArgument, NULL, NULL);
}






static char* sport_listen(char msg[], int fd)
{
	char msg_in[strlen(msg)-1];
    do {
        int rdlen;								/* number of bytes read */
		char buf[80];
		
		printf("fyi before read\n");
//		fcntl(fd, F_SETFL, O_NDELAY); /* causes read function to return 0 if no characters available on port */
        rdlen = (int)read(fd, buf, sizeof(buf) - 1);
//		fcntl(fd, F_SETFL, 0); /* causes read function to restore normal blocking behaviour */

		strncat(msg_in, buf, strlen(msg)-1); /* appends part of read input message from buffer to msg_in */
//		printf("fyi msg_in = %s\n",&msg_in);
        if (rdlen > 0) {
#ifdef DISPLAY_STRING
            buf[rdlen] = 0;
            printf("Read %d: \"%s\"\n", rdlen, buf);
#else /* display hex */
            unsigned char   *p;
            printf("Read %d:", rdlen);
            for (p = buf; rdlen-- > 0; p++)
                printf(" 0x%x", *p);
            printf("\n");
			if (1 && strcmp(msg_in, msg+1)){
				printf("Message accepted");
				break
			}
#endif
        } else if (rdlen < 0) {
            printf("Error from read: %d: %s\n", rdlen, strerror(errno));
			return strcat("Error from read", strerror(errno));
        } else {  /* rdlen == 0 */
            printf("Timeout from read\n");
			return "Timeout from read\n";
        }
        /* repeat read to get full message */
    } while (1);
}
