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

void test_env(const char * arg){
	
	
	return 0;
}
