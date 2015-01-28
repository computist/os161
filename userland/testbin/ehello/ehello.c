/* prints "Hello world!" */

#include <stdio.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;
	execv("helloworld", NULL);
	return 0;
}
