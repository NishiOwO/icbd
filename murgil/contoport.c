/* Copyright (c) 1988 by Carrick Sean Casey. All rights reserved. */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "config.h"
#include "globals.h"

/*

Given a string, return an internet address.

This accepts address in the form of 

num.num.num.num
[num.etc]
machine.dom.dom.dom

*/
struct in_addr *_getaddress(char *s)
{
	static struct in_addr iaddr;
	struct hostent *hp;

	/* handle case of "[num.num.num.num]" */
	if (*s == '[') 
	{
		register char *p;

		s++;
		for (p = s; *p != '\0'; p++)
			if (*p == ']') {
				*p = '\0';
				break;
			}
	}

	/* handle case of "num.num.num.num" */
	if (*s >= '0' && *s <= '9') {
		if ((iaddr.s_addr = (unsigned long)inet_addr(s)) == -1)
			return(NULL);
		return(&iaddr);
	} 

	/* handle a symbolic address */
	if ((hp = gethostbyname(s)) == (struct hostent *) 0)
		return(NULL);

	/* copy address into inet address struct */
	memcpy(&iaddr.s_addr, hp->h_addr, (unsigned int) hp->h_length);

	return((struct in_addr *) &iaddr);
}

int connecttoport(char *host_name, int port_number)
{
	int s;
	struct sockaddr_in saddr;
	struct in_addr *addr;

	/* get the client host inet address */
	if ((addr = _getaddress(host_name)) == 0) {
		perror("can't lookup server INET address");
		return(-1);
	}

	/* insert hostname into address */
	memset(&saddr, 0, sizeof(saddr));
	memcpy(&saddr.sin_addr, addr, sizeof(struct in_addr));

	/* fill in socket domain and port number */
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons((u_short)port_number);

	/* create a socket */
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("couldn't create socket");
		return(-1);
	}

	/* connect it to the server inet address */
	if (connect(s, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
		perror("couldn't connect to server (not running?)");
		return(-1);
	}

	port_fd = s;
	return(0);
}
