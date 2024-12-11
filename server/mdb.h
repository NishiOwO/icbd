/* Copyright 1991 by John Atwood deVries II. */
/* For copying and distribution information, see the file COPYING. */

#ifndef _MDB_H_
#define _MDB_H_

/* a little debugging routine */

/* Message should be somewhat less than BUFSIZ chars, as the bufferspace
 * for message + hostname + timestamp is BUFSIZ. */

void mdb(char *message);

void vmdb(char *fmt, ...);


#endif /* #ifdef _MDB_H_ */

