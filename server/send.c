/* Copyright (c) 1991 by John Atwood deVries II. */
/* For copying and distribution information, see the file COPYING. */

/* send various messages to the client */

#include "config.h"

#include <string.h>
#include <errno.h>

#include "version.h"
#include "server.h"
#include "externs.h"
#include "protocol.h"
#include "strutil.h"
#include "msgs.h"
#include "mdb.h"
#include "send.h"

#include "murgil/murgil.h" /* for _sendpacket() */

extern long time();
extern char * getremotename();


/* send an error message to the client */
void senderror(int to, char *error_string)
{
	filtertext(error_string);
        sprintf(pbuf, "%c%s", ICB_M_ERROR, error_string);
	doSend(-1, to);
}

/* send personal message to a client */
void sendperson(int from, int to, char *message_string)
{
	char one[255], two[MAX_NICKLEN+1];

	filtertext(message_string);
        sprintf(one, "%s@%s", u_tab[from].loginid, u_tab[from].nodeid);
        ucaseit(one);
        strncpy(two, u_tab[from].nickname, MAX_NICKLEN);
        ucaseit(two);

	if ((!nlmatch(one, *u_tab[to].pri_s_hushed)) &&
           (!nlmatch(two, *u_tab[to].pri_n_hushed))) {
        	sprintf(pbuf, "%c%s\001%s", ICB_M_PERSONAL,
		    u_tab[from].nickname, message_string);
		lpriv_id[to] = from;
		doSend(from, to);
	    }
	else
		sendstatus(from, "Bounce", "Message did not go through");
}

/* send normal open group message to the client */
void sendopen(int from, int to, char *txt)
{
	filtertext(txt);
	/* construct open message */
	sprintf(pbuf,"%c%s\001%s", ICB_M_OPEN, u_tab[from].nickname, txt);
	doSend(from, to);
}

/* send an exit message to the client -- makes the client disconnect */
void sendexit(int to)
{
	/* construct exit message */
	sprintf(pbuf, "%c",ICB_M_EXIT);
	doSend(-1, to);
}

/* send a ping */
void sendping(int to, char *who)
{
	/* construct ping message */
	sprintf(pbuf, "%c%s",ICB_M_PING, who);
	doSend(-1, to);
}



/* send a status message to the client */
void sendstatus(int to, char *class_string, char *message_string)
{
	filtertext(class_string);
	filtertext(message_string);
        sprintf(pbuf, "%c%s\001%s", ICB_M_STATUS, class_string, message_string);
	doSend(-1, to);
}

/* send "end of command" to the client */
void send_cmdend(int to, char *output_string)
{
	filtertext(output_string);
      	sprintf(pbuf, "%c%s\001\001%s", ICB_M_CMDOUT, "ec", output_string);
	doSend(-1, to);
}

/* send simple command output message to the client */
void sends_cmdout(int to, char *output_string)
{
	filtertext(output_string);
      	sprintf(pbuf, "%c%s\001%s", ICB_M_CMDOUT, "co", output_string);
	doSend(-1, to);
}


/* send beep message to a client */
void sendbeep(int from, int to)
{
	if ( u_tab[to].nobeep != 0 )
	{
	    senderror(from, "User has nobeep enabled.");

	    if ( u_tab[to].nobeep == 2 )
	    {
		sprintf (mbuf, "%s attempted (and failed) to beep you",
		    u_tab[from].nickname);
		sendstatus (to, "No-Beep", mbuf);
	    }

	    return;
	}
        sprintf(pbuf, "%c%s", ICB_M_BEEP, u_tab[from].nickname);
	doSend(from, to);
}


void autoBeep(int to)
{
	sendperson(NICKSERV, to, "Beep yerself!");
	u_tab[NICKSERV].t_recv = time(NULL);
}

/* n  =  fd of their socket */
void s_new_user(int n)
{
	char *cp;

	/* construct proto(col) message */
	sprintf(pbuf, "%c%d\001%s\001%s",ICB_M_PROTO,PROTO_LEVEL,
	   thishost, VERSION);
	doSend(-1, n);
	cp = getremotename(n);

	if (cp == NULL) {
		sprintf(mbuf, "[CONNECT] %d", n);
		mdb(mbuf);
		return;
	}

	if (strlen(cp) == 0) {
		sprintf(mbuf, "[CONNECT] %d", n);
		mdb(mbuf);
		return;
	}

#if defined(SHORT_HOSTNAME) && defined(FQDN)
	if(strcasecmp(SHORT_HOSTNAME, cp) == 0) {
		cp = FQDN;
	}
#endif	/* SHORT_HOSTNAME && FQDN */

	sprintf(mbuf, "[CONNECT] %d: %s", n, cp);
	memset(u_tab[n].nodeid, 0, MAX_NODELEN+1);
	strncpy(u_tab[n].nodeid, cp, MAX_NODELEN);
	mdb(mbuf);
}

void send_loginok(int to)
{
	/* construct loginok message */
	sprintf(pbuf, "%c",ICB_M_LOGINOK);
	doSend(-1, to);
}

/* send an important message to the client */
void sendimport(int to, char *class_string, char *output_string)
{
	filtertext(class_string);
	filtertext(output_string);
        sprintf(pbuf, "%c%s\001%s", ICB_M_IMPORTANT, class_string,
		output_string);
	doSend(-1, to);
}



/*
	Copyright (c) 1991 by Keith Graham
	Modifications Copyright (c) 1991 by John Atwood deVries II

	"to" is the destination socket..
*/

void user_wline(int to, 
		char *mod, 
		char *nick, 
		int idle, 
		int resp, 
		int login, 
		char *user, 
		char *site, 
		char *name)
{ 
	sprintf(pbuf,
		"%cwl\001%s\001%s\001%ld\001%ld\001%ld\001%s\001%s\001%s\000",
		ICB_M_CMDOUT, mod, nick, (long)idle, (long)resp, (long)login, user, site, name);
	doSend(-1, to);
}

/* XXX what the hell does this do? what are c and d? -hoche 5/10/00 */
/*
void user_wgroupline(int to, char *group, char* topic, c, d)
{ 
	sprintf(pbuf,
		"%cwg\001%s\001%s\001%s\001%s\000",
		ICB_M_CMDOUT, group, topic, c, d);
	doSend(-1, to);
}
*/

void user_whead(int to)
{ 
	sprintf(pbuf,"%cwh\000",ICB_M_CMDOUT);
	doSend(-1, to);
}

int doSend(int from,int to)
{
	int ret;
	char line[256];
	char tbuf[256];

	if(to < 0) {
		sprintf(tbuf, "[ERROR] Attempted to send to negative fd: %d",
			to);
		mdb(tbuf);
		return -1;
		}
	if(to >= MAX_REAL_USERS) {
		*pp = ' ';
		switch (*(pp+1)) {
		case ICB_M_BEEP: /* someone beeped us */
			autoBeep(from);
			break;
		case ICB_M_PERSONAL: /* someone sent us a message */
			split(pp);
			memset(line, 0, 256);
			sprintf(line, "%s\001%s", getword(fields[1]), 
				get_tail(fields[1]));
			cmdmsg(from, line);
			break;
		default: /* do nothing -- ignore */
			break;
		}
	} else {
		if (strlen(pbuf) > 254) {
			mdb ("[ERROR] doSend: pbuf too large:");
			sprintf (mbuf, "[ERROR] %s", pbuf);
			mdb (mbuf);
			sprintf (mbuf, "Cannot transmit packet: too large");
			senderror(from, mbuf);
			return -1;
			}
		if (S_kill[to] > 0) 
			return -1;
		setlen;
		if ((ret = _sendpacket(to,pp)) < 0) {
			sprintf(tbuf, "[ERROR] doSend: %d: %s (%d)",
				to, strerror(errno), ret);
			mdb(tbuf);
			if (ret == -2) {
				/* bad news! */
				S_kill[to]++;
				/* need to clear the notifies of this user,
				   lest he try to someone else who's dead
				   who has this guy in his notify. Endless
				   loop! */
				/* Don't know if it's still needed, but it
				   doesn't hurt */
				nlclear(u_tab[to].n_notifies);
				nlclear(u_tab[to].s_notifies);
				}
			}
	}
	return 0;
}
