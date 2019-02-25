/**************************/
/* serial port connection */
/**************************/

/* set file descriptor for a serial port */
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

	cfsetospeed(&tty, (speed_t)speed);
	cfsetispeed(&tty, (speed_t)speed);
	/* set baud rates to 19200 */
//	cfsetospeed(&tty, B115200);
//	cfsetispeed(&tty, B115200);

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

	printf("tty c_cflag %d \n",tty.c_cflag);
    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;	/* enable timeout to receive first charakter of read if non-zero; read will block until VMIN bytes are received */
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
//				printf ("error %d from tggetattr \n", errno);
				printf("Error %d from tcgetattr: %s\n", errno, strerror(errno));
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 10;            // 1 second read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                printf ("error %d setting term attributes \n", errno);
}

/* copied in this form */
static int 
sport_send_msg(char* msg, int fd)
{
    int wlen = (int)write(fd, msg, strlen(msg)); /* number of bytes written */
    if (wlen != (int)strlen(msg)) {
        printf("Error %d from write: %s \n wlen= %d\n", errno, strerror(errno), wlen);
    }
    tcdrain(fd);    /* delay for output */
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "sport_send_msg was called");
    return wlen;
}

// TODO: static UA_StatusCode sport_read(UA_Int32 fd, UA_String *msg)

/* callable by server and client */
static UA_StatusCode 
sportSendMsgMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
						 size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output)
{
	/* initial declerations */
	globalstructMC *global = (globalstructMC*)objectContext;
	UA_String *inputStr = (UA_String*)input->data; 						/* message to be sent */
	/* create message */ 
	char msg[] = "#"; 													/* write start sign # to message */
	char motorAddr[3];
	sprintf(motorAddr,"%d",global->motorAddr);
	strcat(msg, motorAddr);												/* append motor adress to message */
	char* cmd = (char*)inputStr->data;
	strcat(msg, cmd);													/* append command to message */
	strcat(msg, "\r");													/* append end sign to message */
	/* send message */
	sport_send_msg(msg, global->fd);

	/* Todo: read on port to check if acknowledgement was sent back */
	
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Send Msg was called");
    return UA_STATUSCODE_GOOD;
}

static void
addSportSendMsgMethod(UA_Server *server, const UA_NodeId *objectNodeId, globalstructMC *global){
	UA_Argument inputArgument;
	UA_Argument_init(&inputArgument);
	inputArgument.description = UA_LOCALIZEDTEXT("en-US", "A string, command e.g. \"A\" or \"J1\" or \"J0\"");
	inputArgument.name = UA_STRING("InputCommand");
	inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
	inputArgument.valueRank = UA_VALUERANK_SCALAR; /* what is this? */
	
	UA_Argument outputArgument;
	UA_Argument_init(&outputArgument);
	outputArgument.description = UA_LOCALIZEDTEXT("en-US", "A string, message that was sent");
	outputArgument.name = UA_STRING("OutputMessage");
	outputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
	outputArgument.valueRank = UA_VALUERANK_SCALAR; /* what is this? */
	
	UA_MethodAttributes sendAttr = UA_MethodAttributes_default;
	sendAttr.description = UA_LOCALIZEDTEXT("en-US","Send a string via serial port");
	sendAttr.displayName = UA_LOCALIZEDTEXT("en-US","Send a message");
	sendAttr.executable = true;
	sendAttr.userExecutable = true;
	UA_Server_addMethodNode(server, UA_NODEID_NULL,	 
//							UA_NODEID_STRING(1, "SportSendMsg"),
//							UA_NODEID_STRING(1, "MCId1"),					/* motor controller 1 as parent */
//							UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), 	/* objectfolder as parent */
							*objectNodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "SportSendMsg"),
                            sendAttr, &sportSendMsgMethodCallback,
                            1, &inputArgument, 1, &outputArgument, (void*)global, NULL);
}

static void
MCcommand(const int motorAddr, const char* cmd, const int* valPtr, char* msg)
{
	memset(msg,0,strlen(msg));											/* empty message */
	char motorAddrStr[3];
	sprintf(motorAddrStr, "%d", motorAddr);
	char valStr[15];
	memset(valStr,0,strlen(valStr));											/* empty message */
	if (valPtr != NULL){
		printf("*valPtr = %d \n", *valPtr);
		printf("before sprintf valStr = %s\n", valStr);
		sprintf(valStr, "%d", *valPtr);
		printf("after sprintf valStr = %s\n", valStr);
	}
	printf("after sprintf \n");
	strcpy(msg,"#"); 													/* write start sign # to message */
	strcat(msg, motorAddrStr);											/* append motor adress to message */
	strcat(msg, cmd);													/* append command to message */
	strcat(msg, valStr);												/* append value to message */
	strcat(msg, "\r");													/* append end sign to message */
}


static UA_StatusCode 
startMotorMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
						 size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output)
{
	globalstructMC *global = (globalstructMC*)objectContext;
	
//	char* msg = NULL;
	char msg[] = "test";
	memset(msg,0,strlen(msg));											/* empty message */
	MCcommand(1, "A", NULL, msg);
	printf("fyi start motor msg = %s \n", msg);
	sport_send_msg(msg, global->fd);									/* send message */
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Send Msg was called");
    return UA_STATUSCODE_GOOD;
}

static void
addStartMotorMethod(UA_Server *server, const UA_NodeId *objectNodeId, globalstructMC *global){
	UA_MethodAttributes sendAttr = UA_MethodAttributes_default;
	sendAttr.description = UA_LOCALIZEDTEXT("en-US","Start Motor with current set");
	sendAttr.displayName = UA_LOCALIZEDTEXT("en-US","Start Motor");
	sendAttr.executable = true;
	sendAttr.userExecutable = true;
	UA_Server_addMethodNode(server, UA_NODEID_NULL,	 *objectNodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "SportSendMsg"),
                            sendAttr, &startMotorMethodCallback,
                            0, NULL, 0, NULL, (void*)global, NULL);
}





static char* sport_listen(char msg[], int fd)
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
			return strcat("Error from read", strerror(errno));
        } else {  /* rdlen == 0 */
            printf("Timeout from read\n");
			return "Timeout from read\n";
        }
        /* repeat read to get full message */
		return "0";
    } while (1);
}
