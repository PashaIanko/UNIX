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
		perror("USAGE: ./Task\ 9 child_filename parent_filename\n");
		return USAGE_ERR;
	}
	char* filename_child = argv[1];
	char* filename_parent = argv[2];

	char* command = "cat ";
	char* child_buf = malloc(300 * sizeof(char));
	char* parent_buf = malloc(300*sizeof(char));	
	if(!child_buf || !parent_buf)
		return ALLOC_ERR;

	
	
	child_buf = command_prep(child_buf, command, filename_child);
	parent_buf = command_prep(parent_buf, command, filename_parent);

	printf("LOG:child command:%s", child_buf);	
	printf("LOG:parent command:%s", parent_buf);


	pid_t child_process = fork();
	/*child and parent execute simultaneously now*/

	if(child_process == -1){
		perror("Creating child process error\n");
		return CHILD_ERR;
	}
	else if(child_process == 0) {
		printf("LOG: child execution\n");
		/*zero is returned to the newly created
			child process -> execute system(cmd)*/
		int exec = system(child_buf);
		if(exec == -1) {
			perror("Executing command in child process error\n");
			return CHILD_EXEC_ERR;
		}
	}
	if(child_process > 0){
		/*positive value is returned to the parent process*/
		printf("LOG: parent execution\n");

		int exec = system(parent_buf);
		if(exec == -1) {
			perror("Executing command in parent process error\n");
			return PARENT_EXEC_ERR;
		}

		int status;
		pid_t wait_code = wait(&status);  /*calling process is suspended,
				waiting for the children termination */
		if(wait_code == -1) {
			perror("Waiting the child error\n");
			return WAIT_ERR;
		}
		if(WIFEXITED(status)){
			printf("Last string output by parent\n");
		}
		else {
			printf("LOG:Unexpected child termination (not by return)\n");
		}
	}

	

	free(child_buf);
	free(parent_buf);
	return 0;
}
