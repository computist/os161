#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>







int
main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;


	printf("a3 malloc \n");


	int *ar;
	ar = malloc(sizeof(int)*2000);

	if(ar==NULL){
		printf("malloc did not work \n");
		exit(1);
	}

	

	for(int i = 0; i < 2000; i++){
		ar[i] = i;
	}

	for(int i = 0; i < 2000; i++){
		printf("%d \n", ar[i]);
	}

	free(ar);

}

