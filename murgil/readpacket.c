/* Copyright (c) 1988 Carrick Sean Casey. All rights reserved. */


#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#include "config.h"
#include "cbuf.h"
#include "globals.h"
#include "murgil.h"

/* read a packet of data into an Ibuf */

/* return -2 if read failed because user disconnected (EOF) */
/* return -1 if read failed because of some error */
/* return  0 if packet is still incomplete */
/* return  1 if packet is complete */

int _readpacket(int user, struct Cbuf *p)
{
	register ssize_t ret;

	if (p->new) 
	{
		/* starting a new command */
		p->rptr = p->buf;
		/* read the length of the command packet */
		if ( (ret = read(user, p->rptr, 1)) < 0 )
		{
			/* we haven't gotten all the data they said they
			 * were gonna send (incomplete packet)
			 */
			if (errno == EWOULDBLOCK)
				return(0);
			else if ( errno == EPIPE || errno == ECONNRESET )
				return (-2);
			else
				return(-1);
		}

		if (!ret)
		{
			/* EOF (closed connection) */
			return(-2);
		}

		/* set our need-to-read size and scootch the pointer up 
		 * past the length byte 
		 */
		p->size = p->remain = *(p->rptr);
		p->rptr++;
		p->new = 0;
	}

	/* read as much of the command as we can get */
	if ( (ret = read(user, p->rptr, p->remain)) < 0) 
	{
		/* we haven't gotten all the data they said they
		 * were gonna send (incomplete packet)
		 */
		if (errno == EWOULDBLOCK)
			return(0);
		else if (errno == EPIPE || errno == ECONNRESET)
			return(-2);
		else
			return(-1);
	}

	if (!ret)
	{
		/* EOF (closed connection) */
		return(-2);
	}

	/* advance read pointer */
	p->rptr += ret;

	/* see if we read the whole thing */
	if ((p->remain -= ret) == 0) 
	{
		/* yes */

		/* nail down the end of the read string */
		*(p->rptr) = '\0';

		p->new = 1;
		return(1);
	} 
	else
	{
		/* command still incomplete */
		return(0);
	}
}
