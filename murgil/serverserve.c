/* Copyright (c) 1988 Carrick Sean Casey. All rights reserved. */

/* largely rewritten, (c) 2000 Michel Hoche-Mong & jon r. luini. GPL license. */

/* main program loop */
/* active clients, new connections, and perform asynch dungeon action */


#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "config.h"
#include "globals.h"
#include "murgil.h"
#include "../server/strlist.h"
#include "../server/msgs.h" /* for ok2read() */
#include "../server/ipcf.h" /* for s_didpoll() */
#include "../server/mdb.h"  /* for mdb() */
#include "../server/send.h" /* for doSend() */

/* globals and file static vars */
extern int restart;

fd_set fdr, efdr;

void sdoinput(void)
{
	int n;
	register int x;
	char buf[1024];

	for (x = 1; x <= highestfd; x++)
	{
		if (FD_ISSET(x, &efdr))
			disconnectuser(x);
	}

	/* examine set of file descriptors */
	for (x = 1; x <= highestfd; x++)
	{
		if (FD_ISSET(x, &fdr)) 
		{
			if (x == port_fd) 
			{
				/* new connect on advertised socket */
				if ((n = _newconnect(x)) > 0)
					/* let server init the user */
					s_new_user(n);
			} 
			else 
			{
				/* filedescriptor x has client input */
				if ( ok2read (x) == 1 )
				{
				    switch(_readpacket(x, &cbufs[x]))
				    {
					    case  1:
						    /* complete packet */
						    s_packet(x, cbufs[x].buf);
						    break;
					    case  0:
						    /* incomplete packet */
						    break;
					    case -1:
						    /* error */
							{
								int saveerr = errno;
								vmdb ("fd#%d: uncaught error: %s", x,
									strerror (saveerr));
							}
						    break;
					    case -2:
						    /* close connection */
						    disconnectuser(x);
						    break;
				    }
				}
				else
				{
				    /* add to ignore and remove from fdset */
				    ignore (x);
				}
			}
		}
	}
}


/* serverserver()
 *
 * The main server loop. Polls the set of file descriptors looking for 
 * active sockets. Sockets with activity get handed off to sdoinput(),
 * which either calls a routine to handle the input, or disconnects 'em.
 */

void serverserve()
{
	register int i;
	int ret;
	long loopcount = 0; /* used to break loop for periodic s_didpoll() */
	struct timeval t;
	struct timeval *poll_timeout;
	char buf[1024];

	/* look at fd where new clients connect */
	if (restart == 0) 
	{
		FD_ZERO(&fdset);
		FD_ZERO(&ignorefds);
		FD_SET(port_fd, &fdset);
	}
	else 
	{
		for (i = 0; i < MAX_USERS; i++)
		    cbufs[i].new = 1;
	}

	if (port_fd > highestfd)
		highestfd = port_fd;


	for (;;) 
	{
		loopcount++;
		memcpy(&fdr, &fdset, sizeof(fdset));
		memcpy(&efdr, &fdset, sizeof(fdset));

		/* must re-set this each time through the loop because linux
		 * mangles the return time.
		 */
		if (POLL_TIMEOUT < 0) 
			poll_timeout = NULL;
		else
		{
			t.tv_sec = POLL_TIMEOUT;
			t.tv_usec = 0;
			poll_timeout = &t;
		}

		if ((ret = select(highestfd+1, &fdr, 0, &efdr, poll_timeout)) > 0) 
			sdoinput();

		if (ret == 0 || errno == EINTR || loopcount%10==0 )
		{
			/* time expired, we were signalled, or it's the 10th time */
			s_didpoll(0);

			/* for all users, check ignore/held state */
			for (i = 0; i < MAX_USERS; i++)
			{
				/* if it's currently being held */
				if (FD_ISSET(i, &ignorefds))
				{
					/* and it's ok to read from it again */
					if ( ok2read (i) == 1 )
						unignore(i);
				}
			}
		}
		else if (ret < 0)
		{
			/* oopsie! select() bombed out */
			vmdb("select: %s", strerror(errno));
		}
	}
}
