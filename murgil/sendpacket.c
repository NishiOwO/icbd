/* Copyright (c) 1988 Carrick Sean Casey. All rights reserved. */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "config.h"
#include "murgil.h"

/*
 * returns -2 on connection being gone
 * returns -1 on incomplete packet sent
 * returns 0 if nothing is written (would block)
 * returne 1 if write succeeded as expected
 *
/* send a packet along a socket file descriptor */
int _sendpacket(int s, char *pkt)
{
	int ret;
	int totlength;

	/* set up a sample packet for testing purposes */
	totlength = (unsigned char)*pkt + 1;

	if ((ret = send(s, pkt, totlength, 0)) < 0) {
		if (errno == EWOULDBLOCK)
			return(0);
		else if (errno == EPIPE || errno == ECONNRESET)
			return(-2);
		else {
			/* DEBUG temorary fix */
			return(-2);
		}
	}
	if (ret != totlength)
		return(-1);
	return(1);
}
