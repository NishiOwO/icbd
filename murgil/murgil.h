#ifndef _MURGIL_H_
#define _MURGIL_H_

#include "cbuf.h"

int makenewport(char *host_name, int port_number);
void serverserve();
int _newconnect(int s);
int _sendpacket(int s, char *pkt);
int _readpacket(int user, struct Cbuf *p);
void disconnectuser(int userfd);
void ignore(int user);
void unignore(int user);

#endif /* #ifdef _MURGIL_H_ */
