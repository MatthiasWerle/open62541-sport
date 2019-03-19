/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 * Peace, Love and Libertarian Anarchy, dear comrades! (A) */

/**
 * OPC UA server implementation for control of multiple Nanotec motor controllers
 * ------------------------------------------------------------------------------
 *
 * This code is suitable for the following motor controller models
 * SMCI12, SMCI33, SMCI35, SMCI36, SMCI47-S, SMCP33, PD2-N, PD4-N and PD6-N
 *
 * Description: Code utilities
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * The following code will run an OPC UA server and establishes a communication with a 
 * predefined number of motor controllers either connected alltogether to a single serial
 * port or otherwise all connected to single serial ports.
 * 
 * Every motor controller is represented by an object instance which owns several callback methods
 * and datasources of the motor controllers parameters.
 * 
 * Notes on Compatibality  
 * ^^^^^^^^^^^^^^^^^^^^^^ 
 * The code was written on Linux (Ubuntu 18.04.2 LTS) and tested for Raspbian GNU/Linux 9.4 (stretch)
 * 
 * Warnings: Important notes for for initial use
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Connect everything properly according to the manufacturers technical manual. 
 * For inital use of a motor controller read comments in motorcontroller_initial_setup.c 
 * and run it's executable build to adjust the motorcontrollers configurations (e.g. phase current) 
 * to prevent damaging the hardware.*/

/**********************/
/* INCLUDED LIBRARIES */
/**********************/

#include <signal.h>
#include <ua_server.h>
#include <ua_config_default.h>
#include <ua_log_stdout.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h> 				/* lib for serial port communication */
#include <unistd.h>
#include <stdint.h>
#include <time.h>					/* clock() function */
#include <sys/select.h>				/* defines select() */
#include <sys/types.h>	
#include <sys/time.h>				/* defines variable type timeval */

#include "server_robolink_config.h"	/* includes macros which can be adjustet by users; note: realising them with ccmake would be better than to manually rewrite the header file */
#include "sport.h" 					/* serial port communication */
#include "nanotec_smci_etc.h"		/* command list for step motor controllers from nanotec */

/**************************/
/* TODO: WORK IN PROGRESS */		/* CTRL+F: "TODO" or "WIP" */
/**************************/

#define WIP 

/*********************************/
/* FUNCTIONS AND TYPEDEFINITIONS */
/*********************************/

/* server stop handler */
UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

/*************/
/* MAIN LOOP */
/*************/

int main(int argc, char** argv){

    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler); 								/* catch ctrl-c */

    /* Create a server listening on port 4840 */
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

	/* DECLARATIONS */
	/* declare various variales, some with variable size */
	int i, j;													/* index for loops */
	int MCIdx[N_MCCOMMANDS];
	char* nameId[N_MOTORCONTROLLERS];							/* pointer to string with name of later used motor controller obj instance nodeId's */
	char* nameBrowse[N_MOTORCONTROLLERS];						/* pointer to string with name of later used browsenames */
	char* nameNr[N_MOTORCONTROLLERS];							/* pointer to string with name of later used number to generate nodeId's name */
	char* ttyname[N_MOTORCONTROLLERS];							/* pointer to string with ttyname for every connected motor controller */
	char* nameMSet[N_MOTORCONTROLLERS];

	UA_NodeId nodeIdMC[N_MOTORCONTROLLERS];						/* Array of NodeId's for every Motor Controller Object Instance */
	UA_NodeId nodeIdMSet[N_MOTORCONTROLLERS];					/* Array of NodeId's for every Motor Settings Object Instance */

	MCObjectInstanceContext MCObj[N_MOTORCONTROLLERS];			/* data context of one motor controller object instance */
	MCCommand MCLib[N_MCCOMMANDS];								/* struct array as list of motor controller commands */
	MCDataSourceContext context[N_MOTORCONTROLLERS][N_MCCOMMANDS]; /* struct matrix with one element for each motor controller and each command */

#ifdef WIP
	char cwd[1000]; 											/* current working directory */
	char MCLib_fp[strlen(cwd)+strlen(MCLIB_FILENAME)+1]; 		/* filepath to MCLib.csv which inhibits the motor controller command list */
	char* tmp_ptr = NULL;
	int bufsize = 600;											/* buffersize to read motor controller commands from csv */
	char linebuf[bufsize];										/* buffer to read motor controller commands from csv */
	char* field = "";
	char delims[] = ",";
//	char *result = NULL;
	FILE *MCLibFile;											/* file descriptor for csv file with motor controller commands */
#endif

	/* Example Assignment of motor controller command*/
	/* TODO: Write commands into a csv file and read it from there, CTRL+F "WIP" */
	enum {s, W, v, p, p1, dollarsign};
	MCLib[s].name = "traversepath";
	MCLib[s].nameDisplay = "traverse-path";
	MCLib[s].write = 1;
	MCLib[s].cmd_write = "s";
	MCLib[s].cmd_read = "Zs";
	MCLib[s].type = 0;
	MCLib[s].min = -100000000;
	MCLib[s].max = +100000000;
	MCLib[W].name = "repeats";
	MCLib[W].nameDisplay = "repeats-of-current-set";
	MCLib[W].write = 1;
	MCLib[W].cmd_write = "W";
	MCLib[W].cmd_read = "ZW";
	MCLib[W].type = 0;
	MCLib[W].min = 0;
	MCLib[W].max = 254;
	MCLib[v].name = "firmwareversion";
	MCLib[v].nameDisplay = "firmware-version";
	MCLib[v].write = 0;
	MCLib[v].cmd_write = "";
	MCLib[v].cmd_read = "v";
	MCLib[v].type = 1;
	MCLib[v].min = 0;
	MCLib[v].max = 0;
	MCLib[p].name = "positionmode";
	MCLib[p].nameDisplay = "position-mode";
	MCLib[p].write = 1;
	MCLib[p].cmd_write = "p";
	MCLib[p].cmd_read = "Zp";
	MCLib[p].type = 0;
	MCLib[p].min = 1;
	MCLib[p].max = 19;
	MCLib[p1].name = "positionmodedescription";
	MCLib[p1].nameDisplay = "position-mode-description";
	MCLib[p1].write = 1;
	MCLib[p1].cmd_write = "";
	MCLib[p1].cmd_read = "Zp";
	MCLib[p1].type = 3;
	MCLib[p1].min = 1;
	MCLib[p1].max = 19;
	MCLib[dollarsign].name = "status";
	MCLib[dollarsign].nameDisplay = "status-description";
	MCLib[dollarsign].write = 0;
	MCLib[dollarsign].cmd_write = "";
	MCLib[dollarsign].cmd_read = "$";
	MCLib[dollarsign].type = 2;
	MCLib[dollarsign].min = 0;
	MCLib[dollarsign].max = 0;

#ifdef WIP
	/* Get Filepath to command list table in MCLib.csv */
	getcwd(cwd, sizeof(cwd));
	printf("cwd = %s\n", cwd);
#if defined(_WIN32) || defined(_WIN64)
	printf("Pls use linux the next time, windows is also supported but I hardly recommend using linux. \nIf there's a file opening error you can configure do the following: \n1) open server_robolink_config.h \n2) comment out #define FILEPATH_DYNAMIC \n3) uncomment #define FILEPATH_ABSOLUT <your filepath> \n4) fill in your actual filepath to MCLib.csv \n Nonetheless: Good luck!\n");
	strcpy(MCLib_fp, cwd);
	tmp_ptr = strstr(MCLib_fp, "build");
	tmp_ptr[0] = '\0';
	strcat(MCLib_fp, "examples\\");
	strcat(MCLib_fp, MCLIB_FILENAME);
#else
#ifdef __linux
	printf("Yey, a linux user :3\n Have fun.\n");
	strcpy(MCLib_fp, cwd);
	tmp_ptr = strstr(MCLib_fp, "build");
	tmp_ptr[0] = '\0';
	strcat(MCLib_fp, "examples/");
	strcat(MCLib_fp, MCLIB_FILENAME);
#else
	printf("Mh, dunno which OS you're using, probably that'll lead to an error when opening the MCLib.csv file. \nI hardly recommend using linux. You can configure do the following: \n1) open server_robolink_config.h \n2) comment out #define FILEPATH_DYNAMIC \n3) uncomment #define FILEPATH_ABSOLUT <your filepath> \n4) fill in your actual filepath to MCLib.csv \n Nonetheless: Good luck!\n");
	strcpy(MCLib_fp, cwd);
	tmp_ptr = strstr(MCLib_fp, "build");
	tmp_ptr[0] = '\0';
	strcat(MCLib_fp, "examples/");
	strcat(MCLib_fp, MCLIB_FILENAME);
#endif
#endif

	/* Open MCLib.csv for reading */
	MCLibFile = fopen(MCLib_fp, "r");							/* open file to read */
	if (MCLibFile == NULL){
		fprintf(stderr, "\nError opening file MCLib.csv\n");
		exit(1);
	}

	/* TODO: Read correctly every field and assign MCLib[]-Arrayelements */
	/* Read MCLib.csv file content till end of file */
	fgets(linebuf, bufsize, MCLibFile);							/* skip header line */
	for (j=0; j<N_MCCOMMANDS; j++){
		printf("Functioning example assignment of MCLib[]:\nidx = %d, name = %s, displayname = %s, type = %d, write = %d\ncmd-write = %s, cmd-read = %s, min = %d, max = %d\n\n", j, MCLib[j].name, MCLib[j].nameDisplay, (int)(MCLib[j].type), MCLib[j].write, MCLib[j].cmd_write, MCLib[j].cmd_read, MCLib[j].min, MCLib[j].max);

		fgets(linebuf, bufsize, MCLibFile);						/* read a line */
		MCLib[j].name = strtok(linebuf, delims);
		MCLib[j].nameDisplay = strtok(NULL, delims);
		field = strtok(NULL, delims);
		MCLib[j].type = (char)atoi(field);
		field = strtok(NULL, delims);
		MCLib[j].write = atoi(field);
		MCLib[j].cmd_write = strtok(NULL, delims);
		if (strcmp(MCLib[j].cmd_write, "empty")) 				/* doesn't work */
			MCLib[j].cmd_write[0] = '\0';
		MCLib[j].cmd_read = strtok(NULL, delims);
		field = strtok(NULL, delims);
		MCLib[j].min = atoi(field);
		field = strtok(NULL, delims);
		MCLib[j].max = atoi(field);
		
		printf("WIP example assignment of MCLib[] from csv file:\nidx = %d, name = %s, displayname = %s, type = %d, write = %d\ncmd-write = %s, cmd-read = %s, min = %d, max = %d\n\n", j, MCLib[j].name, MCLib[j].nameDisplay, (int)(MCLib[j].type), MCLib[j].write, MCLib[j].cmd_write, MCLib[j].cmd_read, MCLib[j].min, MCLib[j].max);
	}
	fclose(MCLibFile);
#endif /* #ifdef WIP */

	/* ASSIGNMENTS */
	/* Assign pointer to first array element of struct array with motor controller commands to context of motor controller object instances */
	for (j=0; j<N_MCCOMMANDS; j++){
		MCIdx[j] = j;
		for (i=0; i<N_MOTORCONTROLLERS; i++){
				MCObj[i].MCLib[j] = MCLib[j];
		}
	}

	/* Assign ttynames for every object instance */
	for (i=0; i<N_MOTORCONTROLLERS; i=i+1){
		ttyname[i] = (char*)malloc(sizeof(char) * SIZE_TTYNAME);
		#ifdef DEFAULT_TTYNAME
			#if (N_PORTS == 1)									/* each motor controller connected to same port */
				strcpy(ttyname[i], "/dev/ttyUSB0");
			#endif
			#if (N_PORTS == N_MOTORCONTROLLERS) 				/* each motor controller connected to own port */
				sprintf(ttyname[i], "/dev/ttyUSB%d", i);
			#endif
			#if (N_PORTS != N_MOTORCONTROLLERS && N_PORTS != 1)
				printf("N_PORTS = %d\n", N_PORTS);
				printf("irregular number of motorcontrollers (N_MOTORCONTROLLERS) or ports (N_PORTS)\n");
			#endif
		#endif
		#ifndef DEFAULT_TTYNAME
				printf("Enter device file name (e.g. \"/dev/ttyUSB0\" for MotorController%d: ", i+1);
				fgets(ttyname[i] SIZE_TTYNAME, stdin);
				ttyname[i][strcspn(ttyname[i], "\n")] = 0; 	/* delete the "\n" from fgets command at the end of the string */
		#endif
	}

	/* Assign motor addresses and set file descriptors for every object instance */
	#ifndef DEFAULT_MOTORADDR
		printf("FYI: individual motor adresses for MotorController range from 1 to 254 \n");
		printf("FYI: broadcast motor adress is \"*\" \n");
	#endif
	for (i=0; i<N_MOTORCONTROLLERS; i=i+1){
		sprintf(MCObj[i].motorAddr, "%d", i+1);
		#ifndef DEFAULT_MOTORADDR
			printf("FYI: individual motor adresses for MotorController range from 1 to 254 \n");
			printf("FYI: broadcast motor adress is \"*\" \n");
			printf("Enter motor adress for MotorController%d: ", i+1);
			fgets(MCObj[i].motorAddr, 3, stdin);
			MCObj[i].motorAddr[strcspn(MCObj[i].motorAddr, "\n")] = 0; 	/* delete the "\n" from fgets command at the end of the string */
		#endif
		get_fd(ttyname[i], &(MCObj[i].fd));								/* get filedescriptors */
	}
	printf("\n");

	/* Assign NodeId's, browsenames and setup serial port connection settings */
	for (i=0; i<N_MOTORCONTROLLERS; i=i+1){
		/* Assign NodeId's for every object instance of a motor controller */
		nameId[i] = (char*)malloc(sizeof(char) * 10);
		if (nameId[i] == NULL)
			printf("\nNo free memory space for nameId[%d] available.\n", i);
		nameNr[i] = (char*)malloc(sizeof(char) * 10);
		if (nameNr[i] == NULL)
			printf("\nNo free memory space for nameNr[%d] available.\n", i);
		nameMSet[i] = (char*)malloc(sizeof(char) * 50);
		if (nameMSet[i] == NULL)
			printf("\nNo free memory space for NameId[%d] available.\n", i);
		strcpy(*(nameId+i), "MCId");
		strcpy(*(nameMSet+i), "MotorSettingsMC");
		sprintf(*(nameNr+i), "%d", i+1);
		strcat(*(nameId+i), *(nameNr+i));
		strcat(*(nameMSet+i), *(nameNr+i));

		nodeIdMC[i] = UA_NODEID_STRING(1,*(nameId+i));
		nodeIdMSet[i] = UA_NODEID_STRING(1, *(nameMSet+i));

		/* Assign browsenames for every object instance of a motor controller */
		nameBrowse[i] = (char*)malloc(sizeof(char) * 50);
		if (nameBrowse[i] == NULL)
			printf("\nNo free memory space for nameBrowse[%d] available.\n", i);
		strcpy(*(nameBrowse+i), "motorController");
		sprintf(*(nameNr+i), "%d", i+1);
		strcat(*(nameBrowse+i), *(nameNr+i));

		/* setup serial port connection settings for every file descriptor */
		set_interface_attribs(MCObj[i].fd, BAUDRATE);						/*baudrate 115200, 8 bits, no parity, 1 stop bit */

		/* user info */
		printf("Idx: %d; fd: %d on %s for nodeIdMC = \"%s\" \n with browsename \"%s\" and motor adress \"%s\" \n\n",i, MCObj[i].fd, ttyname[i], (char*)nodeIdMC[i].identifier.string.data, *(nameBrowse+i), MCObj[i].motorAddr);
	}

	/* Add Object Instances */
    defineObjectTypes(server);
#ifdef TYPECONSTRUCTOR
	addMotorControllerTypeConstructor(server);								/* need this? initializes lifecycle?! */
#endif
	for (i=0; i<N_MOTORCONTROLLERS; i=i+1){
		addMotorControllerObjectInstance(server, *(nameBrowse+i), *(nameNr+i), &nodeIdMC[i], &MCObj[i]);
		printf("Added motor controller %d\n",i+1);
		/* add callback methods to object instance */
		addStartMotorMethod(server, &nodeIdMC[i], &MCObj[i]);
		addStopMotorMethod(server, &nodeIdMC[i], &MCObj[i]);
		addSportSendMsgMethod(server, &nodeIdMC[i], &MCObj[i]);
		addReadSetMethod(server, &nodeIdMC[i], &MCObj[i]);
		printf("Added callback methods for motor controller %d\n",i+1);

		/* add datasources to motor controller object instance */
		addCurrentAngleDataSourceVariable(server, &nodeIdMC[i], &MCObj[i]);				/* hard coded data source */

		/* add motor settings object instance as child to motor controller object instance */
		addMotorSettingsObjectInstance(server, nameMSet[i], &nodeIdMSet[i], &nodeIdMC[i], &MCObj[i]);

		/* add datasources for every command in command list to motor controller object instance*/
		for(j = 0; j < N_MCCOMMANDS; j++){
			context[i][j].idx = &MCIdx[j];
			context[i][j].MCObj = &MCObj[i];
			addCurrentDataSourceVariable(server,  &nodeIdMC[i], &context[i][j]);
			printf("Added datasource variable %s for motor controller %d\n", context[i][j].MCObj->MCLib[j].nameDisplay, i+1);
		}
	}
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Object types defined and all object instances created\n");

	/* Run the server loop */
    UA_StatusCode status = UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
	return status == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
