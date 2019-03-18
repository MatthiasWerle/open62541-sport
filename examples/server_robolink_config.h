/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 * Peace, Love and Libertarian Anarchy, dear comrades! (A) */

/**
 * Header file for OPC UA server implementation for control of multiple Nanotec motor controllers
 * ----------------------------------------------------------------------------------------------
 *
 * This code is suitable for the following motor controller models
 * SMCI12, SMCI33, SMCI35, SMCI36, SMCI47-S, SMCP33, PD2-N, PD4-N and PD6-N
 *
 * Description: Code utilities
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * The following code will define macros which can be adjusted by users to configure server_robolink.c */

/**********/
/* MACROS */
/**********/

/* global parameters */
#define BAUDRATE B115200
#define READ_TIMEOUT_S 0			/* reading timeout in s */
#define READ_TIMEOUT_US 100000		/* reading timeout in us, should be greater than 100000 otherwise segmentation faults could happen with Read currently configured set callback method */
#define SIZE_TTYNAME 50
#define N_MOTORCONTROLLERS 3 		/* max. number of motor controllers */
#define N_PORTS 1					/* number of ports, either 1 or N_MOTORCONTROLLERS (1 if all motor controllers are connected to the same port or N if each controller is connected to a single port) */
#define N_MCCOMMANDS 5				/* WIP: number of currently defined motor controller commands */

/* options which can be commented out */
#define DEFAULT_TTYNAME
#define DEFAULT_MOTORADDR
#define READ_RESPONSE
#define NEW							/* for debugging to test new code snippets */