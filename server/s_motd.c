/* Copyright 1991 by John Atwood deVries II. */
/* For copying and distribution information, see the file COPYING. */

/* primitive to print the motd */

#include "config.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_NDBM_H
#include <ndbm.h>
#elif defined (HAVE_GDBM_NDBM_H)
#include <gdbm/ndbm.h>
#elif defined (HAVE_DB1_NDBM_H)
#include <db1/ndbm.h>
#endif
#include <fcntl.h>
#include <errno.h>

#include "server.h"
#include "externs.h"
#include "mdb.h"
#include "send.h"


int s_motd(int n, int argc)
{
    int motd_fd;
    char c;
    DBM *db;
    char key[64];
    datum info, request;
    char temp[256];
    int i;

    motd_fd = open(ICBDMOTD, O_RDONLY);

    memset(temp, 0, 255);
    /* if the file is there, list it, otherwise report error */
    if (motd_fd >= 0) 
    {
	while ((i = read(motd_fd, &c, 1)) > 0) 
        {
	    if (c == '%') 
		{
		    i = read(motd_fd, &c, 1);
		    if (i <= 0) {
		       if (close(motd_fd) != 0) {
			    sprintf(mbuf, "[ERROR] MOTD File Close: %s",
				    strerror(errno));
			    mdb(mbuf);
			    }
		    }
		    else if (c == 'U') {
		       if ((db = dbm_open(USERDB, O_RDONLY, 0)) == NULL) {
			    sprintf(mbuf, "[ERROR] User DB Open: %s", 
				    strerror(errno));
			    mdb(mbuf);
			    if (close(motd_fd) != 0) {
				    sprintf(mbuf, 
					    "[ERROR] MOTD File Close: %s",
					    strerror(errno));
				    mdb(mbuf);
				    return -1;
				    }
			}
			else
			{
			    strcpy (key, "server.signon");
			    request.dptr = key;
			    request.dsize = strlen(key);
			    info = dbm_fetch(db, request);
			    if (info.dptr)
				    strncat(temp, info.dptr, info.dsize);
			    dbm_close(db);
			}
		    }
		    else
			    strncat(temp, &c, 1);
		    }
	    else if (c == '\012') {
		    sends_cmdout(n, temp);
		    memset(temp, 0, 255);
		    }
	    else strncat(temp, &c, 1);
	}
	if (close(motd_fd) != 0) {
		sprintf(mbuf, "[ERROR] MOTD File Close: %s",
			strerror(errno));
		mdb(mbuf);
		}
    } else {
	sprintf(mbuf, "[ERROR] MOTD File Open: %s", 
		strerror(errno));
	mdb(mbuf);
	senderror(n, "No MOTD file found.");
    }

    return 0;
}
