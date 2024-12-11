/* Copyright (c) 1988 by Carrick Sean Casey. All rights reserved. */



#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "globals.h"
#include "../server/ipcf.h"  /* for s_lost_user() */

/* shutdown a connection and take it out of the input loop */
void disconnectuser(int userfd)
{
	int i;

	close(userfd);
	FD_CLR(userfd,&fdset);
	if (userfd == highestfd) {
		for (i = FD_SETSIZE-1; i > 0 && !FD_ISSET(i, &fdset); i--);
		highestfd = i;
	}

	/* let main program know user went bye bye */
	s_lost_user(userfd);
}
