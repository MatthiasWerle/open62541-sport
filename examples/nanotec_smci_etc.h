#define DISPLAY_STRING



/*****************************************/
/* Add Motor Controller Object Instances */
/*****************************************/
/* typedef for global variable */
	typedef struct {
		char ttyname[SIZE_TTYNAME];															/* tty portname e.g. /dev/ttyUSB0 */
		int fd;																	/* file descriptor of tty port */
		char motorAddr[SIZE_MOTORADDR];															/* preconfigured motor adress of nanotec stepper motor driver with integrated controller */
	} globalstructMC;

/* predefined identifier for later use */
UA_NodeId motorControllerTypeId = {1, UA_NODEIDTYPE_NUMERIC, {1001}};

static void
defineObjectTypes(UA_Server *server) {
	/* default values */
	UA_String defmn = UA_STRING("Nanotec"); 											/* default manufacturer name */
	UA_String defmodel = UA_STRING("SMCI47-S-2"); 									/* default model */
	/* default values placeholder */
	UA_Int32 defInt = 161;															/* default variable */

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

	/* Device Address */
    UA_VariableAttributes addrAttr = UA_VariableAttributes_default;
    addrAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Device adress");
    addrAttr.valueRank = UA_VALUERANK_SCALAR;
	addrAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    addrAttr.accessLevel = UA_ACCESSLEVELMASK_READ; 
    UA_Variant_setScalar(&addrAttr.value, &defInt, &UA_TYPES[UA_TYPES_INT32]);
	UA_NodeId addrId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, motorControllerTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "addrId"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), addrAttr, NULL, &addrId);
    /* Make the device address variable mandatory */
    UA_Server_addReference(server, addrId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
}

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
/*******************/
/* OTHER FUNCTIONS */
/*******************/

static void
MCcommand(const char* motorAddr, const char* cmd, const int* valPtr, char* msg)
{
	memset(msg,0,strlen(msg));											/* empty message */
	char valStr[15];
	memset(valStr,0,strlen(valStr));											/* empty message */
	if (valPtr != NULL){
		printf("*valPtr = %d \n", *valPtr);
		printf("before sprintf valStr = %s\n", valStr);
		sprintf(valStr, "%d", *valPtr);
		printf("after sprintf valStr = %s\n", valStr);
	}
	printf("after sprintf \n");
	strcpy(msg,"#"); 													/* write start sign # to message */
	strcat(msg, motorAddr);												/* append motor adress to message */
	strcat(msg, cmd);													/* append command to message */
	strcat(msg, valStr);												/* append value to message */
	strcat(msg, "\r");													/* append end sign to message */
}

/*************************************/
/* CALLBACK METHODS FOR OBJECT NODES */
/*************************************/

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
	UA_String *inputStr = (UA_String*)input->data; 						/* message to be sent */

	/* create message */ 
	char msg[50];
	char* cmd = (char*)inputStr->data;
	MCcommand(global->motorAddr, cmd, NULL, msg);
	/* send message */
	sport_send_msg(msg, global->fd);

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Send Msg was called");
    return UA_STATUSCODE_GOOD;
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
//							UA_NODEID_STRING(1, "SportSendMsg"),
//							UA_NODEID_STRING(1, "MCId1"),					/* motor controller 1 as parent */
//							UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), 	/* objectfolder as parent */
							*objectNodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "SportSendMsg"),
                            sendAttr, &sportSendMsgMethodCallback,
                            1, &inputArgument, 1, &outputArgument, (void*)global, NULL);
}

static UA_StatusCode 
startMotorMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
						 size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output)
{
	globalstructMC *global = (globalstructMC*)objectContext;
	
	char msg[] = "test";
	memset(msg,0,strlen(msg));											/* empty message */
	MCcommand(global->motorAddr, "A", NULL, msg);
	printf("fyi start motor msg = %s \n", msg);
	sport_send_msg(msg, global->fd);									/* send message */
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
                            UA_QUALIFIEDNAME(1, "SportSendMsg"),
                            sendAttr, &startMotorMethodCallback,
                            0, NULL, 0, NULL, (void*)global, NULL);
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
                            motorControllerTypeId, /* this refers to the object type identifier */
                            oAttr, global, NULL);
	/* add methods to object instance */
	addSportSendMsgMethod(server, nodeId, global);
	addStartMotorMethod(server, nodeId, global);
}