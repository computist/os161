#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

int
main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;


	char filename[] = "/file_syscall.txt";


	int file = open(filename, O_RDONLY);

	if(file < 0){
		printf("could not open file");

		return -1;
	}

	printf("\n reading \n");
	
	char read_buffer[50];

	bzero((void*) read_buffer, 50);

	int count = 14;

	int res;

	res = read(file, read_buffer, count);

	close(file);

	printf(read_buffer);

	printf("\n done \n");

}


