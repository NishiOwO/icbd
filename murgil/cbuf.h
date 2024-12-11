/* Copyright (c) 1988 Carrick Sean Casey. All rights reserved. */
/* For copying and distribution information, see the file "copyright.h." */


#ifndef _CBUF_H_
#define _CBUF_H_

#include "../config.h"

/* packet input buffer */

struct Cbuf {
	char buf[USER_BUF_SIZE];
	char *rptr;	/* pointer to location for next read */
	char new;	/* set to 1 if next read is start of new data packet */
	unsigned char size;
	unsigned char remain;	/* packet characters remaining to be read */
};


#endif /* #ifdef _CBUF_H_ */
