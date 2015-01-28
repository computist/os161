#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

int
main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	int i = getpid();
	printf("parent pid %d \n", i);

	int j = fork();
	
	if(j == 0) {
		//child
		printf("i am the child, my pid is %d\n", getpid());
		_exit(0);
	}

	//parent
	printf("i am the parent, child pid is %d \n",j);
	printf("waiting...\n");
	int r = 0;
	j = waitpid(j, &r, 0);
	printf("child %d exited with status %d\n", j, r);

	return 0;
}
