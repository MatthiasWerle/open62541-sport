/* Author Matthias Markus Werle 2019
 * 
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 * 
 * Peace, Love and Unicorns! You're free to use the work but pleased to use it for a contribution
 * to a better world without discrimination against anyone, where the only domination is 
 * consentual for personal pleasures and where no form of unnecessary violence is tolerated. 
 * 
 * Kaffee oder lieber Tee? -> Liberté! */
 
/************************/
/* edit node attributes */
/************************/
static UA_StatusCode
getChildNodeId(UA_Server *server, const UA_NodeId *parentNodeId, char* browsename, 
								 UA_NodeId *childNodeId) {
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "getNodeId Method called");

	/* Find NodeId of status child variable */
	UA_RelativePathElement rpe;
	UA_RelativePathElement_init(&rpe);
	rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT); 	/* What's this? */
	rpe.isInverse = false;												/* What's this? */
	rpe.includeSubtypes = false;										/* What's this? */
	rpe.targetName = UA_QUALIFIEDNAME(1, browsename);

	UA_BrowsePath bp;
	UA_BrowsePath_init(&bp);
	bp.startingNode = *parentNodeId;
	bp.relativePath.elementsSize = 1;
	bp.relativePath.elements = &rpe;	

	UA_BrowsePathResult bpr =
		UA_Server_translateBrowsePathToNodeIds(server, &bp);
	if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
		return bpr.statusCode; }

	/* Set the childNodeId */
	if (childNodeId->identifierType == UA_NODEIDTYPE_NUMERIC)
		childNodeId->identifier.numeric = bpr.targets[0].targetId.nodeId.identifier.numeric;
	else if (childNodeId->identifierType == UA_NODEIDTYPE_STRING)
		childNodeId->identifier.string = bpr.targets[0].targetId.nodeId.identifier.string;
	
	UA_BrowsePathResult_clear(&bpr);
	return UA_STATUSCODE_GOOD;
}

static UA_StatusCode 
setChildAttrVal(UA_Server *server, const UA_NodeId *nodeId, char* browsename, 
								UA_Variant newval, UA_DataType type) {
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "set Child Attribute Value Method called");

	/* Find NodeId of status child variable */
	UA_RelativePathElement rpe;
	UA_RelativePathElement_init(&rpe);
	rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
	rpe.isInverse = false;
	rpe.includeSubtypes = false;
	rpe.targetName = UA_QUALIFIEDNAME(1, browsename);

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
	printf("nodeId relatively searched: %u\n", (unsigned int)bpr.targets[0].targetId.nodeId.identifier.numeric);
	UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, newval);
	UA_BrowsePathResult_clear(&bpr);
		
	return UA_STATUSCODE_GOOD;
}


/* works but unnecesary */
//static UA_StatusCode
//setttyname(UA_Server *server, const UA_NodeId *nodeId, char *newttyname) {
//	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "set ttyname called");
//	
//	/* Find NodeId of status child variable */
//    UA_RelativePathElement rpe;
//    UA_RelativePathElement_init(&rpe);
//    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
//    rpe.isInverse = false;
//    rpe.includeSubtypes = false;
//    rpe.targetName = UA_QUALIFIEDNAME(1, "TTYname");
//
//    UA_BrowsePath bp;
//    UA_BrowsePath_init(&bp);
//    bp.startingNode = *nodeId;
//    bp.relativePath.elementsSize = 1;
//    bp.relativePath.elements = &rpe;	
//
//    UA_BrowsePathResult bpr =
//        UA_Server_translateBrowsePathToNodeIds(server, &bp);
//    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
//		return bpr.statusCode; }
//		
//    /* Set the status value */
//    UA_String ttyname = UA_STRING(newttyname);
//    UA_Variant value;
//    UA_Variant_setScalar(&value, &ttyname, &UA_TYPES[UA_TYPES_STRING]);
//    UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
//    UA_BrowsePathResult_clear(&bpr);
//		
//	return UA_STATUSCODE_GOOD;
//}


