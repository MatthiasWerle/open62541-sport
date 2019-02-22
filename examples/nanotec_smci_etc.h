#define DISPLAY_STRING


/*****************************************/
/* Add Motor Controller Object Instances */
/*****************************************/

/* predefined identifier for later use */
UA_NodeId motorControllerTypeId = {1, UA_NODEIDTYPE_NUMERIC, {1001}};

static void
defineObjectTypes(UA_Server *server) {
	UA_Int32 defInt = 161;														/* default variable */
	UA_String defStr = UA_STRING("Alerta");										/* default variable */
	
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

	/* ttyname */
    UA_VariableAttributes ttynameAttr = UA_VariableAttributes_default;
    ttynameAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TTYname");
    ttynameAttr.valueRank = UA_VALUERANK_SCALAR;
	ttynameAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    ttynameAttr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_Variant_setScalar(&ttynameAttr.value, &defStr, &UA_TYPES[UA_TYPES_STRING]);
	UA_NodeId ttynameId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, motorControllerTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TTYname"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), ttynameAttr, NULL, &ttynameId);
    /* Make the ttyname variable mandatory */
    UA_Server_addReference(server, ttynameId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);

	/* Filedescriptor */
    UA_VariableAttributes fdAttr = UA_VariableAttributes_default;
    fdAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Filedescriptor for serial port");
    fdAttr.valueRank = UA_VALUERANK_SCALAR;
	fdAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    fdAttr.accessLevel = UA_ACCESSLEVELMASK_READ; 
    UA_Variant_setScalar(&fdAttr.value, &defInt, &UA_TYPES[UA_TYPES_INT32]);
	UA_NodeId fdId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, motorControllerTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "fdId"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), fdAttr, NULL, &fdId);
    /* Make the fd variable mandatory */
    UA_Server_addReference(server, fdId,
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

static void
addMotorControllerObjectInstance(UA_Server *server, char *name, const UA_NodeId *nodeId, int *fd) 
{
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", name);
//    oAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
	printf("this is argument *fd: %d \n", *fd);
    UA_Server_addObjectNode(server, *nodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, name),
                            motorControllerTypeId, /* this refers to the object type
                                           identifier */
                            oAttr, fd, NULL);
		addSportSendMsgMethod(server, nodeId);			/* add method to object instance */
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

