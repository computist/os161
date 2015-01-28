#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	char a0[] = "add2";
	char a1[2] = "2";
	char a2[2] = "3";
	char* args[4] = {a0,a1,a2,NULL}; 

	execv("add2", args);

	return 0;
}
