/* Copyright (c) 1988 Carrick Sean Casey. All rights reserved. */


#include <sys/types.h>
#include <sys/socket.h>

#include "config.h"
#include "globals.h"

/* turn off input polling for a user */

void ignore(int user)
{
    FD_CLR(user, &fdset);
    FD_SET(user, &ignorefds);
}


/* restore input polling for a user */

void unignore(int user)
{
    FD_CLR(user, &ignorefds);
    FD_SET(user, &fdset);
}

