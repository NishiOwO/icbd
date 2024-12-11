/* Copyright (c) 1988 Carrick Sean Casey. All rights reserved. */

#ifndef _MURGIL_GLOBALS_H_
#define _MURGIL_GLOBALS_H_

#include "../config.h"

#include <sys/types.h>
#include <sys/time.h> /* for struct timeval and struct itimerval */

#include "cbuf.h"


extern struct Cbuf cbufs[MAX_USERS];	/* user packet buffers (YECCH!) */
extern fd_set fdset;			/* player fd set for select() */
extern fd_set ignorefds;		/* fds that are not to be polled */
extern int port_fd;
extern int highestfd;
/* timeout value in usec for select() */
extern struct timeval *polltimeout;
extern struct itimerval *polldelay;


#endif /* #define _MURGIL_GLOBALS_H_ */
