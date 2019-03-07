/*****************************************/
/* Add Motor Controller Object Instances */
/*****************************************/
/* typedef for global variable */
	typedef struct {
		char ttyname[SIZE_TTYNAME];														/* tty portname e.g. /dev/ttyUSB0 */
		int fd;																			/* file descriptor of tty port */
		char motorAddr[SIZE_MOTORADDR];													/* preconfigured motor adress of nanotec stepper motor driver with integrated controller */
		pthread_mutex_t *lock;
		fd_set *readfds;
		struct timeval *tv;
	} globalstructMC;

/* predefined identifier for later use */
UA_NodeId motorControllerTypeId = {1, UA_NODEIDTYPE_NUMERIC, {1001}};

static void
defineObjectTypes(UA_Server *server) {
	/* default values */
	UA_String defmn = UA_STRING("Nanotec"); 											/* default manufacturer name */
	UA_String defmodel = UA_STRING("SMCI47-S-2"); 										/* default model */

    /* Define the object type for "Device" */
    UA_ObjectTypeAttributes dtAttr = UA_ObjectTypeAttributes_default;
    dtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "DeviceType");
    UA_NodeId deviceTypeId; /* get the nodeid assigned by the server */
    UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "DeviceType"), dtAttr,
                                NULL, &deviceTypeId);
    /* Make the object type mandatory */
    UA_Server_addReference(server, deviceTypeId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
						   
	/* Manufacturer Name */
    UA_VariableAttributes mnAttr = UA_VariableAttributes_default;
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ManufacturerName");
    UA_Variant_setScalar(&mnAttr.value, &defmn, &UA_TYPES[UA_TYPES_STRING]);
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
    UA_Variant_setScalar(&mnAttr.value, &defmodel, &UA_TYPES[UA_TYPES_STRING]);
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
    UA_ObjectTypeAttributes ctAttr = UA_ObjectTypeAttributes_default;
    ctAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MotorControllerType");
    UA_Server_addObjectTypeNode(server, motorControllerTypeId,
                                deviceTypeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "MotorControllerType"), ctAttr,
                                NULL, NULL);
}

#ifdef TYPECONSTRUCTOR
/* type constructor for controller type */
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
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
		return bpr.statusCode; }

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
#endif

/*******************/
/* OTHER FUNCTIONS */
/*******************/
static UA_INLINE UA_StatusCode
MCcommand(const char* motorAddr, const char* cmd, const int* valPtr, char* msg)
{
	memset(msg,0,strlen(msg));											/* empty message */
	char valStr[20];
	memset(valStr,0,strlen(valStr)); 									/* empty message */
	if (valPtr != NULL){
		sprintf(valStr, "%d", *valPtr);
	}
	strcpy(msg,"#"); 													/* write start sign # to message */
	strcat(msg, motorAddr);												/* append motor adress to message */
	strcat(msg, cmd);													/* append command to message */
	strcat(msg, valStr);												/* append value to message */
	strcat(msg, "\r");													/* append end sign to message */
	return UA_STATUSCODE_GOOD;
}

/****************/
/* DATA SOURCES */
/****************/
static UA_StatusCode
readCurrentAngle(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
                UA_DataValue *dataValue) {
	globalstructMC *global = (globalstructMC*)nodeContext;
	char msg[30];
	MCcommand(global->motorAddr, "C", NULL, msg);						/* concatenate message */
	tcflush(global->fd, TCIFLUSH); 										/* flush input buffer */
	sport_send_msg(msg, global->fd);									/* send message */
	sport_read_msg(msg, global->fd);									/* read response */
	UA_Int32 angle = atoi(msg+strlen(global->motorAddr)+1);				/* convert char* to int */
	
	UA_Variant_setScalarCopy(&dataValue->value, &angle, &UA_TYPES[UA_TYPES_INT32]);
	dataValue->hasValue = true;											/* probably obsolet? */
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeCurrentAngle(UA_Server *server,
                 const UA_NodeId *sessionId, void *sessionContext,
                 const UA_NodeId *nodeId, void *nodeContext,
                 const UA_NumericRange *range, const UA_DataValue *dataValue) {
	globalstructMC *global = (globalstructMC*)nodeContext;
	char msg[30];
	char positionstr[10];
	char cmd[11] = "s";
	int* position = (int*)dataValue->value.data;
	sprintf(positionstr, "%d", *position);
	strcat(cmd, positionstr);
	MCcommand(global->motorAddr, "p2", NULL, msg);
	sport_send_msg(msg, global->fd);									/* config: positioning mode absolute */
	MCcommand(global->motorAddr, cmd, NULL, msg);
	sport_send_msg(msg, global->fd);									/* config: traverse path to input position*/
	MCcommand(global->motorAddr, "d1", NULL, msg);
	sport_send_msg(msg, global->fd);									/* config: turn direction forward */
	MCcommand(global->motorAddr, "W0", NULL, msg);
	sport_send_msg(msg, global->fd);									/* config: no repeats of configured set */
	MCcommand(global->motorAddr, "N0", NULL, msg);
	sport_send_msg(msg, global->fd);									/* config: no following set to be performed */
	MCcommand(global->motorAddr, "A", NULL, msg);
	sport_send_msg(msg, global->fd);									/* Start Motor */
	

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Changing the angle like this is not implemented");
    return UA_STATUSCODE_BADINTERNALERROR;
}

static void
addCurrentAngleDataSourceVariable(UA_Server *server, const UA_NodeId *nodeId, globalstructMC *global) {
	char name[50];
	strcpy(name, (char*)((*nodeId).identifier.string.data));
	strcat(name, "-current-angle-datasource");

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Current angle - data source");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId currentNodeId = UA_NODEID_STRING(1, name);
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, name);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_DataSource angleDataSource;
	angleDataSource.read = readCurrentAngle;
	angleDataSource.write = writeCurrentAngle;
    UA_Server_addDataSourceVariableNode(server, currentNodeId, *nodeId,
                                        parentReferenceNodeId, currentName,
                                        variableTypeNodeId, attr,
                                        angleDataSource, (void*)global, NULL);
}

static UA_StatusCode
readCurrentStatus(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
                UA_DataValue *dataValue) {
	globalstructMC *global = (globalstructMC*)nodeContext;
	char msg[30];
	MCcommand(global->motorAddr, "$", NULL, msg);						/* concatenate message */
	tcflush(global->fd, TCIFLUSH); 										/* flush input buffer */
	sport_send_msg(msg, global->fd);									/* send message */
	sport_read_msg(msg, global->fd);									/* read response */

	UA_Int32 status = atoi(strpbrk(msg, "$")+1);				/* convert char* to int */
	
	UA_Variant_setScalarCopy(&dataValue->value, &status, &UA_TYPES[UA_TYPES_INT32]);
	dataValue->hasValue = true;											/* probably obsolet? */
    return UA_STATUSCODE_GOOD;
}

static void
addCurrentStatusDataSourceVariable(UA_Server *server, const UA_NodeId *nodeId, globalstructMC *global) {
	char name[50];
	strcpy(name, (char*)((*nodeId).identifier.string.data));
	strcat(name, "-current-status-datasource");

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Current status - data source");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;

    UA_NodeId currentNodeId = UA_NODEID_STRING(1, name);
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, name);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_DataSource statusDataSource;
	statusDataSource.read = readCurrentStatus;
    UA_Server_addDataSourceVariableNode(server, currentNodeId, *nodeId,
                                        parentReferenceNodeId, currentName,
                                        variableTypeNodeId, attr,
                                        statusDataSource, (void*)global, NULL);
}

/*************************************/
/* CALLBACK METHODS FOR OBJECT NODES */
/*************************************/
static UA_StatusCode 
startMotorMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
						 size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output)
{
	globalstructMC *global = (globalstructMC*)objectContext;

	if (global->fd != -1){
		char msg[8];
		memset(msg,0,sizeof(msg)); 											/* empty message */
		MCcommand(global->motorAddr, "A", NULL, msg);						/* concatenate message */
		tcflush(global->fd, TCIFLUSH); 										/* flush input buffer */
		sport_send_msg(msg, global->fd);									/* send message */
#ifdef READ_RESPONSE
		sport_read_msg(msg, global->fd);
	}
#endif
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Start Motor was called");
    return UA_STATUSCODE_GOOD;
}

static void
addStartMotorMethod(UA_Server *server, const UA_NodeId *objectNodeId, globalstructMC *global){
	UA_MethodAttributes sendAttr = UA_MethodAttributes_default;
	sendAttr.description = UA_LOCALIZEDTEXT("en-US","Start Motor with current set");
	sendAttr.displayName = UA_LOCALIZEDTEXT("en-US","Start Motor");
	sendAttr.executable = true;
	sendAttr.userExecutable = true;
	UA_Server_addMethodNode(server, UA_NODEID_NULL,	 *objectNodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "StartMotorMethod"),
                            sendAttr, &startMotorMethodCallback,
                            0, NULL, 0, NULL, (void*)global, NULL);
}

static UA_StatusCode 
stopMotorMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
						 size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output)
{
	globalstructMC *global = (globalstructMC*)objectContext;
	char msg[8];
	memset(msg,0,sizeof(msg));											/* empty message */
	MCcommand(global->motorAddr, "S", NULL, msg);
	tcflush(global->fd, TCIFLUSH); 										/* flush input buffer */
	sport_send_msg(msg, global->fd);									/* send message */
#ifdef READ_RESPONSE
	sport_read_msg(msg, global->fd);
#endif
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Stop Motor was called");
    return UA_STATUSCODE_GOOD;
}

static void
addStopMotorMethod(UA_Server *server, const UA_NodeId *objectNodeId, globalstructMC *global){
	UA_MethodAttributes sendAttr = UA_MethodAttributes_default;
	sendAttr.description = UA_LOCALIZEDTEXT("en-US","Stop Motor with current set");
	sendAttr.displayName = UA_LOCALIZEDTEXT("en-US","Stop Motor");
	sendAttr.executable = true;
	sendAttr.userExecutable = true;
	UA_Server_addMethodNode(server, UA_NODEID_NULL,	 *objectNodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "StopMotorMethod"),
                            sendAttr, &stopMotorMethodCallback,
                            0, NULL, 0, NULL, (void*)global, NULL);
}

static UA_StatusCode 
readSetMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
						 size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output)
{
	printf("readSetMethodCallback ... \n");
	/* initial declerations */
	globalstructMC *global = (globalstructMC*)objectContext;
//	int lockcheck = pthread_mutex_trylock(global->lock);
//	if (lockcheck != 0){
		/* send msg and receive response */
		char msg[80];													/* empty message */
		memset(msg,0,sizeof(msg));
		char* cmd = "Z|";
		MCcommand(global->motorAddr, cmd, NULL, msg);					/* concatenate message "#<motorAddr>Z|\r" */
		tcflush(global->fd, TCIFLUSH); 									/* flush input buffer */
		sport_send_msg(msg, global->fd);								/* send message */
		sport_read_msg(msg, global->fd);								/* read response */
//		pthread_mutex_unlock(global->lock);								/* unlock */
//		printf("readSetMethod: unlocked mutex\n");
		/* prepare user output */
#define OUTPUT
#ifdef OUTPUT
		/* variables for set configuration with allowed values and datatype */
		char strp[3] = ""; 	/* +1...+19 s8(int) */
		char strs[10] = "";	/* -100.000.000...+100.000.000 s32(int) */
		char stru[7] = "";	/* +1...+160.000 u32(int) */
		char stro[8] = "";	/* +1...+1.000.000 u32(int) */
		char strn[8] = "";	/* +1...+1.000.000 u32(int) */
		char strb[6] = "";	/* +1...65.535 u16(int) */
		char strB[6] = "";	/* +0...65.535 u16(int) */
		char strd[2] = "";	/* +0...+1 u8(int) */
		char strt[2] = ""; /* +0...+1 u8(int) */
		char strW[4] = "";	/* +0...254 u32(int) */
		char strP[6] = "";	/* +0...65.535 u16(int) */
		char strN[10] = "";	/* +0...+32 u8(int) */
		strncat(strp, strpbrk(msg,"p")+1, (size_t)(strpbrk(msg,"s")-strpbrk(msg,"p"))-1);
		strncat(strs, strpbrk(msg,"s")+1, (size_t)(strpbrk(msg,"u")-strpbrk(msg,"s"))-1);
		strncat(stru, strpbrk(msg,"u")+1, (size_t)(strpbrk(msg,"o")-strpbrk(msg,"u"))-1);
		strncat(stro, strpbrk(msg,"o")+1, (size_t)(strpbrk(msg,"n")-strpbrk(msg,"o"))-1);
		strncat(strn, strpbrk(msg,"n")+1, (size_t)(strpbrk(msg,"b")-strpbrk(msg,"n"))-1);
		strncat(strb, strpbrk(msg,"b")+1, (size_t)(strpbrk(msg,"B")-strpbrk(msg,"b"))-1);
		strncat(strB, strpbrk(msg,"B")+1, (size_t)(strpbrk(msg,"d")-strpbrk(msg,"B"))-1);
		strncat(strd, strpbrk(msg,"d")+1, (size_t)(strpbrk(msg,"t")-strpbrk(msg,"d"))-1);
		strncat(strt, strpbrk(msg,"t")+1, (size_t)(strpbrk(msg,"W")-strpbrk(msg,"t"))-1);
		strncat(strW, strpbrk(msg,"W")+1, (size_t)(strpbrk(msg,"P")-strpbrk(msg,"W"))-1);
		strncat(strP, strpbrk(msg,"P")+1, (size_t)(strpbrk(msg,"N")-strpbrk(msg,"P"))-1);
		strcat(strN, strpbrk(msg,"N")+1);
		strN[strlen(strN)-1]='\0';
		int p = atoi(strp);
		int s = atoi(strs);
		int u = atoi(stru);
//		int o = atoi(stro);
//		int n = atoi(strn);
//		int b = atoi(strb);
//		int B = atoi(strB);
//		int d = atoi(strd);
//		int t = atoi(strt);
//		int W = atoi(strtW),
//		int P = atoi(strP);
//		int N = atoi(strN);
		printf("strp = %s = %d \n", strp, p);
		printf("strs = %s = %d \n", strs, s);
		printf("stru = %s = %u \n", stru, u);
		printf("stro = %s\n", stro);
		printf("strn = %s\n", strn);
		printf("strb = %s\n", strb);
		printf("strB = %s\n", strB);
		printf("strd = %s\n", strd);
		printf("strt = %s\n", strt);
		printf("strW = %s\n", strW);
		printf("strP = %s\n", strP);
		printf("strN = %s\n", strN);
#endif
		UA_String tmp = UA_STRING_ALLOC(msg);
		UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]); /* user output */
//	}
//	else{
//		printf("trylock == 0 \n");
//		printf("couldn't use method, mutex was locked \n");
//		return UA_STATUSCODE_BADUNEXPECTEDERROR;
//	}


	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Read Set was called");
    return UA_STATUSCODE_GOOD;
}



static void
addReadSetMethod(UA_Server *server, const UA_NodeId *objectNodeId, globalstructMC *global){
	UA_Argument inputArgument;
	UA_Argument_init(&inputArgument);
	inputArgument.description = UA_LOCALIZEDTEXT("en-US", "An unnecessary string");
	inputArgument.name = UA_STRING("InputCommand");
	inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
	inputArgument.valueRank = UA_VALUERANK_SCALAR; /* what is this? */
	
	UA_Argument outputArgument;
	UA_Argument_init(&outputArgument);
	outputArgument.description = UA_LOCALIZEDTEXT("en-US", "A string, message that was sent");
	outputArgument.name = UA_STRING("OutputMessage");
	outputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
	outputArgument.valueRank = UA_VALUERANK_SCALAR; /* what is this? */
	
	UA_MethodAttributes sendAttr = UA_MethodAttributes_default;
	sendAttr.description = UA_LOCALIZEDTEXT("en-US","Reads the currently configured set");
	sendAttr.displayName = UA_LOCALIZEDTEXT("en-US","Read currently configured set");
	sendAttr.executable = true;
	sendAttr.userExecutable = true;
	UA_Server_addMethodNode(server, UA_NODEID_NULL,	 
							*objectNodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "ReadSet"),
                            sendAttr, &readSetMethodCallback,
                            1, &inputArgument, 1, &outputArgument, (void*)global, NULL);
}

static UA_StatusCode 
sportSendMsgMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
						 size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output)
{
	/* initial declerations */
	globalstructMC *global = (globalstructMC*)objectContext;
	UA_String *inputStr = (UA_String*)input->data; 									/* message to be sent */
	if(inputStr->length > 0 ){
		char* cmd = (char*)inputStr->data;
		char msg[80];
		MCcommand(global->motorAddr, cmd, NULL, msg);								/* concatenate message */
		tcflush(global->fd, TCIFLUSH); 												/* flush input buffer */
		sport_send_msg(msg, global->fd);											/* send message */
	#ifdef READ_RESPONSE
		sport_read_msg(msg, global->fd);
	#endif
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Send Msg was called");
		return UA_STATUSCODE_GOOD;
	}
	else{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Send Msg was called, but empty messages can't be sent");
		return UA_STATUSCODE_BADUNEXPECTEDERROR;
	}
}

static void
addSportSendMsgMethod(UA_Server *server, const UA_NodeId *objectNodeId, globalstructMC *global){
	UA_Argument inputArgument;
	UA_Argument_init(&inputArgument);
	inputArgument.description = UA_LOCALIZEDTEXT("en-US", "A string, command e.g. \"A\" or \"J1\" or \"J0\"");
	inputArgument.name = UA_STRING("InputCommand");
	inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
	inputArgument.valueRank = UA_VALUERANK_SCALAR; /* what is this? */
	
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
	UA_Server_addMethodNode(server, UA_NODEID_NULL,	 
							*objectNodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "SportSendMsg"),
                            sendAttr, &sportSendMsgMethodCallback,
                            1, &inputArgument, 1, &outputArgument, (void*)global, NULL);
}

/************************/
/* ADD OBJECT INSTANCES */
/************************/
static void
addMotorControllerObjectInstance(UA_Server *server, char *name, const UA_NodeId *nodeId, globalstructMC *global) 
{
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", name);
//    oAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
	printf("this is argument global->fd: %d \n", global->fd);
    UA_Server_addObjectNode(server, *nodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, name),
                            motorControllerTypeId, oAttr, global, NULL);
	/* add methods to object instance */
	addStartMotorMethod(server, nodeId, global);
	addStopMotorMethod(server, nodeId, global);
	addSportSendMsgMethod(server, nodeId, global);
	addReadSetMethod(server, nodeId, global);
	addCurrentAngleDataSourceVariable(server, nodeId, global);
	addCurrentStatusDataSourceVariable(server, nodeId, global);
}