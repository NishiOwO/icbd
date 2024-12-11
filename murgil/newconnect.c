/* Copyright (c) 1988 Carrick Sean Casey. All rights reserved. */


#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <errno.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "config.h"
#include "globals.h"

/* accept and initialize a new client connection */

/* returns: */
/* the resultant socket file descriptor */
/*  0 if operation would block */
/* -1 if operation failed */
int _newconnect(int s)
{
	int ns;
	int one = 1;
	int flags;
	struct linger nolinger;

	/* accept the connection */
	if ((ns = accept(s, (struct sockaddr *) NULL, NULL)) < 0) {
		if (errno == EWOULDBLOCK)
			return(0);
		else {
			perror("new_client:accept");
			return(-1);
		}
	}

	/* force occasional connection check */
	if (setsockopt(ns, SOL_SOCKET, SO_KEEPALIVE,
	  (char *)&one, sizeof(one)) < 0) {
		perror("newclient:setsockopt (keepalive)");
	}

	/* don't allow broken connections to linger */
	/* nolinger.l_onoff = 0; */
	nolinger.l_onoff = 1;
	nolinger.l_linger = 0;
	if (setsockopt(ns, SOL_SOCKET, SO_LINGER,
	  (char *)&nolinger, sizeof(nolinger)) < 0) {
		perror("newclient:setsockopt (dontlinger)");
	}

        one = 24576;
        if (setsockopt(s, SOL_SOCKET, SO_SNDBUF,
           (char *)&one, sizeof(one)) < 0) {
                perror("SO_SNDBUF");
                }

	/* make the socket non-blocking */
	if (fcntl(ns, F_SETFL, FNDELAY) < 0) {
		perror("new_client:fcntl");
		return(-1);
	}

        /* Don't close on exec */
        flags = fcntl(ns, F_GETFD, 0);
        flags = flags & ~ FD_CLOEXEC;
        fcntl(ns, F_SETFD, flags);

	/* enter the user's fd into the active set */
	FD_SET(ns, &fdset);

	if (ns > highestfd)
		highestfd = ns;

	/* first command is a "new" command */
	cbufs[ns].new = 1;

	return(ns);
}
