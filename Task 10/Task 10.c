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

	if(argc<2){
		perror("USAGE: ./Task\ 10 ./Task Args...\n");
		return USAGE_ERR;
	}
	
	
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
		int exec = execvp(argv[1], &argv[1]);
		if(exec == -1) {
			perror("Executing child process error\n");
			return CHILD_EXEC_ERR;
		}
	}
	if(child_process > 0){
		/*positive value is returned to the parent process*/
		printf("LOG: parent execution\n");

		
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

	return 0;
}
