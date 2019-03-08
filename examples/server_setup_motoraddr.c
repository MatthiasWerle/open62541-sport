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
int main(int argc, char** argv)
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler); /* catch ctrl-c */

    /* Create a server listening on port 4840 */
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

	char setup_ttyname[SIZE_TTYNAME];
	int setup_fd = -1;
	char setup_motorAddr[4];
	char setup_msg[SIZE_MESSAGE];

	/* user prompt to define variables */
#ifndef DEFAULT_TTYNAME
	printf("Find out device file name e.g. with terminal command 'dmesg | grep tty'\n");
	printf("Enter device file name (e.g. \"/dev/ttyUSB0\" for motor controller: ");
	fgets(setup_ttyname, 50, stdin);
	setup_ttyname[strcspn(setup_ttyname, "\n")] = 0; /* delete the "\n" from fgets command at the end of the string */
#else
	strcpy(setup_ttyname, "/dev/ttyUSB0");
#endif
	printf("Enter motor adress for assignment (1...254): ");
	fgets(setup_motorAddr, 4, stdin);
	setup_motorAddr[strcspn(setup_motorAddr, "\n")] = 0; /* delete the "\n" from fgets command at the end of the string */
	strcpy(setup_msg, "#*m");
	strcat(setup_msg, setup_motorAddr);
	strcat(setup_msg, "\r");

	/* setup tty interface and assign userdefined motor addr */ 
	get_fd(setup_ttyname, &setup_fd);
	if(setup_fd > 0){
		set_interface_attribs(setup_fd, B115200);		/*baudrate 115200, 8 bits, no parity, 1 stop bit */

		/* user info */
		printf("setup_ttyname: %s\n", setup_ttyname);
		printf("derived setup_fd = %d\n", setup_fd);
		printf("motor adress will be set to: %s\n", setup_motorAddr);

		tcflush(setup_fd, TCIFLUSH); 										/* flush input buffer */
		sport_send_msg(setup_msg, setup_fd);								/* send message */
		sport_read_msg(setup_msg, setup_fd);								/* read response */
	}
	else
		printf("\nError: Bad filedescriptor! \nEither the ttyname doesn't exist or you have no permission to write onto the block device\n");
	/* Run the server loop */
    UA_StatusCode status = UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
	return (int)status;
}
