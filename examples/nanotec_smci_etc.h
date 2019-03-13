/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Nanotec Motor controller implementation on OPC UA Server
 * --------------------------------------------------------
 *
 * In OPC UA-based architectures, servers are typically situated near the source
 * of information. In an industrial context, this translates into servers being
 * near the physical process and clients consuming the data at runtime. In the
 * previous tutorial, we saw how to add variables to an OPC UA information
 * model. This tutorial shows how to connect a variable to runtime information,
 * for example from measurements of a physical process. For simplicity, we take
 * the system clock as the underlying "process".
 *
 * The following code snippets are each concerned with a different way of
 * updating variable values at runtime. Taken together, the code snippets define
 * a compilable source file.
 *
 * Updating variables manually
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * As a starting point, assume that a variable for a value of type
 * :ref:`datetime` has been created in the server with the identifier
 * "ns=1,s=current-time". Assuming that our applications gets triggered when a
 * new value arrives from the underlying process, we can just write into the
 * variable. */

/*************************************/
/* MOTOR CONTROLLER TYPE DEFINITIONS */
/*************************************/

/* typedef for global variable */
	typedef struct {
		char ttyname[SIZE_TTYNAME];														/* tty portname e.g. /dev/ttyUSB0 */
		int fd;																			/* file descriptor of tty port */
		char motorAddr[SIZE_MOTORADDR];													/* preconfigured motor adress of nanotec stepper motor driver with integrated controller */
		pthread_mutex_t *lock;															/* yet not implemented */
		fd_set *readfds;																/* yet not implemented */
		struct timeval *tv;																/* yet not implemented */
	} globalstructMC;

/* predefined identifier for later use */
UA_NodeId motorControllerTypeId = {1, UA_NODEIDTYPE_NUMERIC, {1001}};
UA_NodeId motorSettingsTypeId = {1, UA_NODEIDTYPE_NUMERIC, {1002}};

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
    UA_Variant_setScalar(&modelAttr.value, &defmodel, &UA_TYPES[UA_TYPES_STRING]);
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

	/* Define the object type for "Motor Settings" */
	UA_ObjectTypeAttributes mtAttr = UA_ObjectTypeAttributes_default;
	mtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MotorSettingsType");
	UA_Server_addObjectTypeNode(server, motorSettingsTypeId,
								motorControllerTypeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
								UA_QUALIFIEDNAME(1, "MotorSettingsType"), mtAttr,
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

/* Concatenates a string as command which is readable for nanotec(R) motor controller models 
 * SMCI12, SMCI33, SMCI35, SMCI36, SMCI47-S, SMCP33, PD2-N, PD4-N and PD6-N
 *
 * @param motorAddr The motor Adress 
 * @param cmd A command from the programming manual for the described motor controllers
 * @param valPtr An optional pointer to an integer with the value for the command
 * @return Returns a status code. */
static UA_INLINE UA_StatusCode
MCcommand(const char* motorAddr, const char* cmd, const int* valPtr, char* msg)
{
	memset(msg,'\0',strlen(msg));											/* empty message */
	char valStr[20];
	memset(valStr,'\0',strlen(valStr)); 									/* empty message */
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

/* Look-up-table for the description of position modes */
static UA_INLINE UA_StatusCode
get_posMode_description(int posModeIdx, char* posMode){
	switch(posModeIdx) {
		case 1: strcpy(posMode, "relative pos. mode"); break;
		case 2: strcpy(posMode, "absolut pos. mode"); break;
		case 3: strcpy(posMode, "intern ref. traverse"); break;
		case 4: strcpy(posMode, "extern ref. traverse"); break;
		case 5: strcpy(posMode, "rotational speed mode"); break;
		case 6: strcpy(posMode, "flag position mode"); break;
		case 7: strcpy(posMode, "clock mode man. left"); break;
		case 8: strcpy(posMode, "clock mode man. right"); break;
		case 9: strcpy(posMode, "clock mode intern ref. traverse"); break;
		case 10: strcpy(posMode, "clock mode extern ref. traverse"); break;
		case 11: strcpy(posMode, "analogue rot. speed mode"); break;
		case 12: strcpy(posMode, "joystick mode"); break;
		case 13: strcpy(posMode, "analoge positon mode"); break;
		case 14: strcpy(posMode, "HW-reference mode"); break;
		case 15: strcpy(posMode, "torque mode"); break;
		case 16: strcpy(posMode, "CL quick test mode"); break;
		case 17: strcpy(posMode, "CL test mode"); break;
		case 18: strcpy(posMode, "CL autotune mode"); break;
		case 19: strcpy(posMode, "CL quick test mode 2"); break;
		default: strcpy(posMode, "Couldn't find out which position mode is active"); break;
	}
	return UA_STATUSCODE_GOOD;
}
/*************************/
/* DATA SOURCE VARIABLES */
/*************************/

/* Request to return motor position from motor controller, read response, 
 * overwrite value of datasource variable and display it with printf */
static UA_StatusCode
readCurrentAngle(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
                UA_DataValue *dataValue) {
	globalstructMC *global = (globalstructMC*)nodeContext;
	char msg[30];
	char* cmd = "C";													/* Command to return position */
	MCcommand(global->motorAddr, cmd, NULL, msg);						/* concatenate message */
	tcflush(global->fd, TCIFLUSH); 										/* flush input buffer */
	sport_send_msg(msg, global->fd);									/* send message */
	sport_read_msg(msg, global->fd);									/* read response */

	if (msg[0] != '\0'){
		if ( (strpbrk(msg,cmd))[strlen(cmd)] != '\r'){
			UA_Int32 angle = atoi(strpbrk(msg, cmd)+strlen(cmd));							/* convert char* to int */
			UA_Variant_setScalarCopy(&dataValue->value, &angle, &UA_TYPES[UA_TYPES_INT32]);
			dataValue->hasValue = true;											/* probably obsolet? */
		}
		else{
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error: No response due to wrong motor adress \n");
			printf("msg = %s\n", msg);
		}
	}

    return UA_STATUSCODE_GOOD;
}

/* Adjusts the position mode of the motor controller to absolute, sets it's traverse path and runs the motor */
static UA_StatusCode
writeCurrentAngle(UA_Server *server,
                 const UA_NodeId *sessionId, void *sessionContext,
                 const UA_NodeId *nodeId, void *nodeContext,
                 const UA_NumericRange *range, const UA_DataValue *dataValue) {
	globalstructMC *global = (globalstructMC*)nodeContext;
	char msg[30];
	int* position = (int*)dataValue->value.data;
	if(global->fd > 0){
		MCcommand(global->motorAddr, "p2", NULL, msg);
		sport_send_msg(msg, global->fd);									/* config: positioning mode absolute */
		MCcommand(global->motorAddr, "s", position, msg);
		sport_send_msg(msg, global->fd);									/* config: traverse path to input position*/
		MCcommand(global->motorAddr, "N0", NULL, msg);
		sport_send_msg(msg, global->fd);									/* config: no following set to be performed */
		MCcommand(global->motorAddr, "A", NULL, msg);
		sport_send_msg(msg, global->fd);									/* Start Motor */
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Motor starts and moves to configured angle position");
	}
	else
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Bad filedescriptor! Can't send message.");
	return UA_STATUSCODE_GOOD;
}

/* Adds current angle of the motor as datasource variable to an object node of a motor controller instance
 * 
 * @param server The server object.
 * @param nodeId A pointer to the parent node Id
 * @param global A pointer to a struct of type globalstructMC */
static void
addCurrentAngleDataSourceVariable(UA_Server *server, const UA_NodeId *nodeId, globalstructMC *global) {
	char name[50];
	strcpy(name, (char*)((*nodeId).identifier.string.data));
	strcat(name, "-current-angle-datasource");

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "angle - data source");
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

/* Request to return motor status from motor controller, read response, 
 * overwrite value of datasource variable with status description and 
 * display numeric status with printf */
static UA_StatusCode
readCurrentStatus(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
                UA_DataValue *dataValue) {
	globalstructMC *global = (globalstructMC*)nodeContext;
	char msg[30];
	char* cmd = "$";
	char* stat = (char*) malloc(1);
	memset(stat, '\0', 1);

	MCcommand(global->motorAddr, cmd, NULL, msg);						/* concatenate message */
	tcflush(global->fd, TCIFLUSH); 										/* flush input buffer */
	sport_send_msg(msg, global->fd);									/* send message */
	sport_read_msg(msg, global->fd);									/* read response */

	/* value of motor controller response should be an 8 bit bitmask, but is actually 24 bit long... */
	if (msg[0] != '\0'){
		if ( (strpbrk(msg,cmd))[strlen(cmd)] != '\0'){
			UA_Int32 status = atoi(strpbrk(msg, cmd)+strlen(cmd));		/* convert char* to int */
			if (status & 1){
				stat = (char*)realloc(stat, strlen(stat)+26);
				strcat(stat, "Bit0: Control is ready\n");
			}
			else if ((status & 1) == 0){
				stat = (char*)realloc(stat, strlen(stat)+25);
				strcat(stat, "Bit0: Control is busy\n");
			}
			if (status & 2){
				stat = (char*)realloc(stat, strlen(stat)+33);
				strcat(stat, "Bit1: Nullposition is reached\n");
			}
			if (status & 4){
				stat = (char*)realloc(stat, strlen(stat)+24);
				strcat(stat, "Bit2: Position Error\n");
			}
			if (status & 8){
				stat = (char*)realloc(stat, strlen(stat)+163);
				strcat(stat, "Bit3: Input1 is set while control is ready. Happens when the control was startet via input1 and the control was faster ready again than the input could be set.\n");
			}
			if (status & 16){
				stat = (char*)realloc(stat, strlen(stat)+91);
				strcat(stat, "Bit4: Warning: This bit is set, though it's designated to be unset always\n");
			}
			if ((status & 32) == 0){
				stat = (char*)realloc(stat, strlen(stat)+91);
				strcat(stat, "Bit5: Warning: This bit is unset, though it's designated to be set always\n");
			}
			if (status & 64){
				stat = (char*)realloc(stat, strlen(stat)+91);
				strcat(stat, "Bit6: Warning: This bit is set, though it's designated to be unset always\n");
			}
			if ((status & 128) == 0){
				stat = (char*)realloc(stat, strlen(stat)+91);
				strcat(stat, "Bit7: Warning: This bit is unset, though it's designated to be set always\n");
			}
			UA_String tmp = UA_STRING_ALLOC(stat);
			UA_Variant_setScalarCopy(&dataValue->value, &tmp, &UA_TYPES[UA_TYPES_STRING]);
			dataValue->hasValue = true;											/* probably obsolet? */
		}
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error: No response due to wrong motor adress \n");
	}
		
    return UA_STATUSCODE_GOOD;
}

/* Adds current status of the motor as datasource variable to an object node of a motor controller instance
 * 
 * @param server The server object.
 * @param nodeId A pointer to the parent node Id
 * @param global A pointer to a struct of type globalstructMC */
static void
addCurrentStatusDataSourceVariable(UA_Server *server, const UA_NodeId *nodeId, globalstructMC *global) {
	char name[50];
	strcpy(name, (char*)((*nodeId).identifier.string.data));
	strcat(name, "-current-status-datasource");

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "status - data source");
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

/* Request to return motors current position from motor controller, read response, 
 * overwrite value of datasource variable and display it with printf */
static UA_StatusCode
readCurrentPositionMode(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
                UA_DataValue *dataValue) {
	globalstructMC *global = (globalstructMC*)nodeContext;
	char msg[30];
	char* cmd = "Zp";
	char posMode[50];
	MCcommand(global->motorAddr, cmd, NULL, msg);						/* concatenate message */
	tcflush(global->fd, TCIFLUSH); 										/* flush input buffer */
	sport_send_msg(msg, global->fd);									/* send message */
	sport_read_msg(msg, global->fd);									/* read response */

	if (msg[0] != '\0'){
		if ( (strpbrk(msg,cmd))[strlen(cmd)] != '\r'){
			get_posMode_description(atoi(strpbrk(msg, cmd)+strlen(cmd)), posMode);
			
			UA_String tmp = UA_STRING_ALLOC(posMode);
			UA_Variant_setScalarCopy(&dataValue->value, &tmp, &UA_TYPES[UA_TYPES_STRING]);
			dataValue->hasValue = true;											/* probably obsolet? */
		}
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error: No response due to wrong motor adress \n");
	}

    return UA_STATUSCODE_GOOD;
}

/* Adds current position mode of the motor as datasource variable to an object node of a motor controller instance
 * 
 * @param server The server object.
 * @param nodeId A pointer to the parent node Id
 * @param global A pointer to a struct of type globalstructMC */
static void
addCurrentPositionModeDataSourceVariable(UA_Server *server, const UA_NodeId *nodeId, globalstructMC *global) {
	char name[50];
	strcpy(name, (char*)((*nodeId).identifier.string.data));
	strcat(name, "-current-position-mode-datasource");

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "position-mode - data source");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;

    UA_NodeId currentNodeId = UA_NODEID_STRING(1, name);
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, name);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_DataSource posModeDataSource;
	posModeDataSource.read = readCurrentPositionMode;
    UA_Server_addDataSourceVariableNode(server, currentNodeId, *nodeId,
                                        parentReferenceNodeId, currentName,
                                        variableTypeNodeId, attr,
                                        posModeDataSource, (void*)global, NULL);
}

/* Request to return current traverse path from motor controller, read response, 
 * overwrite value of datasource variable and display it with printf */
static UA_StatusCode
readCurrentTraversePath(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
                UA_DataValue *dataValue) {
	globalstructMC *global = (globalstructMC*)nodeContext;
	char msg[30];
	char* cmd = "Zs";													/* Command to return position */
	MCcommand(global->motorAddr, cmd, NULL, msg);						/* concatenate message */
	tcflush(global->fd, TCIFLUSH); 										/* flush input buffer */
	sport_send_msg(msg, global->fd);									/* send message */
	sport_read_msg(msg, global->fd);									/* read response */

	if (msg[0] != '\0'){
		if ( (strpbrk(msg,cmd))[strlen(cmd)] != '\r'){
			UA_Int32 traversePath = atoi(strpbrk(msg, cmd)+strlen(cmd));							/* convert char* to int */
			UA_Variant_setScalarCopy(&dataValue->value, &traversePath, &UA_TYPES[UA_TYPES_INT32]);
			dataValue->hasValue = true;											/* probably obsolet? */
		}
		else{
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error: No response due to wrong motor adress \n");
			printf("msg = %s\n", msg);
		}
	}
    return UA_STATUSCODE_GOOD;
}

/* Adjusts the traverse path of the motor controller to absolute, sets it's traverse path and runs the motor */
static UA_StatusCode
writeCurrentTraversePath(UA_Server *server,
                 const UA_NodeId *sessionId, void *sessionContext,
                 const UA_NodeId *nodeId, void *nodeContext,
                 const UA_NumericRange *range, const UA_DataValue *dataValue) {
	globalstructMC *global = (globalstructMC*)nodeContext;
	char msg[30];
	char* cmd = "s";														/* Command to set traverse path */
	int* traversePath = (int*)dataValue->value.data;
	if(global->fd > 0){
		MCcommand(global->motorAddr, cmd, traversePath, msg);				/* concatenate message */
		printf("DEBUG: msg = %s\n",msg);
		tcflush(global->fd, TCIFLUSH); 										/* flush input buffer */
		sport_send_msg(msg, global->fd);									/* send message */
#ifdef READ_RESPONSE
		sport_read_msg(msg, global->fd);									/* read response */
#endif
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Motors traverse path is set");
	}
	else
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Bad filedescriptor! Can't send message.");
	return UA_STATUSCODE_GOOD;
}

/* Adds current traverse path of the motor as datasource variable to an object node of a motor controller instance
 * 
 * @param server The server object.
 * @param nodeId A pointer to the parent node Id
 * @param global A pointer to a struct of type globalstructMC */
static void
addCurrentTraversePathDataSourceVariable(UA_Server *server, const UA_NodeId *nodeId, globalstructMC *global) {
	char name[50];
	strcpy(name, (char*)((*nodeId).identifier.string.data));
	strcat(name, "-current-traverse-path-datasource");

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "traverse path - data source");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId currentNodeId = UA_NODEID_STRING(1, name);
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, name);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_DataSource traversePathDataSource;
	traversePathDataSource.read = readCurrentTraversePath;
	traversePathDataSource.write = writeCurrentTraversePath;
    UA_Server_addDataSourceVariableNode(server, currentNodeId, *nodeId,
                                        parentReferenceNodeId, currentName,
                                        variableTypeNodeId, attr,
                                        traversePathDataSource, (void*)global, NULL);
}

/******************************************************************/
/* CALLBACK METHODS FOR OBJECT INSTANCES OF TYPE MOTOR CONTROLLER */
/******************************************************************/

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
		memset(msg,'\0',sizeof(msg)); 										/* empty message */
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
	memset(msg,'\0',sizeof(msg));											/* empty message */
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
		memset(msg,'\0',sizeof(msg));
		char* cmd = "Z|";
		MCcommand(global->motorAddr, cmd, NULL, msg);					/* concatenate message "#<motorAddr>Z|\r" */
		tcflush(global->fd, TCIFLUSH); 									/* flush input buffer */
		sport_send_msg(msg, global->fd);								/* send message */
		sport_read_msg(msg, global->fd);								/* read response */
//		pthread_mutex_unlock(global->lock);								/* unlock */
//		printf("readSetMethod: unlocked mutex\n");

		if (msg[0] != '\0'){
			if ( (strpbrk(msg,cmd))[strlen(cmd)] != '\r'){
				printf("DEBUG: msg = %s\n",msg);
				/* prepare user output */
				/* variables for set configuration with allowed values and datatype; string-length is decimal numberlength + 2 */
				char strp[4] = ""; 	/* +1...+19 s8(int) */
				char strs[11] = "";	/* -100.000.000...+100.000.000 s32(int) */
				char stru[8] = "";	/* +1...+160.000 u32(int) */
				char stro[9] = "";	/* +1...+1.000.000 u32(int) */
				char strn[9] = "";	/* +1...+1.000.000 u32(int) */
				char strb[7] = "";	/* +1...65.535 u16(int) */
				char strB[7] = "";	/* +0...65.535 u16(int) */
				char strd[3] = "";	/* +0...+1 u8(int) */
				char strt[3] = ""; /* +0...+1 u8(int) */
				char strW[5] = "";	/* +0...254 u32(int) */
				char strP[7] = "";	/* +0...65.535 u16(int) */
				char strN[11] = "";	/* +0...+32 u8(int) */
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

				int i = 0;
				int parameters[12] = {atoi(strp), atoi(strs), atoi(stru), atoi(stro), atoi(strn), atoi(strb), atoi(strB), atoi(strd), atoi(strt), atoi(strW), atoi(strP), atoi(strN)};
				char posMode[50];
				printf("posMode = %d\n", parameters[0]);
				get_posMode_description(parameters[0], posMode);

				UA_String tmp = UA_STRING_ALLOC(msg);
				UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]); /* user output; TODO: single output for every extracted numerical variable of the read set */
				tmp = UA_STRING_ALLOC(posMode);
				UA_Variant_setScalarCopy(output+1, &tmp, &UA_TYPES[UA_TYPES_STRING]); /* user output; TODO: single output for every extracted numerical variable of the read set */
				for (i=2; i<=12; i++)
					UA_Variant_setScalarCopy(output+i, &parameters[i-1], &UA_TYPES[UA_TYPES_INT32]);
			}
			else
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error: No response due to wrong motor adress");
		}

//		printf("trylock == 0 \n");
//		printf("couldn't use method, mutex was locked \n");
//		return UA_STATUSCODE_BADUNEXPECTEDERROR;

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

	UA_Argument outputArgument[13];
	int i = 0;
	char istr[3];
	char outputArgumentName[17] = "OutputMessage";
	for(i = 2; i <= 12; i++){
		memset(outputArgumentName+13, '\0', 2);
		sprintf(istr, "%d", i+1);
		strcat(outputArgumentName, istr);
		UA_Argument_init(&outputArgument[i]);
		outputArgument[i].name = UA_STRING_ALLOC(outputArgumentName);
		outputArgument[i].valueRank = UA_VALUERANK_SCALAR;
		outputArgument[i].dataType = UA_TYPES[UA_TYPES_INT32].typeId;
	}
	outputArgument[0].name = UA_STRING("OutputMessage1");
	outputArgument[0].valueRank = UA_VALUERANK_SCALAR;
	outputArgument[0].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
	outputArgument[1].name = UA_STRING("OutputMessage2");
	outputArgument[1].valueRank = UA_VALUERANK_SCALAR;
	outputArgument[1].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
	outputArgument[0].description = UA_LOCALIZEDTEXT("en-US", "current set");
	outputArgument[1].description = UA_LOCALIZEDTEXT("en-US", "position mode");
	outputArgument[2].description = UA_LOCALIZEDTEXT("en-US", "traverse path");
	outputArgument[3].description = UA_LOCALIZEDTEXT("en-US", "start step frequency");
	outputArgument[4].description = UA_LOCALIZEDTEXT("en-US", "max. step frequency");
	outputArgument[5].description = UA_LOCALIZEDTEXT("en-US", "2nd max. step frequency");
	outputArgument[6].description = UA_LOCALIZEDTEXT("en-US", "acceleration ramp");
	outputArgument[7].description = UA_LOCALIZEDTEXT("en-US", "brake ramp");
	outputArgument[8].description = UA_LOCALIZEDTEXT("en-US", "rotation direction");
	outputArgument[9].description = UA_LOCALIZEDTEXT("en-US", "invert direction between repeated sets");
	outputArgument[10].description = UA_LOCALIZEDTEXT("en-US", "repeats");
	outputArgument[11].description = UA_LOCALIZEDTEXT("en-US", "break between repeats and followed sets");
	outputArgument[12].description = UA_LOCALIZEDTEXT("en-US", "number of following set");
	
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
                            1, &inputArgument, 13, outputArgument, (void*)global, NULL);
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
		UA_String tmp = UA_STRING_ALLOC(msg);
		UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]); 
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

/*****************************************/
/* Add Motor Controller Object Instances */
/*****************************************/

static void
addMotorControllerObjectInstance(UA_Server *server, char *nameMC, const UA_NodeId *nodeId, globalstructMC *global){
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", nameMC);
//    oAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addObjectNode(server, *nodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, nameMC),
                            motorControllerTypeId, oAttr, global, NULL);
	printf("\n");
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Add Methods and Datasource Variables to object instance with name = \"%s\"", nameMC);

	/* add methods to object instance */
	addStartMotorMethod(server, nodeId, global);
	addStopMotorMethod(server, nodeId, global);
	addSportSendMsgMethod(server, nodeId, global);
	addReadSetMethod(server, nodeId, global);

	/* add datasources to object instance */
	addCurrentAngleDataSourceVariable(server, nodeId, global);
	addCurrentStatusDataSourceVariable(server, nodeId, global);
	addCurrentPositionModeDataSourceVariable(server, nodeId, global);
	addCurrentTraversePathDataSourceVariable(server, nodeId, global);

#ifdef NEW
	/* add motor settings object instance as child to motor controller object instance */
	char nameMSet[50];
	strcpy(nameMSet, nameMC);
	strcat(nameMSet, "MotorSettings");
	addMotorSettingsObjectInstance(server, nameMSet, nodeId, global);
#endif
}

#ifdef NEW
static void
addMotorSettingsObjectInstance(UA_Server *server, char *name, const UA_NodeId *nodeId, globalstructMC *global){
	UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
	oAttr.displayName = UA_LOCALIZEDTEXT("en-US", name);
	UA_Server_addObjectNode(server, *nodeId,
							UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
							UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
							UA_QUALIFIEDNAME(1, name),
							motorSettingsTypeId, oAttr, global, NULL);

	UA_Server_addObjectNode(...);

}
#endif

