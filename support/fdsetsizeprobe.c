#include <sys/types.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
	printf("%d", FD_SETSIZE);
	return(0);
}
