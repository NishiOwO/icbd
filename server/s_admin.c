/* Copyright 1991 by John Atwood deVries II. */
/* For copying and distribution information, see the file COPYING. */

/* primitives to deal with admin commands */

#include "config.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "server.h"
#include "externs.h"
#include "users.h"
#include "access.h"
#include "send.h"
#include "icbutil.h"
#include "strutil.h"
#include "mdb.h"

#include "murgil/murgil.h"  /* for disconnectuser()  */

int s_drop(int n, int argc)
{
	int TheVictim;

	if (argc == 2) {
		/* fields[1] can be a string, not just a single nick */
		TheVictim = find_user(getword(fields[1]));
		if (TheVictim < 0) {
			senderror(n, "User not found.");
		} else if (check_auth(n))
			disconnectuser(TheVictim);
		else if (valuser(u_tab[TheVictim].nickname,
				getword(get_tail(fields[1])), NULL) == 0) 
		{
			sprintf(mbuf,"You have been disconnected by %s",
				u_tab[n].nickname);
			sendstatus(TheVictim, "Drop", mbuf);
			sprintf (mbuf,"[DROP] %s (%d) dropped %s (%d)", 
				u_tab[n].nickname, n, 
				u_tab[TheVictim].nickname, TheVictim);
			mdb(mbuf);
			S_kill[TheVictim]++;
		}
		else
			senderror(n, "Authentication failure.");
	} else
		mdb("drop: wrong number of parz");
	return 0;
}

int s_shutdown(int n, int argc)
{
	int user;
	char line[255];

	/* before we even look at what they sent us, are the allowed? */
	if (!check_auth(n)) {
		senderror(n,"You are not authorized to do this.");
		return (-1);
	}
	if (argc == 2) {
		/* fields[1] is when and comments, separated by a blank */
/*		mdb(getword(fields[1])); */
/*		mdb(get_tail(fields[1]));  why we are shutting down*/
		/* BUG	we are presuming when is always now.
			...but then, I don't know how to schedule
			anything anyhow.
			we are presuming that no warning is given */
		/* icbexit(); */
		sprintf(line, "Server going down in %ld minute(s)!", 
			atol(fields[1]));
        	for (user=0; user < MAX_REAL_USERS; user++)
			if (u_tab[user].login > LOGIN_FALSE)
                        	sendimport(user,"Shutdown", line);
		TimeToDie = time(NULL) + 60.0 * atol(fields[1]);
		ShutdownNotify = 0;
		if (atol(fields[1]) < 300)
			ShutdownNotify++;
	} else {
		mdb("shutdown: wrong number of parz");
	}
	return 0;
}

int s_restart(int n, int argc)
{
	void icbdump();
	int user, ret;
	char *env[2];
	char tbuf[255];
	int quiet = 0;

	/* before we even look at what they sent us, are the allowed? */
	if (!check_auth(n)) {
		senderror(n,"You are not authorized to do this.");
		return (-1);
	}

	if (argc == 2)
		if ( !strcasecmp (fields[1], "quiet") )
			quiet = 1;

	if ( !quiet )
	{
		for (user=0; user < MAX_REAL_USERS; user++)
			if (u_tab[user].login > LOGIN_FALSE)
				sendimport(user,"Restart", 
					 "Server restarting...please be patient.");
	}
	else
	{
		sendimport (n, "Quiet-Restart",
			"Server restarting...please be patient.");
	}

	mdb("[RESTART] Server restarting");
	icbdump();
	icbcloselogs();

	/* determine if we need to add the -R flag or not */
	for ( ret =0; ret < restart_argc; ret++ )
	    if ( restart_argv[ret] != (char *)NULL
			&& ( !strcmp (restart_argv[ret], "-R") ||
				!strcmp (restart_argv[ret], "-Rq") )
			)
		{
			if ( quiet == 1 && !strcmp (restart_argv[ret], "-R") )
			{
				free (restart_argv[ret]);
				restart_argv[ret] = "-Rq";
			}
			else if ( quiet == 0 && !strcmp (restart_argv[ret], "-Rq") )
			{
				free (restart_argv[ret]);
				restart_argv[ret] = "-R";
			}

			break;
		}

	/* yup, it wasn't in there already */
	if ( ret == restart_argc )
	{
		if ( quiet == 0 )
			restart_argv[restart_argc-2] = "-R";
		else
			restart_argv[restart_argc-2] = "-Rq";
	    restart_argv[restart_argc-1] = NULL;
	}

	env[0] = "RESTART=Y";
	env[1] = NULL;
	ret = fcntl(6, F_GETFD, 0);

	if (fork()) exit(0);

	if (execve(restart_argv[0], restart_argv, env) < 0)
	{
		sprintf(tbuf, "[ERROR] RESTART failed: %s", 
			strerror(errno));

		mdb(tbuf);

		if ( !quiet )
		{
			for (user=0; user < MAX_REAL_USERS; user++)
				if (u_tab[user].login > LOGIN_FALSE)
					sendimport (user, "Restart", "Restart not done.");
		}
		else
			sendimport (n, "Quiet-Restart", "Restart not done.");

		mdb("[RESTART] Restart failed");
		return -1;
	}
	return 0;
}

int s_wall(int n, int argc)
{
	int user;

	/* before we even look at what they sent us, are the allowed? */
	if (!check_auth(n)) {
		senderror(n,"You are not authorized to do this.");
		return (-1);
	}
	if (argc == 2) {
/*		mdb(fields[1]);  text of message */
		/* send it only to the real users */
        	for (user=0; user < MAX_REAL_USERS; user++) {
			if (u_tab[user].login > LOGIN_FALSE) {
                        	sendimport(user,"WALL", fields[1]);
                	}
        	}
	} else {
		mdb("wall: wrong number of parz");
	}
	return 0;
}

