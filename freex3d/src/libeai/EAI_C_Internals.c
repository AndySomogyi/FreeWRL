#include "EAI_C.h"

#define WAIT_FOR_RETVAL ((command!=SENDEVENT) && (command!=MIDICONTROL))
int _X3D_FreeWRL_FD;
int _X3D_FreeWRL_Swig_FD = 0;
int _X3D_FreeWRL_listen_FD = 0;
int isSwig = 0;
int _X3D_queryno = 1;

int receivedData= FALSE;
/* for waiting on a socket */
fd_set rfds;
struct timeval tv;
struct timeval tv2;

void X3D_error(char *msg) {
    perror(msg);
    exit(0);
}

double mytime;

char readbuffer[2048];
char *sendBuffer = NULL;
int sendBufferSize = 0;

/* handle a callback - this should get a line like:
 EV
1170697988.125835
31
0.877656
EV_EOT
*/

/* make a buffer large enough to hold our data */
void verifySendBufferSize (int len) {
	if (len < (sendBufferSize-50)) return;
	
	/* make it large enough to contain string, plus some more, as we usually throw some stuff on the beginning. */
	while (len>(sendBufferSize-200)) sendBufferSize+=1024;
	sendBuffer = realloc(sendBuffer,sendBufferSize);
}

/* count the number of numbers on a line - useful for MFNode return value mallocs */
int _X3D_countWords(char *ptr) {
	int ct;
	
	ct = 0;
	
	while (*ptr >= ' ') {
		SKIP_CONTROLCHARS
		SKIP_IF_GT_SPACE
		ct ++;
	}
	return ct;
}

void freewrlSwigThread(void) {
	const int on=1;
	int flags;
	int len;

	struct sockaddr_in servaddr, cliaddr;

	if ((_X3D_FreeWRL_listen_FD= socket(AF_INET, SOCK_STREAM, 0)) < 0) {
              X3D_error("ERROR opening swig socket");
              return;
        }
	
	setsockopt(_X3D_FreeWRL_listen_FD, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(EAIBASESOCKET+ 500);
	
	if (bind((_X3D_FreeWRL_listen_FD), (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		X3D_error("ERROR in bind");
	}

	if (listen(_X3D_FreeWRL_listen_FD, 1024) < 0) {
		X3D_error("ERROR in listen");
		return;
	}

	len = sizeof(cliaddr);

	_X3D_FreeWRL_Swig_FD = accept((_X3D_FreeWRL_listen_FD), (struct sockaddr*) &cliaddr, (socklen_t *) &len);

	if (_X3D_FreeWRL_Swig_FD)
		isSwig = 1;
}

/* read in the reply - if it is an RE; it is the reply to an event; if it is an
   EV it is an async event */

void freewrlReadThread(void)  {
	int retval;
	while (1==1) {


                tv.tv_sec = 0;
                tv.tv_usec = 100;
                FD_ZERO(&rfds);
                FD_SET(_X3D_FreeWRL_FD, &rfds);

                /* wait for the socket. We HAVE to select on "sock+1" - RTFM */
                retval = select(_X3D_FreeWRL_FD+1, &rfds, NULL, NULL, &tv);

                if (retval) {
			retval = read(_X3D_FreeWRL_FD,readbuffer,2048);
				if (retval  <= 0) {
					printf("ERROR reading fromsocket\n");
					exit(1);
				}
			readbuffer[retval] = '\0';

			/* if this is normal data - signal that it is received */
			if (strncmp ("RE",readbuffer,2) == 0) {
				receivedData = TRUE;
			} else if (strncmp ("EV",readbuffer,2) == 0) {
					_handleFreeWRLcallback(readbuffer);
			} else if (strncmp ("RW",readbuffer,2) == 0) {
				_handleReWireCallback(readbuffer);
			} else if (strncmp ("QUIT",readbuffer,4) == 0) {
				exit(0);
			} else {
				printf ("readThread - unknown prefix - %s\n",readbuffer);
			}

		/*
                } else {
                                printf ("waitForData, timing out\n","");
		*/

                }

	}
}

/* threading - we thread only to read from a different thread. This
allows events and so on to go quickly - no return value required. */

static char *sendToFreeWRL(char *callerbuffer, int size, int waitForResponse) {
	int retval;
	int readquery;
	char *ptr;

	#ifdef VERBOSE
	printf ("sendToFreeWRL - sending :%s:\n",callerbuffer);
	#endif

	retval = write(_X3D_FreeWRL_FD, callerbuffer, size);
	if (retval < 0) 
		 X3D_error("ERROR writing to socket");

	if (waitForResponse) {

		receivedData = FALSE;
		while (!receivedData) {
			sched_yield();
		}


		/* have the response here now. */

		#ifdef VERBOSE
		printf("Client got: %s\n",readbuffer);
		#endif
		
		/* should return something like: RE
1165347857.925786
1
0.000000
RE_EOT
*/
		/* see if it is a reply, or an event return */


		ptr = readbuffer;
		while ((*ptr != '\0') && (*ptr <= ' ')) ptr++;

		#ifdef VERBOSE
		printf ("found a reply\n");
		#endif

		SKIP_IF_GT_SPACE
		SKIP_CONTROLCHARS
		if (sscanf(ptr,"%lf",&mytime) != 1) {
			printf ("huh, expected the time, got %s\n",ptr);
			exit(1);
		}
		#ifdef VERBOSE
		printf ("time of command is %lf\n",mytime);
		#endif

		SKIP_IF_GT_SPACE
		SKIP_CONTROLCHARS

		#ifdef VERBOSE
		printf ("this should be the query number: %s\n",ptr);
		#endif

		if (sscanf(ptr,"%d",&readquery) != 1) {
			printf ("huh, expected the time, got %s\n",ptr);
			exit(1);
		}
		#ifdef VERBOSE
		printf ("query returned is %d\n",readquery);
		#endif

		if (_X3D_queryno != readquery) {
			printf ("server: warning, _X3D_queryno %d != received %d\n",_X3D_queryno,readquery);
			sched_yield();
			sleep(5);
			sched_yield();
		}

		SKIP_IF_GT_SPACE
		SKIP_CONTROLCHARS


		strncpy(callerbuffer,readbuffer,retval);

	}
	_X3D_queryno ++;
	return ptr;
}


void _X3D_sendEvent (char command, char *string) {
        char *myptr;
	verifySendBufferSize (strlen(string));
        sprintf (sendBuffer, "%d %c %s\n",_X3D_queryno,command,string);
        myptr = sendToFreeWRL(sendBuffer, strlen(sendBuffer),WAIT_FOR_RETVAL);
}

char *_X3D_makeShortCommand (char command) {
	char *myptr;

	verifySendBufferSize (100);
	sprintf (sendBuffer, "%d %c\n",_X3D_queryno,command);
	myptr = sendToFreeWRL(sendBuffer, strlen(sendBuffer),WAIT_FOR_RETVAL);
	#ifdef VERBOSE
	printf ("makeShortCommand, buffer now %s\n",myptr);
	#endif
	return myptr;
}

char *_X3D_make1VoidCommand (char command, uintptr_t *adr) {
	char *myptr;

	verifySendBufferSize (100);
	sprintf (sendBuffer, "%d %c %p\n",_X3D_queryno,command,adr);
	myptr = sendToFreeWRL(sendBuffer, strlen(sendBuffer),WAIT_FOR_RETVAL);
	#ifdef VERBOSE
	printf ("make1VoidCommand, buffer now %s\n",myptr);
	#endif
	return myptr;
}

char *_X3D_make1StringCommand (char command, char *name) {
	char *myptr;
	
	verifySendBufferSize (strlen(name));
	sprintf (sendBuffer, "%d %c %s\n",_X3D_queryno,command,name);
	myptr = sendToFreeWRL(sendBuffer, strlen(sendBuffer),WAIT_FOR_RETVAL);
	#ifdef VERBOSE
	printf ("make1StringCommand, buffer now %s\n",myptr);
	#endif
	return myptr;
}

char *_X3D_make2StringCommand (char command, char *str1, char *str2) {
	char *myptr;
	char sendBuffer[2048];
	
	verifySendBufferSize ( strlen(str1) + strlen(str2));
	sprintf (sendBuffer, "%d %c %s%s\n",_X3D_queryno,command,str1,str2);
	myptr = sendToFreeWRL(sendBuffer, strlen(sendBuffer),WAIT_FOR_RETVAL);

	#ifdef VERBOSE
	printf ("make2StringCommand, buffer now %s\n",myptr);
	#endif
	return myptr;
}


char *_X3D_Browser_SendEventType(uintptr_t *adr,char *name, char *evtype) {
	char *myptr;

	verifySendBufferSize (100);
	sprintf (sendBuffer, "%u %c 0 %d %s %s\n",_X3D_queryno, GETFIELDTYPE, (unsigned int) adr, name, evtype);

	myptr = sendToFreeWRL(sendBuffer, strlen(sendBuffer),TRUE);
	#ifdef VERBOSE
	printf ("_X3D_Browser_SendEventType, buffer now %s\n",myptr);
	#endif
	return myptr;
}

char * _RegisterListener (X3DEventOut *node, int adin) {
	char *myptr;
	

	verifySendBufferSize (100);
	#ifdef VERBOSE
	printf ("in RegisterListener, we have query %d advise index %d nodeptr %d offset %d datatype %d datasize %d field %s\n",
		_X3D_queryno,
                adin, node->nodeptr, node->offset, node->datatype, node->datasize, node->field);
	#endif

/*
 EAIoutSender.send ("" + queryno + "G " + nodeptr + " " + offset + " " + datatype +
                " " + datasize + "\n");
*/
	sprintf (sendBuffer, "%u %c %ld %d %c %d\n",
		_X3D_queryno, 
		REGLISTENER, 
		node->nodeptr,
		node->offset,
		mapFieldTypeToEAItype(node->datatype),
		node->datasize);

	myptr = sendToFreeWRL(sendBuffer, strlen(sendBuffer),TRUE);
	#ifdef VERBOSE
	printf ("_X3D_Browser_SendEventType, buffer now %s\n",myptr);
	#endif
	return myptr;
}
