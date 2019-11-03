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
#define SETENV_ERR 8
#define EXECVPE_ERR 9 

extern char **environ;

int set_environ(char * const envp[]){
	//environ = NULL;
	clearenv();
	while(*envp != NULL){
		
		printf("LOG:SET_ENVIRON: envp = %s\n", *envp);
		int i = putenv(*envp);
		if(i != 0) {
			return SETENV_ERR;
		}
		*envp++;
	}
	return 0;
}

int execvpe_(const char* dir, char * const argv[], char * const envp[]) {
	printf("LOG: %s\n", envp[1]);	
	printf("LOG: %s\n", envp[2]);
	int set_code = set_environ(envp);
	execv(dir, argv);
}


int main(int argc, char * argv[]){

	if(argc<2){
		perror("USAGE: ./Task\ 11 ./Child\n");
		return USAGE_ERR;
	}
	
	
	pid_t child_process = fork();
	/*child and parent execute simultaneously now*/

	if(child_process == -1){
		perror("Creating child process error\n");
		return CHILD_ERR;
	}
	else if(child_process == 0) {
		//printf("LOG: child execution in parent process\n");
		char * const envp[] = {"PATH=/pavel", "MYVAR=myvar"};
		int exec = execvpe_(argv[1], &argv[1], envp);
		if(exec != 0) {
			perror("Execvpe error\n");
			return EXECVPE_ERR;
		}
	}
	if(child_process > 0){
		/*positive value is returned to the parent process*/
				
		int status;
		pid_t wait_code = wait(&status);  /*calling process is suspended,
				waiting for the children termination */
		if(wait_code == -1) {
			perror("Waiting the child error\n");
			return WAIT_ERR;
		}
		if(WIFEXITED(status)){
			printf("child process exited successfully\n");
		}
		else {
			printf("LOG:Unexpected child termination (not by return)\n");
		}
	}

	return 0;
}
