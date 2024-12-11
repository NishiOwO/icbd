/* Copyright (c) 1988 Carrick Sean Casey. All rights reserved. */


#include <stdio.h>

#include "config.h"
#include "murgil.h"

void packetsend(int s, char *pkt)
{
	switch(_sendpacket(s, pkt)) {
	case  1:
		/* ok */
		break;
	case  0:
		/* incomplete packet */
#ifdef NEVERDEFFED
		fprintf(stderr, "DEBUG: send blocked  - panic stop\n");
		kill(getpid(), 3); /* debug */
		sleep(20000);
		exit(1);
#endif
/* DEBUG */
		disconnectuser(s);
		break;
	case -1:
		/* error */
#ifdef NEVERDEFFED
		fprintf(stderr, "DEBUG: error in send - panic stop\n");
		kill(getpid(), 3); /* debug */
		sleep(20000);
		exit(1);
#endif
		/* DEBUG - this should log an error */
		disconnectuser(s);
		break;
	case -2:
		/* lost connection */
		disconnectuser(s);
		break;
	}
}
