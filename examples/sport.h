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
        tty.c_cc[VTIME] = 10;            // 1 second read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                printf ("error %d setting term attributes \n", errno);
}

/********************************************/
/* RECEIVE AND SEND MESSAGES ON SERIAL PORT */
/*******************************************/

static int 
sport_send_msg(char* msg, int fd)
{
    int wlen = (int)write(fd, msg, strlen(msg)); /* number of bytes written */
    if (wlen != (int)strlen(msg)) {
        printf("Error %d from write: %s \n wlen= %d\n", errno, strerror(errno), wlen);
    }
    tcdrain(fd);    /* delay for output */
	
	/*debug*/
//	delay(1000);
	int rdlen = 0; 
	char buf[80];
	rdlen = (int)read(fd, buf, sizeof(buf));
	printf("\nsent msg = %s \n", msg);
	printf("received msg: rdlen = %d \n",rdlen);
	printf("received msg: buf = %s \n\n", buf);
	strcpy(buf, "");
	
    return (int)wlen;
}

static int
sport_listen(char* msg, int fd)
{
	char msg_in[strlen(msg)+50];
    do {
        int rdlen = 0;								/* number of bytes read */
		char buf[80];
		printf("fyi sizeof(buf): %d \n", sizeof(buf));
//		fcntl(fd, F_SETFL, O_NDELAY); /* causes read function to return 0 if no characters available on port */
        rdlen = (int)read(fd, buf, sizeof(buf));
		//		fcntl(fd, F_SETFL, 0); /* causes read function to restore normal blocking behaviour */
		printf("fyi rdlen: %i \n", rdlen);
		printf("fyi buf: ");
		int i;
		for(i = 1 ; i == 80; i = i + 1) 
		{
			printf("%c", (char)buf[i]); 
		}
		printf("\n ... \n");
		strncat(msg_in, buf, strlen(msg)+50); /* appends part of read input message from buffer to msg_in */
		printf("fyi msg_in = %s\n",msg_in);
        if (rdlen > 0) {
            buf[rdlen] = 0;
            printf("Read %d: \"%s\"\n", rdlen, buf);
//			/* display hex */
//			unsigned char   *p;
//			printf("Read %d:", rdlen);
//			for (p = buf; rdlen-- > 0; p++)
//				printf(" 0x%x", *p);
//			printf("\n");
        } else if (rdlen < 0) {
            printf("Error from read: %d: %s\n", rdlen, strerror(errno));
			return -1;
        } else {  /* rdlen == 0 */
            printf("Timeout from read\n");
			return -2;
        }
        /* repeat read to get full message */
		return 0;
    } while (1);
}
