/* Copyright 1991 by John Atwood deVries II. */
/* For copying and distribution information, see the file COPYING. */

/* a little debugging routine */

#include "config.h"

#include <unistd.h>
#include <time.h>

#include "server.h"
#include "externs.h"
#include "mdb.h"
#include "unix.h"
#ifdef HAVE_STDARG_H
#include "stdarg.h"
#endif

/* XXX NEED TO FIX
 * POSSIBLE BUFFER OVERRUN IF message IS TOO LONG - hoche 5/10/00  
 */ 


void mdb(char *message)
{
#ifdef DEBUG
	char timebuf[255];
	char tbuf[BUFSIZ];
	
	gettime();
	strftime(timebuf, 255, "%b %d %Y %H:%M:%S", localtime(&curtime));
        sprintf(tbuf, "%s %s: %s \n", timebuf, thishost, message);
	write(icbd_log, tbuf, strlen(tbuf));
#endif
}



void vmdb(char *fmt, ...)
{
#ifdef DEBUG
#ifdef HAVE_STDARG_H
	va_list ap;
	char tbuf[BUFSIZ];
	
	va_start(ap, fmt);
	vsprintf(tbuf, fmt, ap);
	va_end(ap);

	mdb(tbuf);

#endif /* HAVE_STDARG_H */
#endif /* DEBUG */
}
