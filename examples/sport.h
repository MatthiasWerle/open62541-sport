/********************************/
/* SETUP SERIAL PORT CONNECTION */
/********************************/

static int 
set_fd(char *ttyname)
{
    int fd = open(ttyname, O_RDWR | O_NOCTTY | O_SYNC); /* read and write, no controlling terminal of process,  */
    if (fd < 0){
        printf("Error opening %s: %s\n", ttyname, strerror(errno));
        return -1;
    }
    return fd;
}

static UA_INLINE UA_StatusCode 
set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
//		printf("Error from tcgetattr: %s\n", strerror(errno));
		printf("Error %d from tcgetattr: %s\n", errno, strerror(errno));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

	/* set baudrate */
	cfsetospeed(&tty, (speed_t)speed);	/* e.g. speed = B115200 */
	cfsetispeed(&tty, (speed_t)speed);

	/* control modes: set flags to customize tty serial communication options */
    tty.c_cflag |= (unsigned int)(CLOCAL | CREAD); 		/* ignore modem control lines; enable receiver */
    tty.c_cflag |= (unsigned int)CSIZE;					/* character size mask */
    tty.c_cflag |= (unsigned int)CS8;         			/* 8-bit characters */
    tty.c_cflag &= ~(unsigned int)PARENB;				/* no parity bit */
    tty.c_cflag &= ~(unsigned int)CSTOPB;				/* only need 1 stop bit */
    // tty.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */

	/* setup for canonical mode where the buffer is sent after a carriage return or line feed was received */
	/* input modes */
	tty.c_iflag |= (unsigned int)(IGNBRK | IGNPAR | ISTRIP | ICRNL); /* ignore BREAK condition; ignore framing errors and parity errors; strip off 8 bytes; map CR to NL */
	tty.c_iflag &= ~(unsigned int)(BRKINT | IGNCR | PARMRK | IGNCR | INPCK | INLCR | IXON | IXOFF); /* don#t ignore carriage return on input */
	/* output modes */
	tty.c_oflag &= ~(unsigned int)(OPOST | ONLCR | OCRNL | ONOCR | ONLRET | OFILL); /* Disable implementation-defined output processing; don't map NL to CR-NL; don't map CR to NL; Do output CR at column 0; Do output CR; don't send fill characters for a delay, rather than using a timed delay */
	/* local modes */
	tty.c_lflag |= (unsigned int)(ICANON); /* enable canonical mode */

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return UA_STATUSCODE_GOOD;
}

/********************************************/
/* RECEIVE AND SEND MESSAGES ON SERIAL PORT */
/*******************************************/

static int 
sport_send_msg(char* msg, int fd)
{
	/* write message */
    int wlen = (int)write(fd, msg, strlen(msg)); /* number of bytes written */
	printf("\n sent msg = %s \n", msg);
    if (wlen != (int)strlen(msg)) {
        printf("Error %d from write: %s \n wlen= %d\n", errno, strerror(errno), wlen);
    }
    tcdrain(fd);    /* delay for output */
    return (int)wlen;
}
