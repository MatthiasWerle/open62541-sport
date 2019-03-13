/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Nanotec Motor controller configurations initial setup
 * -----------------------------------------------------
 *
 * This code is suitable for the following motor controller models
 * SMCI12, SMCI33, SMCI35, SMCI36, SMCI47-S, SMCP33, PD2-N, PD4-N and PD6-N
 *
 * The following code should be run in a command line terminal. The user will be given 
 * instructions and prompts for parameters to overwrite the default configurations of 
 * the motor controller.
 *
 * Warnings: Important notes for physical setup
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * 1) Don't connect any motor to the motor controller! Otherwise the 
 * default configurations for phase current can damage the motor! 
 * 2) Connect only a single motor controller to the server host. */

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
#include <time.h>				/* clock() function */
#include <unistd.h>
#include <pthread.h>			/* allows multi-threading */

#include "sport.h" 				/* serial port communication */

#define SIZE_TTYNAME 50
#define SIZE_MESSAGE 10
//#define DEFAULT_TTYNAME
#define READ_RESPONSE

/*************/
/* MAIN LOOP */
/*************/

int main(int argc, char** argv)
{

	char setup_ttyname[SIZE_TTYNAME];
	int setup_fd = -1;
	char setup_motorAddr[4];
	char setup_phaseCurrent[4];
	char setup_phaseCurrentHalt[4];
	char setup_msg[SIZE_MESSAGE];

	/* user prompt to define variables for serial port communication and motor adress */
#ifndef DEFAULT_TTYNAME
	printf("Find out device file name e.g. with terminal command 'dmesg | grep tty'\n");
	printf("Enter device file name (e.g. \"/dev/ttyUSB0\" for motor controller: ");
	fgets(setup_ttyname, SIZE_TTYNAME, stdin);
	setup_ttyname[strcspn(setup_ttyname, "\n")] = 0; /* delete the "\n" from fgets command at the end of the string */
#else
	strcpy(setup_ttyname, "/dev/ttyUSB0");
#endif


	/* setup tty interface and assign userdefined motor addr */ 
	get_fd(setup_ttyname, &setup_fd);
	if(setup_fd > 0){
		set_interface_attribs(setup_fd, B115200);		/*baudrate 115200, 8 bits, no parity, 1 stop bit */

		/* user info */
		printf("\nsetup_ttyname: %s\n", setup_ttyname);
		printf("derived setup_fd = %d\n", setup_fd);
		printf("Enter motor adress for assignment (1...254): ");
		fgets(setup_motorAddr, 4, stdin);
		setup_motorAddr[strcspn(setup_motorAddr, "\n")] = 0; /* delete the "\n" from fgets command at the end of the string */
		strcpy(setup_msg, "#*m");
		strcat(setup_msg, setup_motorAddr);
		strcat(setup_msg, "\r");
		printf("motor adress will be set to: %s\n\n", setup_motorAddr);

		/* setup motor adress */
		tcflush(setup_fd, TCIFLUSH); 										/* flush input buffer */
		sport_send_msg(setup_msg, setup_fd);								/* send message */
		sport_read_msg(setup_msg, setup_fd);								/* read response */

		/* get preconfigured phase current at halt */
		tcflush(setup_fd, TCIFLUSH); 										/* flush input buffer */
		sport_send_msg("#*Zi\r", setup_fd);
		sport_read_msg(setup_msg, setup_fd);

		/* user info and prompt to define phase current */
		setup_msg[strlen(setup_msg)-1] = '\0';
		printf("\nThe preconfigured phase current is set to %s %% of 7 A\n", setup_msg+4);
		printf("If you'd like to change the phase current, type in the share of 7 A in %% \n");
		printf("Warning: phase current has to be adjustet to the motor specifications! \nAlso: values out of range whill be ignored (0...150): ");
		fgets(setup_phaseCurrent, 4, stdin);
		setup_phaseCurrent[strcspn(setup_phaseCurrent, "\n")] = 0; 			/* delete the "\n" from fgets command at the end of the string */
		strcpy(setup_msg, "#*i");
		strcat(setup_msg, setup_phaseCurrent);
		strcat(setup_msg, "\r");
		printf("phase current will be set to: %s\n\n", setup_phaseCurrent);

		/* setup phase current */
		tcflush(setup_fd, TCIFLUSH); 										/* flush input buffer */
		sport_send_msg(setup_msg, setup_fd);								/* send message */
		sport_read_msg(setup_msg, setup_fd);								/* read response */

		/* get preconfigured phase current at halt */
		tcflush(setup_fd, TCIFLUSH); 										/* flush input buffer */
		sport_send_msg("#*Zr\r", setup_fd);
		sport_read_msg(setup_msg, setup_fd);

		/* user info and prompt to define phase current at halt */
		setup_msg[strlen(setup_msg)-1] = '\0';
		printf("\nThe preconfigured phase current at halt is set to %s %% of 7 A\n", setup_msg+4);
		printf("If you'd like to change the phase current at halt, type in the share of 7 A in %% \n");
		printf("Warning: phase current at halt has to be adjustet to the motor specifications! \nAlso: values out of range whill be ignored (0...150): ");
		fgets(setup_phaseCurrentHalt, 4, stdin);
		setup_phaseCurrentHalt[strcspn(setup_phaseCurrentHalt, "\n")] = 0; 			/* delete the "\n" from fgets command at the end of the string */
		strcpy(setup_msg, "#*r");
		strcat(setup_msg, setup_phaseCurrentHalt);
		strcat(setup_msg, "\r");
		printf("phase current will be set to: %s\n\n", setup_phaseCurrentHalt);

		/* setup phase current at halt */
		tcflush(setup_fd, TCIFLUSH); 										/* flush input buffer */
		sport_send_msg(setup_msg, setup_fd);								/* send message */
		sport_read_msg(setup_msg, setup_fd);								/* read response */

	}
	else
		printf("\nError: Bad filedescriptor! \nEither the ttyname doesn't exist or you have no permission to write onto the block device\n");
	
	return (int)1;
}