#ifndef _ICBUTIL_H_
#define _ICBUTIL_H_

/* icbopenlogs()
 *
 * open the log file with append
 */
void icbopenlogs();

/* icbcloselogs
 *
 * close the current log file
 */
void icbcloselogs();

/* icbcyclelogs
 *
 * close and reopen the log files
 */
void icbcyclelogs();


/* icbexit
 *
 * disconnect all users and exit
 */
void icbexit();


/* icbdump
 *
 * dump the current state to a file so if we restart we can pick up
 * where we left off.
 */
void icbdump();

/* icbload
 *
 * load a server state from a file created by icbdump
 */
void icbload();

#endif /* #ifndef _ICBUTIL_H_ */
