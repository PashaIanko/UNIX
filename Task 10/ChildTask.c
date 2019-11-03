#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define USAGE_ERR 2
#define CHILD_ERR 4
#define CHILD_EXEC_ERR 5
#define WAIT_ERR 7


int main(int argc, char * argv[]){

	if(argc == 2) {
		printf("LOG: one external argument");
	}	

	int count = 1;	
	for(count = 1; count<argc; count++){
		printf("LOG: count = %d\n", count);
		printf("%s\n", argv[count]);
	}
	return 0;
}
