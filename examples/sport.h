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

#if !defined(READ_TIMEOUT_S) 
	#define READ_TIMEOUT_S 0
#endif
#if !defined(READ_TIMEOUT_MS)
	#define READ_TIMEOUT_MS 10000 																		
#endif
#if !defined(WRITE_TIMEOUT_S) 
	#define WRITE_TIMEOUT_S 0
#endif
#if !defined(WRITE_TIMEOUT_MS)
	#define WRITE_TIMEOUT_MS 100																		
#endif

 /********************************/
/* SETUP SERIAL PORT CONNECTION */
/********************************/

static UA_INLINE UA_StatusCode 
get_fd(char *ttyname, int *fd)
{
    *fd = open(ttyname, O_RDWR | O_NOCTTY | O_SYNC); 	/* read and write, no controlling terminal of process,  */
    if (*fd < 0){
        printf("Error opening %s: %s\n", ttyname, strerror(errno));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode 
set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
		printf("Error %d from tcgetattr: %s\n", errno, strerror(errno));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

	/* set baudrate */
	cfsetospeed(&tty, (speed_t)speed);					/* baudrate output e.g. speed = B115200 */
	cfsetispeed(&tty, (speed_t)speed);					/* baudrate input e.g. speed = B115200 */

	/* control modes: set flags to customize tty serial communication options */
	tty.c_cflag |= (unsigned int)(CLOCAL | CREAD); 		/* ignore modem control lines; enable receiver */
	tty.c_cflag |= (unsigned int)CSIZE;					/* character size mask */
	tty.c_cflag |= (unsigned int)CS8;         			/* 8-bit characters */
	tty.c_cflag &= ~(unsigned int)PARENB;				/* no parity bit */
	tty.c_cflag &= ~(unsigned int)CSTOPB;				/* only need 1 stop bit */
	// tty.c_cflag &= ~CRTSCTS; 						/* no hardware flowcontrol */

	/* setup for canonical mode where the buffer is sent after a carriage return or line feed was received */
	/* input modes */
	tty.c_iflag |= (unsigned int)(IGNBRK | IGNPAR | ISTRIP | ICRNL); 										/* ignore BREAK condition; ignore framing errors and parity errors; strip off 8 bytes; map CR to NL */
	tty.c_iflag &= ~(unsigned int)(BRKINT | IGNCR | PARMRK | IGNCR | INPCK | INLCR | IXON | IXOFF); 		/* don't ignore carriage return on input */
	/* output modes */
	tty.c_oflag &= ~(unsigned int)(OPOST | ONLCR | OCRNL | ONOCR | ONLRET | OFILL); 						/* Disable implementation-defined output processing; don't map NL to CR-NL; don't map CR to NL; Do output CR at column 0; Do output CR; don't send fill characters for a delay, rather than using a timed delay */
	/* local modes */
	tty.c_lflag |= (unsigned int)(ICANON); 																	/* enable canonical mode */

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error %d from tcsetattr: %s\n", errno, strerror(errno));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return UA_STATUSCODE_GOOD;
}

/********************************/
/* SEND MESSAGES ON SERIAL PORT */
/********************************/

/* TODO: Implement select() to check if message can be sent already similar to sport_read_msg() use of select() */
static UA_INLINE UA_StatusCode 
sport_send_msg(char* msg, int fd)
{
	if (fd != -1){
		/* check buffer */
		int ready;
		struct timeval tv = {WRITE_TIMEOUT_S, WRITE_TIMEOUT_MS};												/* Timeout in Sekunden, in micro-Sekunden */
		fd_set writefds;
		FD_ZERO(&writefds);
		FD_SET(fd, &writefds);
		ready = select(fd+1, NULL, &writefds, NULL, &tv);
		
		if(ready > 0){
			/* write message */
			int wlen = (int)write(fd, msg, strlen(msg)); 														/* number of bytes written */
			/* user info */
			if (wlen >= 0) {
				char msg_cut[strlen(msg)+1];
				strcpy(msg_cut, msg);
				msg_cut[strlen(msg)-1] = '\0';
				printf("sent %d bytes on fd %d as msg = \"%s\\r\" \n", (int)strlen(msg), fd, msg_cut);
			}
			else{
				printf("Error %d from write: %s \n wlen= %d\n", errno, strerror(errno), wlen);
				return UA_STATUSCODE_BADUNEXPECTEDERROR;
			}
			tcdrain(fd);    																					/* delay for output */
			return UA_STATUSCODE_GOOD;
		}
		if(ready == 0){
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Warning: Timeout, can't write into buffer");
			return UA_STATUSCODE_BADUNEXPECTEDERROR;
		}
		else { /* eg. ready == -1 */
			printf("Error %d from select: %s \n", errno, strerror(errno));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Warning: Error from select()");
			return UA_STATUSCODE_BADUNEXPECTEDERROR;
		}
	}
	else{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error: bad filedescriptor, couldn't write buffer");
		return UA_STATUSCODE_BADUNEXPECTEDERROR;
	}
}

/**********************************/
/* RECEIVE MESSAGE ON SERIAL PORT */
/**********************************/

static UA_INLINE UA_StatusCode
sport_read_msg(char* msg, int fd)
{
	memset(msg, '\0', strlen(msg));
	if (fd != -1){
		/* check buffer */
		int ready;
		struct timeval tv = {READ_TIMEOUT_S, READ_TIMEOUT_MS};												/* Timeout in Sekunden, in micro-Sekunden */
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);
		ready = select(fd+1, &readfds, NULL, NULL, &tv);
		if(ready > 0){
			/* read answer */
			char buf[80];
			memset(buf, '\0', sizeof(buf));
			int rdlen = (int)read(fd, buf, sizeof(buf));													/* number of bytes read */
			/* user info */
			char msg_cut[strlen(buf)];
			strcpy(msg, buf);
			strcpy(msg_cut, msg);
			msg_cut[strlen(msg)-1] = '\0';																	/* cut out the LFCR which was converted from the received CR due to tty settings at the end of the message */
			printf("received msg: %d bytes on fd %d in buf = \"%s\\r\" \n", rdlen, fd, msg_cut); 			/* show the message in the original form with the CR */
			return UA_STATUSCODE_GOOD;
		}
		if(ready == 0){
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Warning: Timeout, nothing to read in buffer");
			return UA_STATUSCODE_BADUNEXPECTEDERROR;
		}
		else { /* eg. ready == -1 */
			printf("Error %d from select: %s \n", errno, strerror(errno));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Warning: Error from select()");
			return UA_STATUSCODE_BADUNEXPECTEDERROR;
		}
	}
	else{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error: bad filedescriptor, couldn't read buffer");
		return UA_STATUSCODE_BADUNEXPECTEDERROR;
	}
}
