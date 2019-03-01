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

	/* set flags to customize tty serial communication options */
    tty.c_cflag |= (unsigned int)(CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~(unsigned int)CSIZE;
    tty.c_cflag |= (unsigned int)CS8;         /* 8-bit characters */
    tty.c_cflag &= ~(unsigned int)PARENB;     /* no parity bit */
    tty.c_cflag &= ~(unsigned int)CSTOPB;     /* only need 1 stop bit */
    // tty.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */

	/* setup for non-canonical mode */
    tty.c_iflag &= ~(unsigned int)(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(unsigned int)(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~(unsigned int)OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;	/* enable timeout to receive first character of read if non-zero; read will block until VMIN bytes are received */
    tty.c_cc[VTIME] = 1; /* timeout time in tenth of a second */

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static void
set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
				printf("Error %d from tcgetattr: %s\n", errno, strerror(errno));
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 50;            // timeout in tenth of a second

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                printf ("error %d setting term attributes \n", errno);
}

/********************************************/
/* RECEIVE AND SEND MESSAGES ON SERIAL PORT */
/*******************************************/

static int 
sport_send_msg(char* msg, int fd)
{
	/* write message */
    int wlen = (int)write(fd, msg, strlen(msg)); /* number of bytes written */
	printf("\nsent msg = %s \n", msg);
    if (wlen != (int)strlen(msg)) {
        printf("Error %d from write: %s \n wlen= %d\n", errno, strerror(errno), wlen);
    }
    tcdrain(fd);    /* delay for output */

#ifdef READ_RESPONSE
	/* read answer */
	int rdlen = 0; 
	char buf[80];
	rdlen = (int)read(fd, buf, sizeof(buf));
	printf("received msg: rdlen = %d \n",rdlen);
	printf("received msg: buf = %s \n\n", buf);
	strcpy(buf, "");
#endif
    return (int)wlen;
}
