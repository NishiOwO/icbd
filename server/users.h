#ifndef _ICB_USERS_H_
#define _ICB_USERS_H_


/* clear a particular user entry */
void clear_user_item(int n);

/* fill a particular user entry */
void fill_user_entry(int n, 
		char *loginid, 
		char *nodeid, 
		char *nickname, 
		char *password, 
		char *group,
		char *awaymsg,
		int mylogin, 
		int echoback, 
		int nobeep, 
		long perms);

/* clear the entire user table */
void clear_users();

/* check the user table to see how many of them belong to a particular group */
int count_users_in_group(char *group);

/* find a slot in the user table with a particular name */
/* return that index if found, -1 otherwise */
/* case insensitive */
int find_user(char *name);

#endif /* #ifndef _ICB_USERS_H_ */
