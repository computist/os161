#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

int
main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;


	char filename[] = "/assignment2.txt";


	int file = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0664);

	if(file < 0){
		printf("could not open file");

		return -1;
	}

	printf("\n Writing to file &%s \n", filename);
	

	char write_buffer[] = "Eichhoernchen";

	int count = 14;

	int res;

	res = write(file, write_buffer, count);

	close(file);

	printf("\n done \n");

}


