#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define USAGE_ERR 2
#define ALLOC_ERR 3
#define CHILD_ERR 4
#define CHILD_EXEC_ERR 5
#define PARENT_EXEC_ERR 6
#define WAIT_ERR 7


char* command_prep(char* buf, char* command, char* filename){
	if(!buf)
		return NULL;

	buf = strcat(buf, command);
	buf = strcat(buf, filename);
	return buf;
}

int main(int argc, char * argv[]){

	if(argc!=3){


	

	free(child_buf);
	free(parent_buf);
	return 0;
}
