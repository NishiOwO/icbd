/* Copyright (c) 1991 by John Atwood deVries II. */
/* For copying and distribution information, see the file COPYING. */

#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

#include "version.h"
#include "server.h"
#include "externs.h"
#include "groups.h"
#include "protocol.h"
#include "mdb.h"
#include "icbutil.h"
#include "unix.h"
#include "users.h"
#include "access.h"
#include "send.h"

#include "murgil/murgil.h"

void trapsignals()
{
	/* cycle the logs on a restart signal */
	signal(SIGHUP, icbcyclelogs);

	/* exit on a hangup or terminate signal */
	signal(SIGTERM, icbexit);
	signal(SIGINT, icbexit);

	/* special internal signals to restart the server */
	signal(SIGUSR1, icbdump);
	signal(SIGUSR2, icbload);
}

int main(int argc, char* argv[])
{
	int c;
	extern char *optarg;
	int clearargsflg = 0;
	int forkflag = 0;
	int port = DEFAULTPORT;
	int i;
	long time();
	long TheTime;
	int pidf;
	char tbuf[255], *bindhost = (char *) NULL;
	struct rlimit rlp;
	int quiet_restart = 0;

	/* save off args that we need in /restart */
	restart_argc = argc+2;	/* room for the -R and NULL args */
	restart_argv = (char **) malloc (restart_argc * sizeof (char *));

	if ( restart_argv == (char **)NULL )
	{
	    perror ("malloc(restart_args)");
	    exit (1);
	}

	for ( i = 0; i <= argc; i++ )
	    restart_argv[i] = ((argv[i] == NULL) ? NULL : strdup(argv[i]));

	restart = 0;

	setbuf(stdout, (char *) 0);

	while ((c = getopt(argc, argv, "cp:fRqb:")) != EOF) {

		switch (c) {

		case 'b':
			bindhost = optarg;
			break;

		case 'R':
			restart++;
			break;

		case 'q':
			quiet_restart++;
			break;

		case 'f':
			forkflag++;
			break;

		case 'c':
			clearargsflg++;
			break;

		case 'p':
			port = atoi(optarg);
			break;

		case '?':
		default:
			puts("usage: icbd [-b host] [-p port] [-cRfq]");
			puts("-c	 wipe args from command line");
			puts("-R	 restart mode");
			puts("-q	 quiet mode (for restart)");
			puts("-f	 don't fork");
			puts("-p port	 bind to port instead of default");
			puts("-b host	 bind socket to \"host\"");
			exit(-1);
			break;
		}
	}

#ifdef RLIMIT_NOFILE
   getrlimit(RLIMIT_NOFILE, &rlp);
   rlp.rlim_cur = MAX_USERS + 1;
   if (setrlimit(RLIMIT_NOFILE, &rlp) < 0) {
	perror("setrlimit");
	exit(1);
	}
#endif

   if (restart == 0)
   {

	icbopenlogs();

	/* get our hostname */
	if ((gethostname(thishost,MAXHOSTNAMELEN)) < 0) {
		sprintf(tbuf, "[ERROR] gethostname failed: %s", 
			strerror(errno));
		mdb(tbuf);
		}

#if defined(SHORT_HOSTNAME) && defined(FQDN)
	if (strcmp(thishost, SHORT_HOSTNAME) == 0)
		strcpy(thishost, FQDN);
#endif	/* SHORT_HOSTNAME && FQDN */

	/*
	 * here's the scoop with this code:
	 *
	 * 1- don't want to remove rich's special case SHORT/FQDN code yet
	 * 2- if BIND_HOSTNAME is set in config.h, bind to only that ip
	 * 3- if -b arg is used, override with its value
	 */

#ifdef	BIND_HOSTNAME
	/* if bindhost is null (unset), let BIND_HOSTNAME set it */
	if ( (char *)NULL == bindhost )
	{
	    bindhost = BIND_HOSTNAME;
	    strcpy (thishost, BIND_HOSTNAME);
	}
#endif	/* BIND_HOSTNAME */

	if ( (char *) NULL != bindhost )
	{
	    /*
	     * if an actual host has been listed, reset thishost to it,
	     * otherwise, we are mapping to all interfaces and using the
	     * existing thishost value for the name
	     */
	    if ( '\0' != *bindhost )
	    {
		(void) strcpy (thishost, bindhost);
	    }
	}

	if (makenewport(bindhost, port) < 0) {
		sprintf(tbuf, "[ERROR] makenewport failed: %s", 
			strerror(errno));
		mdb(tbuf);
		exit (-1);
	}

	if (forkflag == 0)
		if (fork()) exit(0);

	if (clearargsflg)
		clearargs(argc, argv);

	/* initialize out tables  */
	init_groups();
	clear_users();

	for (i = 0; i < MAX_REAL_USERS; i++)
	    S_kill[i] = 0;

	/* invent our resident automagical user */
	TheTime = time(NULL);
	fill_user_entry(NICKSERV, "server",thishost,
		"Server", "", "ICB", "", TheTime, 0, 1, PERM_NULL);
	u_tab[NICKSERV].login = LOGIN_COMPLETE;
	u_tab[NICKSERV].t_on = TheTime;
	u_tab[NICKSERV].t_recv = TheTime;
	strcpy(u_tab[NICKSERV].realname, "registered");
	fill_group_entry(0, "ICB", "...here to serve you!", SUPERSECRET,
		RESTRICTED, NICKSERV, QUIET);
	nickwritetime(NICKSERV, 0, NULL);

	sprintf(mbuf, "[INFO] ICB revision %s on %s.", VERSION, thishost);
	mdb(mbuf);
	sprintf(mbuf, "[INFO] There are %d users possible.", MAX_USERS);
	mdb(mbuf);
	sprintf(mbuf, "[INFO] Of those, %d are real.", MAX_REAL_USERS);
	mdb(mbuf);
	
	trapsignals();
   }
   else
   {
	extern int port_fd;
	extern char *getlocalname();
	char	*th = (char *)NULL;

	icbopenlogs();
	/* get our hostname */
	if ((gethostname(thishost,MAXHOSTNAMELEN)) < 0) {
		sprintf(tbuf, "[ERROR] gethostname failed: %s", 
			strerror(errno));
		mdb(tbuf);
		}

#if defined(SHORT_HOSTNAME) && defined(FQDN)
	if (strcmp(thishost, SHORT_HOSTNAME) == 0)
		strcpy(thishost, FQDN);
#endif	/* SHORT_HOSTNAME && FQDN */

	init_groups();
	clear_users();
	trapsignals(); 
	mdb("[RESTART] Reloading server data");
	icbload();

	if ( (th = getlocalname (port_fd)) != (char *) NULL )
	{
	    memset(thishost, 0, sizeof(thishost));
	    strncpy(thishost, th, sizeof(thishost));
	}

		for (i = 0; i < MAX_USERS; i++)
			if (u_tab[i].login > LOGIN_FALSE)
			{
				if ( ! quiet_restart ||
					(strcasecmp (u_tab[i].nickname, "admin") == 0) )
				{
					sendimport (i,
						quiet_restart == 0 ? "Restart" : "Quiet-Restart",
						"Server is back up.");
				}
			}

    }

    if ((pidf = open(PID_FILE, O_WRONLY|O_CREAT|O_TRUNC)) >= 0)
    {
	    /* write the process id on line 1 */
	    sprintf(mbuf, "%d\n", (int)getpid());
	    write(pidf, mbuf, strlen(mbuf));
	    fchmod (pidf, 0644);
	    close(pidf);
    }

#ifdef RLIMIT_NOFILE
    getrlimit(RLIMIT_NOFILE, &rlp);
    sprintf(tbuf, "[INFO] Max fds set to %d", (int)(rlp.rlim_cur));
    mdb(tbuf);
#endif

    for (i = 0; i < MAX_REAL_USERS; i++)
    {
	lpriv_id[i] = -1;
	pong_req[i] = -1;
	timerclear (&ping_time[i]);
    }

    /* start the serve loop */
    serverserve();

    /* all's well that ends */
    return 0;
}

