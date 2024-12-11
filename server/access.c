
#include "config.h"

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_NDBM_H
#include <ndbm.h>
#elif defined (HAVE_GDBM_NDBM_H)
#include <gdbm/ndbm.h>
#elif defined (HAVE_DB1_NDBM_H)
#include <db1/ndbm.h>
#endif
#include <fcntl.h>
#include <time.h>

#include "server.h"
#include "externs.h"
#include "send.h"
#include "users.h"
#include "mdb.h"
#include "strutil.h"
#include "unix.h"

#include "s_commands.h"  /* for talk_report() */

int setsecure(int forWhom, int secure, DBM *openDb)
{
	DBM            *db;
	char           key[64];
	datum          result, key_dat, request;
	char           line[255];
	int            retval = 0;

	if (strlen(u_tab[forWhom].realname) == 0) {
		senderror(forWhom, 
			"You must be registered to change your security.");
		return -1;
		}

	if (openDb)
		db = openDb;
	else
	{
		if ((db = dbm_open(USERDB, O_RDWR, 0)) == NULL) 
		{
			int err = errno;
			sprintf(line, "[ERROR] User Database Open: %s", strerror(err));
			mdb(line);
			sprintf(line, "User Database Open: %s", strerror(err));
			senderror(forWhom, line);
			return -1;
		}
	}

	if (secure == 0) 
	{
		strcpy(key, u_tab[forWhom].nickname);
		strcat(key, ".secure");
		lcaseit(key);
		key_dat.dptr = key;
		key_dat.dsize = strlen(key);
		result = dbm_fetch(db, key_dat);
		if (result.dptr)
			dbm_delete(db, key_dat);
		sends_cmdout (forWhom, "Security set to automatic.");
	}
	else if (secure == 1) 
	{
		strcpy(key, u_tab[forWhom].nickname);
		strcat(key, ".secure");
		lcaseit(key);
		key_dat.dptr = key;
		key_dat.dsize = strlen(key);
		memset(line, 0, 255);
		strcpy(line, "SECURED");
		request.dptr = line;
		request.dsize = strlen(line);
		result = dbm_fetch(db, key_dat);
		if (!result.dptr)
			dbm_store(db, key_dat, request, DBM_INSERT);
		else 
			dbm_store(db, key_dat, request, DBM_REPLACE);
		sends_cmdout (forWhom, "Security set to password required.");
	}
	else 
	{
		mdb("Illegal setsecure value");
		retval = -2;
	}

	if (!openDb)
		dbm_close(db);
	return retval;
}

int valuser(char *user, char *password, DBM *openDb)
{
	DBM            *db;
	char           key[64];
	datum          result, key_dat;
	char           line[255];

	if (strlen(password) == 0)
		return -1;
	
	if ( strlen(password) > MAX_PASSWDLEN )
	    password[MAX_PASSWDLEN] = '\0';

	if (openDb)
		db = openDb;
	else
	{
		if ((db = dbm_open(USERDB, O_RDWR, 0)) == NULL) 
		{
			sprintf(line, "[ERROR] User Database Open: %s", 
				strerror(errno));
			mdb(line);
			return -1;
		}
	}

	strcpy(key, user);
	strcat(key, ".password");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (!openDb)
		dbm_close(db);

	if (!result.dptr) 
		return -1;	/* not found */
	memset(line, 0, 255);
	strncpy(line, result.dptr, result.dsize);
	if (strcmp(line, password) != 0)
		return -1;
	else
		return 0;
}

int check_auth(int n)
{
    int auth;

    auth = 0;
    if (strcasecmp(u_tab[n].nickname, "ADMIN") == 0)
	    auth++;
    if (strcasecmp(u_tab[n].password, ADMIN_PASSWD) == 0)
	    auth++;
    sprintf(mbuf, "[INFO] Checking authorization of %s: %s",
	    u_tab[n].nickname, auth == 2 ? "yes" : "no");
    mdb(mbuf);
    return (auth == 2);
}

int nickdelete(int forWhom, char *password, DBM *openDb)
{
	DBM            *db;
	char           key[64];
	datum          result, key_dat;
	char           line[255];
	int            i;

	if ( strlen (password) == 0 )
	{
	    senderror (forWhom, "Password cannot be null");
	    return -1;
	}

	if ( strlen(password) > MAX_PASSWDLEN )
	    password[MAX_PASSWDLEN] = '\0';

	if (strlen(u_tab[forWhom].realname) == 0) {
		senderror(forWhom, 
			"You must be registered to delete your entry.");
		return -1;
		}

	if (openDb)
		db = openDb;
	else
	{
		if ((db = dbm_open(USERDB, O_RDWR, 0)) == NULL) 
		{
			int err = errno;
			sprintf(line, "[ERROR] User Database Open: %s", 
								strerror(err));
			mdb(line);
			sprintf(line, "User Database Open: %s", strerror(err));
			senderror(forWhom, line);
			return -1;
		}
	}

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".password");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if ( !result.dptr ) {
	    senderror (forWhom, "You don't have a password.");
	    if (!openDb)
		    dbm_close(db);
	    return -1;
	}
	
	strncpy(line, result.dptr, result.dsize);

	if ( strcmp (line, password) != 0 ) {
	    senderror (forWhom, "Password incorrect.");
	    if (!openDb)
		    dbm_close(db);
	    return -1;
	}

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".secure");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (result.dptr)
		dbm_delete(db, key_dat);

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".realname");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (result.dptr)
		dbm_delete(db, key_dat);

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".email");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (result.dptr)
		dbm_delete(db, key_dat);

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".signoff");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (result.dptr)
		dbm_delete(db, key_dat);

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".signon");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (result.dptr)
		dbm_delete(db, key_dat);

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".nick");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (result.dptr)
		dbm_delete(db, key_dat);

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".password");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (result.dptr)
		dbm_delete(db, key_dat);

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".phone");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (result.dptr)
		dbm_delete(db, key_dat);

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".text");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (result.dptr)
		dbm_delete(db, key_dat);

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".home");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (result.dptr)
		dbm_delete(db, key_dat);

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".addr");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (result.dptr)
		dbm_delete(db, key_dat);

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".nummsg");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (result.dptr)
		dbm_delete(db, key_dat);

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".www");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (result.dptr)
		dbm_delete(db, key_dat);

	for (i = 1; i <= MAX_WRITES; i++) {
		sprintf(key, "%s.header%d", u_tab[forWhom].nickname, i);
		lcaseit(key);
		key_dat.dptr = key;
		key_dat.dsize = strlen(key);
		result = dbm_fetch(db, key_dat);
		if (result.dptr)
			dbm_delete(db, key_dat);
		sprintf(key, "%s.message%d", u_tab[forWhom].nickname, i);
		lcaseit(key);
		key_dat.dptr = key;
		key_dat.dsize = strlen(key);
		result = dbm_fetch(db, key_dat);
		if (result.dptr)
			dbm_delete(db, key_dat);
		sprintf(key, "%s.from%d", u_tab[forWhom].nickname, i);
		lcaseit(key);
		key_dat.dptr = key;
		key_dat.dsize = strlen(key);
		result = dbm_fetch(db, key_dat);
		if (result.dptr)
			dbm_delete(db, key_dat);
		}

	sends_cmdout(forWhom, "Record Deleted");
	if (!openDb)
		dbm_close(db);
	return 0;
}

int nickwritemsg(int forWhom, char *user, char *message, DBM *openDb)
{
	DBM            *db;
	char           key[64];
	datum          result, key_dat, request;
	char           line[255], timebuf[255];
	int            count, i;

	if ((strlen(user) == 0) || (strlen(message) ==0)) {
		sends_cmdout(forWhom, "Usage: write nickname message text");
		return -1;
		}

	if (strlen(u_tab[forWhom].realname) == 0) {
		senderror(forWhom, 
			"You must be registered to write a message.");
		return -1;
		}

	if (openDb)
		db = openDb;
	else
	{
		if ((db = dbm_open(USERDB, O_RDWR, 0)) == NULL) 
		{
			int err = errno;
			sprintf(line, "[ERROR] User Database Open: %s", 
								strerror(err));
			mdb(line);
			sprintf(line, "User Database Open: %s", strerror(err));
			senderror(forWhom, line);
			return -1;
		}
	}

	memset(line, 0, 255);
	strcpy(key, user);
	strcat(key, ".nick");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (!result.dptr) {
		sprintf(line, "%s is not registered", user);
		senderror(forWhom, line);
		if (!openDb)
			dbm_close(db);
		return -1;
		}

	memset(line, 0, 255);
	strcpy(key, user);
	strcat(key, ".nummsg");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (!result.dptr) {
		count = 0;
		i = 1;
		}
	else {
		strncpy(line, result.dptr, result.dsize);
		count = atoi(line);
		i = 0;
		}
	count++;
	if (count > MAX_WRITES) {
		senderror(forWhom, "User mailbox full");
		if (!openDb)
			dbm_close(db);
		return -1;
		}
	gettime();
	strftime(timebuf, 255, "%e-%h-%Y %H:%M %Z", localtime(&curtime));
	sprintf(line, "Message left at %s:", timebuf);
	sprintf(key, "%s.header%d", user, count);
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	request.dptr = line;
	request.dsize = strlen(line);
	dbm_store(db, key_dat, request, DBM_INSERT);
	sprintf(key, "%s.from%d", user, count);
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	request.dptr = u_tab[forWhom].nickname;
	request.dsize = strlen(u_tab[forWhom].nickname);
	dbm_store(db, key_dat, request, DBM_INSERT);
	sprintf(key, "%s.message%d", user, count);
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	filtertext(message);
	request.dptr = message;
	request.dsize = strlen(message);
	dbm_store(db, key_dat, request, DBM_INSERT);
	strcpy(key, user);
	strcat(key, ".nummsg");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	sprintf(line, "%d", count);
	request.dptr = line;
	request.dsize = strlen(line);
	if (i == 1)
		dbm_store(db, key_dat, request, DBM_INSERT);
	else
		dbm_store(db, key_dat, request, DBM_REPLACE);
	sendstatus(forWhom, "Message", "Text saved to file");
	if ((i = find_user(user)) > 0) {
		sprintf(line, "%s is logged in now.", u_tab[i].nickname);
		sendstatus(forWhom, "Warning", line);
		sprintf(line, "You have %d message", count);
		if (count > 1) strcat(line, "s");
		sendstatus(i, "Message", line);
		}

	if (!openDb)
		dbm_close(db);

	return 0;
}

int nickckmsg(int forWhom, DBM *openDb)
{
	DBM            *db;
	char           key[64];
	datum          result, key_dat, request;
	char           line[255];
	int            retval;

	if (strlen(u_tab[forWhom].realname) == 0) {
		return -1;
		}

	if (openDb)
		db = openDb;
	else
	{
		if ((db = dbm_open(USERDB, O_RDWR, 0)) == NULL) 
		{
			int err = errno;
			sprintf(line, "[ERROR] User Database Open: %s", strerror(err));
			mdb(line);
			sprintf(line, "User Database Open: %s", strerror(err));
			senderror(forWhom, line);
			return -1;
		}
	}

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".nummsg");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (!result.dptr) {
		strcpy(line, "0");
		request.dptr = line;
		request.dsize = strlen(line);
		dbm_store(db, key_dat, request, DBM_INSERT);
		retval = 0;
		}
	else {
		strncpy(line, result.dptr, result.dsize);
		retval = atoi(line);
		}

	if (!openDb)
		dbm_close(db);

	return(retval);
}

int nickreadmsg(int forWhom, DBM *openDb)
{
	DBM            *db;
	char           key[64];
	datum          result, key_dat, request;
	char           line[255], from[MAX_NICKLEN+1];
	int            count, i;

	if (strlen(u_tab[forWhom].realname) == 0) {
		senderror(forWhom, 
			"You must be registered to read any messages.");
		return -1;
		}

	if (openDb)
		db = openDb;
	else
	{
		if ((db = dbm_open(USERDB, O_RDWR, 0)) == NULL) 
		{
			int err = errno;
			sprintf(line, "[ERROR] User Database Open: %s", 
								strerror(err));
			mdb(line);
			sprintf(line, "User Database Open: %s", strerror(err));
			senderror(forWhom, line);
			return -1;
		}
	}

	memset(line, 0, 255);
	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".nummsg");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (!result.dptr) {
		strcpy(line, "0");
		request.dptr = line;
		request.dsize = strlen(line);
		dbm_store(db, key_dat, request, DBM_INSERT);
		senderror(forWhom, "No messages");
		}
	else 
	{
		strncpy(line, result.dptr, result.dsize);
		count = atoi(line);
		strcpy(line, "0");
		request.dptr = line;
		request.dsize = strlen(line);
		dbm_store(db, key_dat, request, DBM_REPLACE);
		if (count == 0)
			senderror(forWhom, "No messages");
		else for (i = 1; i <= count; i++) 
		{
			sprintf(key, "%s.header%d", u_tab[forWhom].nickname, i);
			lcaseit(key);
			key_dat.dptr = key;
			key_dat.dsize = strlen(key);
			result = dbm_fetch(db, key_dat);
			if (result.dptr) {
				memset(line, 0, 255);
				strncpy(line, result.dptr, result.dsize);
				sends_cmdout(forWhom, line);
				}
			dbm_delete(db, key_dat);
			sprintf(key,"%s.from%d", u_tab[forWhom].nickname, i);
			lcaseit(key);
			key_dat.dptr = key;
			key_dat.dsize = strlen(key);
			result = dbm_fetch(db, key_dat);
			memset(from, 0, MAX_NICKLEN+1);
			if (result.dptr)
				strncpy(from, result.dptr, result.dsize);
			else
				strcpy(from, "Server");
			dbm_delete(db, key_dat);
			sprintf(key,"%s.message%d", u_tab[forWhom].nickname, i);
			lcaseit(key);
			key_dat.dptr = key;
			key_dat.dsize = strlen(key);
			result = dbm_fetch(db, key_dat);
			if (result.dptr) {
				memset(line, 0, 255);
				strncpy(line, result.dptr, result.dsize);
				sprintf(pbuf, "%c%s\001%s", ICB_M_PERSONAL, 
				   from, line);
				doSend(-1, forWhom);
				/* sends_cmdout(forWhom, line); */
				}
			dbm_delete(db, key_dat);
		}
	}

	if (!openDb)
		dbm_close(db);
	return 0;
}

int nickwritetime(int forWhom, int class, DBM *openDb)
{
	DBM            *db;
	char           key[64];
	datum          result, key_dat, request;
	char           line[255];
	char           timebuf[255];
	struct tm      *localtime();

	if (strlen(u_tab[forWhom].realname) == 0) {
		return -1;	/* This shouldn't happen */
	}

	gettime();	/* update to the current time */
	strftime(timebuf, 255, "%e-%h-%Y %H:%M %Z", localtime(&curtime));

	if (openDb)
		db = openDb;
	else
	{
		if ((db = dbm_open(USERDB, O_RDWR, 0)) == NULL)
		{
			int err = errno;
			sprintf(line, "[ERROR] User Database Open: %s", strerror(err));
			mdb(line);
			sprintf(line, "User Database Open: %s", strerror(err));
			senderror(forWhom, line);
			return -1;
		}
	}

	/* if class != 0, it's a signoff and we want to make
	 * sure the nickname is still valid before storing the
	 * signoff info in the db. otherwise people delete their
	 * records and then when quitting this adds an orphaned db entry
	 */
	if ( class != 0 )
	{
	    sprintf(key, "%s.nick", u_tab[forWhom].nickname);
	    lcaseit(key);
	    key_dat.dptr = key;
	    key_dat.dsize = strlen(key);
	    result = dbm_fetch(db, key_dat);

	    /* no match for their .nick entry means it's not valid so return */
	    if ( !result.dptr )
	    {
			if (!openDb)
				dbm_close(db);
			return -1;
	    }
	}

	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".");

	if (class == 0)
		strcat(key, "signon");
	else
		strcat(key, "signoff");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	request.dptr = timebuf;
	request.dsize = strlen(timebuf);
	result = dbm_fetch(db, key_dat);
	if (!result.dptr) {		 /* no entry */
		dbm_store(db, key_dat, request, DBM_INSERT);
	}
	else {
		dbm_store(db, key_dat, request, DBM_REPLACE);
	}

	if (!openDb)
		dbm_close(db);

	return 0;
}

int nickchinfo(int forWhom, char *tag, char *data, int max, char *message, DBM *openDb)
{
	DBM            *db;
	char           key[64];
	datum          result, key_dat, request;
	char           line[255];
	char           newstr[255];

	if (strlen(u_tab[forWhom].realname) == 0) {
		senderror(forWhom, 
			"Setting your name requites that you be registered.");
		return -1;
		}

	if (strlen(data) > max) {
		memset(newstr, 0, 255);
		strncpy(newstr, data, max);
		data = newstr;
		sprintf (line, "%s truncated to %d characters", message, max);
		senderror(forWhom, line);
		}

	if (openDb)
		db = openDb;
	else
	{
		if ((db = dbm_open(USERDB, O_RDWR, 0)) == NULL) 
		{
			int err = errno;
			sprintf(line, "[ERROR] User Database Open: %s", strerror(err));
			mdb(line);
			sprintf(line, "User Database Open: %s", strerror(err));
			senderror(forWhom, line);
			return -1;
		}
	}

	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".");
	strcat(key, tag);
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	request.dptr = data;
	request.dsize = strlen(data);
	result = dbm_fetch(db, key_dat);
	if (!result.dptr) {		 /* no entry */
		dbm_store(db, key_dat, request, DBM_INSERT);
		sprintf (line, "%s set to '%s'", message, data);
		sends_cmdout(forWhom, line);
		}
	else {
		dbm_store(db, key_dat, request, DBM_REPLACE);
		sprintf (line, "%s set to '%s'", message, data);
		sends_cmdout(forWhom, line);
		}

	if (!openDb)
		dbm_close(db);
	return 0;
}

int nickchpass(int forWhom, char *oldpw, char *newpw, DBM *openDb)
{
	DBM            *db;
	char           key[64];
	datum          result, key_dat, request;
	char           line[255];

	if ( strlen (oldpw) > MAX_PASSWDLEN )
	    oldpw[MAX_PASSWDLEN] = '\0';

	if ( strlen (newpw) > MAX_PASSWDLEN )
	    newpw[MAX_PASSWDLEN] = '\0';

	if (openDb)
		db = openDb;
	else
	{
		if ((db = dbm_open(USERDB, O_RDWR, 0)) == NULL) 
		{
			int err = errno;
			sprintf(line, "[ERROR] User Database Open: %s", strerror(err));
			mdb(line);
			sprintf(line, "User Database Open: %s", strerror(err));
			senderror(forWhom, line);
			return -1;
		}
	}

	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".password");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (!result.dptr) {		 /* This nick isn't registered */
		sprintf(line, "Authorization failure");
		senderror(forWhom, line);
		}
	else {
		memset(line, 0, 255);
		strncpy(line, result.dptr, result.dsize);
		if (strcmp(line, oldpw)) {
			sprintf(line, "Authorization failure");
			senderror(forWhom, line);
			}
		else {
			request.dptr = newpw;
			request.dsize = strlen(newpw);
			if (request.dsize <= 0) {
				sprintf(line, "Missing paramater");
				senderror(forWhom,line);
				}
			else {
				dbm_store(db, key_dat, request, DBM_REPLACE);
				sprintf(line, "Password changed");
				sendstatus(forWhom,"Pass",line);
				strcpy(key, u_tab[forWhom].nickname);
				strcat(key, ".home");
				lcaseit(key);
				key_dat.dptr = key;
				key_dat.dsize = strlen(key);
				strcpy(line, u_tab[forWhom].loginid);
				strcat(line, "@");
				strcat(line, u_tab[forWhom].nodeid);
				request.dptr = line;
				request.dsize = strlen(line);
				dbm_store(db, key_dat, request, DBM_REPLACE);
				}
			}
		}

	if (!openDb)
		dbm_close(db);
	return 0;
}

/*
 * nickwrite() - register a nick, creating it's database entry if need be,
 * otherwise verifying the password on the existing record
 *
 * args:
 *  forWhom - index into the user table
 *  password - password for the nick
 *  verifyOnly - set to 1 to prevent nicks from being created. ie, return
 *    with a failure code if the nick isn't already in the db
 *
 * returns: 0 on success, -1 on failure
 */
int nickwrite(int forWhom, char *password, int verifyOnly, DBM *openDb)
{
	DBM            *db;
	char           key[64];
	datum          result, key_dat, request;
	char           line[255];
	int            retval, i, j;

	retval = -1;

	if (strlen(password) == 0) {
	    senderror (forWhom, "Password cannot be null");
	    return (retval);
	}

	if ( strlen(password) > MAX_PASSWDLEN )
	    password[MAX_PASSWDLEN] = '\0';

	if (openDb)
		db = openDb;
	else
	{
		if ((db = dbm_open(USERDB, O_RDWR, 0)) == NULL)
		{
		    int err = errno;
		    sprintf(line, "[ERROR] User Database Open: %s", strerror(err));
		    mdb(line);
		    sprintf(line, "User Database Open: %s", strerror(err));
		    senderror(forWhom, line);
		    return (retval);
		}
	}

	strcpy(key, u_tab[forWhom].nickname);
	strcat(key, ".password");
	lcaseit(key);
	key_dat.dptr = key;
	key_dat.dsize = strlen(key);
	result = dbm_fetch(db, key_dat);
	if (!result.dptr) {		/* This nick isn't registered */
		if ( verifyOnly == 1 )
		{
		    sprintf (line, "[ERROR] Nick %s not found",
			u_tab[forWhom].nickname);
		    senderror (forWhom, line);
		}
		else
		{
		    strcpy(line, password);
		    request.dptr = line;
		    request.dsize = strlen(line);
		    dbm_store(db, key_dat, request, DBM_INSERT);

		    strcpy(key, u_tab[forWhom].nickname);
		    strcat(key, ".nick");
		    lcaseit(key);
		    key_dat.dptr = key;
		    key_dat.dsize = strlen(key);
		    strcpy(line, u_tab[forWhom].nickname);
		    request.dptr = line;
		    request.dsize = strlen(line);
		    dbm_store(db, key_dat, request, DBM_INSERT);

		    strcpy(key, u_tab[forWhom].nickname);
		    strcat(key, ".home");
		    lcaseit(key);
		    key_dat.dptr = key;
		    key_dat.dsize = strlen(key);
		    strcpy(line, u_tab[forWhom].loginid);
		    strcat(line, "@");
		    strcat(line, u_tab[forWhom].nodeid);
		    request.dptr = line;
		    request.dsize = strlen(line);
		    dbm_store(db, key_dat, request, DBM_INSERT);
		    sendstatus(forWhom, "Register", "Nick registered");
		    strcpy(u_tab[forWhom].realname, "registered");
		    nickwritetime(forWhom, 0, db);
		    strcpy(u_tab[forWhom].password, password); /* jonl */
		    retval = 0;
		}
	}
	else
	{
		memset(line, 0, 255);
		strncpy(line, result.dptr, result.dsize);
		if (strcmp(line, password)) {
		    sprintf(mbuf, "Authorization failure");
		    senderror(forWhom, mbuf);
		    memset(u_tab[forWhom].realname, 0, MAX_REALLEN + 1);
		}
		else
		{
		    sendstatus(forWhom, "Register", "Nick registered");
		    strcpy(u_tab[forWhom].realname, "registered");
		    nickwritetime(forWhom, 0, db);
		    strcpy(u_tab[forWhom].password, password); /* jonl */
		    for (i = 1; i < MAX_GROUPS; i++)
		    if ((g_tab[i].modtimeout > 0.0) &&
		      (strcmp(g_tab[i].missingmod, 
			    u_tab[forWhom].nickname)==0))
		    {
			g_tab[i].modtimeout = 0;
			g_tab[i].mod = forWhom;
			memset(g_tab[i].missingmod, 0, MAX_NICKLEN);
			sprintf(mbuf, "%s is the active moderator again.",
			    u_tab[forWhom].nickname);
			for (j = 1; j < MAX_REAL_USERS; j++)
			    if ((strcasecmp(u_tab[j].group, g_tab[i].name)
				== 0) && (j != forWhom))
				sendstatus(j, "Mod", mbuf);
			sprintf(mbuf, "You are the moderator of group %s",
			    g_tab[i].name);
			sendstatus(forWhom, "Mod", mbuf);
		    }
		    if ((i = nickckmsg(forWhom, db)) > 0) {
			if (i == 1)
			    sendstatus(forWhom, "Message", 
				"You have 1 message");
			else {
			    sprintf(mbuf, "You have %d messages", i);
			    sendstatus(forWhom, "Message", mbuf);
			}
		    }
		    for ( i = 1; i < MAX_GROUPS; i++ )
		    	talk_report (forWhom, i);

		    retval = 0;
		}
	}

	if (!openDb)
		dbm_close(db);

	return (retval);
}

int nicklookup(int forWhom, char *theNick, DBM *openDb)
{
	DBM            *db;
	char           key[64];
	datum          home, nick, request;
	char           line[255];
	char           temp[255];
	char           nickstr[255];
	int            retval;
	int            count = 0;
	char           *s, *p, *lastw;
	char           tmp1, tmp2;

	if (strlen(theNick) == 0)
	{
	    if ((forWhom > 0) && (forWhom < MAX_REAL_USERS))
		sends_cmdout(forWhom, "Usage: whois nickname");
	    return -1;
	}

	if (strlen(theNick) > MAX_NICKLEN)
	{
	    senderror(forWhom, "nickname too long");
	    return -1;
	}

	if (openDb)
		db = openDb;
	else
	{
		if ((db = dbm_open(USERDB, O_RDONLY, 0)) == NULL)
		{
		    int err = errno;
		    sprintf(line, "[ERROR] User Database Open: %s", strerror(err));
		    mdb(line);
		    sprintf(line, "User Database Open: %s", strerror(err));
		    senderror(forWhom, line);
		    return -1;
		}
	}

	strcpy (key, theNick);
	strcat (key, ".nick");
	lcaseit(key);
	request.dptr = key;
	request.dsize = strlen(key);
	nick = dbm_fetch(db, request);
	if (nick.dptr)
	{
	    memset(nickstr, 0, 255);
	    strncpy(nickstr, nick.dptr, nick.dsize);
	}

	strcpy (key, theNick);
	strcat (key, ".home");
	lcaseit(key);
	request.dptr = key;
	request.dsize = strlen(key);
	home = dbm_fetch(db, request);
	/* mdb("have looked up name"); */

	if (home.dptr)
	{
	    /* mdb("user's name was found"); */
	    retval = 0;
	    if (forWhom >= 0)
	    {
		memset(line, 0, 255);
		memset(temp, 0, 255);
		strcpy(temp, nickstr);
		sprintf(line, "Nickname:     %s", temp);
		while (strlen(line) < 36)
			strcat(line, " ");
		strcat(line, "Address:   ");
		strncat(line, home.dptr, home.dsize);
		sends_cmdout(forWhom, line);
		strcpy (key, theNick);
		strcat (key, ".phone");
		lcaseit(key);
		request.dptr = key;
		request.dsize = strlen(key);
		nick = dbm_fetch(db, request);
		sprintf(line, "Phone Number: ");
		if (!nick.dptr)
		    strcat(line, "(None)");
		else
		    strncat(line, nick.dptr, nick.dsize);
		while (strlen(line) < 36)
		    strcat(line, " ");
		strcpy (key, theNick);
		strcat (key, ".realname");
		lcaseit(key);
		request.dptr = key;
		request.dsize = strlen(key);
		nick = dbm_fetch(db, request);
		strcat(line, "Real Name: ");
		if (!nick.dptr)
		    strcat(line, "(None)");
		else
		    strncat(line, nick.dptr, nick.dsize);
		sends_cmdout(forWhom, line);

		memset(line, 0, 255);
		memset(temp, 0, 255);
		strcpy (key, theNick);
		strcat (key, ".signon");
		lcaseit(key);
		request.dptr = key;
		request.dsize = strlen(key);
		nick = dbm_fetch(db, request);
		strcpy(line, "Last signon:  ");
		if (nick.dptr) 
		    strncat(line, nick.dptr, nick.dsize);
		else
		    strcat(line, "(unknown)");
		while (strlen(line) < 36)
		    strcat(line, " ");
		memset(temp, 0, 255);
		strcpy (key, theNick);
		strcat (key, ".signoff");
		lcaseit(key);
		request.dptr = key;
		request.dsize = strlen(key);
		nick = dbm_fetch(db, request);
		strcat(line, "Last signoff:  ");
		if (nick.dptr)
		    strncat(line, nick.dptr, nick.dsize);
		else
		    strcat(line, "(unknown)");
		sends_cmdout(forWhom, line);

		memset(line, 0, 255);
		memset(temp, 0, 255);
		strcpy (key, theNick);
		strcat (key, ".email");
		lcaseit(key);
		request.dptr = key;
		request.dsize = strlen(key);
		nick = dbm_fetch(db, request);
		if (nick.dptr)
		{
		    strcpy(line, "E-mail addr:  ");
		    strncat(line, nick.dptr, nick.dsize);
		    sends_cmdout(forWhom, line);
		}

		memset(line, 0, 255);
		memset(temp, 0, 255);
		strcpy (key, theNick);
		strcat (key, ".www");
		lcaseit(key);
		request.dptr = key;
		request.dsize = strlen(key);
		nick = dbm_fetch(db, request);
		if (nick.dptr)
		{
		    strcpy(line, "WWW:  ");
		    strncat(line, nick.dptr, nick.dsize);
		    sends_cmdout(forWhom, line);
		}

		memset(line, 0, 255);
		memset(temp, 0, 255);
		strcpy (key, theNick);
		strcat (key, ".addr");
		lcaseit(key);
		request.dptr = key;
		request.dsize = strlen(key);
		nick = dbm_fetch(db, request);
		if (nick.dptr)
		{
		    strncpy(line, nick.dptr, nick.dsize);
		    sends_cmdout(forWhom, "Street Address:");
		    s = line;
		    strcpy(temp, "  ");
		    while (strlen(s) > 0)
		    {
			if (*s == '|')
			{
			    sends_cmdout(forWhom, temp);
			    strcpy(temp, "  ");
			}
			else
			    strncat(temp, s, 1);

			s++;
		    }
		    sends_cmdout(forWhom, temp);
		}

		memset(line, 0, 255);
		memset(temp, 0, 255);
		strcpy (key, theNick);
		strcat (key, ".text");
		lcaseit(key);
		request.dptr = key;
		request.dsize = strlen(key);
		nick = dbm_fetch(db, request);
		if (nick.dptr)
		{
		    strncpy(line, nick.dptr, nick.dsize);
		    s = line;
		    /* traverse s and try to break on a word */
		    p = s;
		    lastw = p;
		    while (*p != '\0')
		    {
			if (*p == '\n')
			{
			    *p++ = '\0';
			    strcpy(temp, "Text: ");
			    strcat(temp, s);
			    sends_cmdout(forWhom, temp);
			    count = 0;
			    lastw = p;
			    s = p;
			    continue;
			}

			/* remember location of last word break */
			if (*p == ' ')
				lastw = p;

			/* break line if we are at max length */
			if (count == (MAX_TEXTLEN - 1))
			{
			    if ((p - lastw) > 40)
			    {
				/* have to break in the middle of a word */
				tmp1 = *(p - 1);
				tmp2 = *p;
				*(p - 1) = '-';
				*p = '\0';
				strcpy(temp, "Text: ");
				strcat(temp, s);
				sends_cmdout(forWhom, temp);
				*(p - 1) = tmp1;
				*p = tmp2;
				p--;
				s = p;
			    }
			    else
			    {
				/* break at last space char */
				tmp1 = *lastw;
				*lastw = '\0';
				strcpy(temp, "Text: ");
				strcat(temp, s);
				sends_cmdout(forWhom, temp);
				*lastw = tmp1;
				p = lastw + 1;
				s = p;
			    }

			    lastw = p;
			    count = 0;
			}
			else
			{
			    count++;
			    p++;
			}
		    }

		    if (count > 0)
			    strcpy(temp, "Text: ");

		    strcat(temp, s);
		    sends_cmdout(forWhom, temp);
		}
	    }
	    else
	    {
		/*
		 * check their user@host with the user@host from
		 * their database entry. if it doesn't match, not
		 * auto-auth possible. if it does match, check the
		 * setting of the .secure to see if they allow auto-auth
		 */
		memset(line, 0, 255);
		memset(temp, 0, 255);
		sprintf(temp, "%s@%s", u_tab[-forWhom].loginid,
			u_tab[-forWhom].nodeid);
		strncat(line, home.dptr, home.dsize);
		if (strcasecmp(line, temp))
		    retval = -2;
		else
		{
		    memset(line, 0, 255);
		    memset(temp, 0, 255);
		    strcpy (key, theNick);
		    strcat (key, ".secure");
		    lcaseit(key);
		    request.dptr = key;
		    request.dsize = strlen(key);
		    nick = dbm_fetch(db, request);
		    if (nick.dptr) retval = -2;
		}
	    }
	}
	else
	{
	    if (forWhom >= 0)
	    {
		sprintf(mbuf, "%s is not in the database.", theNick);
		senderror(forWhom, mbuf);
	    }
	    retval = -1;
	}

	if (!openDb)
		dbm_close(db);
	return retval;
}
