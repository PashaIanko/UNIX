#include <sys/types.h> 
#include <sys/wait.h> 
#include <fcntl.h> 
#include <unistd.h>  
#include <string.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <ctype.h> 
#include <errno.h>

#define STDOUT_FD 1
#define STDIN_FD 0

#define READ 0
#define WRITE 1

#define BUF_SIZE 20

#define USAGE_ERR -1
#define PIPE_ERR -2
#define READ_ERR -3
#define CLOSE_ERR -4
#define EXECLP_ERR -5
#define DUP_ERR -6
#define WAIT_ERR -7

int main(int argc, char *const argv[]) {
    if (argc != 2) {
        perror("Error, usage example: ./Task\ 27 CountEmpty.txt\n");
        return USAGE_ERR;
    }

    int fds[2];
    int fds2[2]; /*to redirect the output stream from standard (console) to
			local pipe output in the parent process*/
    if (pipe(fds2) == -1 || pipe(fds) == -1) {
        perror("Error while creating pipe\n");
        return PIPE_ERR;
    }

    if (fcntl(fds[0], F_SETFD, FD_CLOEXEC) == -1) {
        perror("Cannot set close-on-exec");
    }
    if (fcntl(fds[1], F_SETFD, FD_CLOEXEC) == -1) {
        perror("Cannot set close-on-exec");
    }
    if (fcntl(fds2[0], F_SETFD, FD_CLOEXEC) == -1) {
        perror("Cannot set close-on-exec");
    }
    if (fcntl(fds2[1], F_SETFD, FD_CLOEXEC) == -1) {
        perror("Cannot set close-on-exec");
    }

    pid_t writer = -1, reader = -1;

    if ((writer = fork()) == -1) {
        perror("Error while forking for grep\n");
    } 
    else if (writer == 0) {
	/*Stream redirection - standard output stream for the writer child 
	process is redirected to fds[WRITE] input stream*/
	if (dup2(fds[WRITE], STDOUT_FD) == -1) {
            perror("Error while redirecting the output for the writer\n");
            return DUP_ERR;
        }

	/*first arg - binary file, second arg - what it will do
	(two commands may correspond to one binary file)
	+ regular expression for the empty string (many spases + $ - end of the line)*/
        int if_terminated_not_ok = execlp("grep", "grep", argv[1], "-e", "^\\s*$", NULL);
        if(if_terminated_not_ok) { /*return only if something is wrong*/ 
		perror("Error while execlp grep\n");
		return EXECLP_ERR;
	}
        
    }


    if (writer != -1) {
        if ((reader = fork()) == -1) {
            perror("Error while fork for writer\n");
        } 
    	else if (reader == 0) {
    	    if (dup2(fds[READ], STDIN_FD) == -1) {
    	        perror("reader: cannot redirect input");
    	        return 1;
    	    }
    	    if (dup2(fds2[WRITE], STDOUT_FD) == -1) { /*now standard output
							stream for reader (console) is
							redirected to the pipe*/
    	        perror("Error while redirecting the output for the reader\n");
    	        return DUP_ERR;
    	    }
    	    execlp("wc", "wc", "-l", NULL);
    	    perror("exec wc");
    	    return 1;
    	}
    }

    if (close(fds[READ]) || close(fds[WRITE]) || close(fds2[WRITE])) {
        perror("Error while closing file\n");
	return CLOSE_ERR;
    }
  
  
    char buf[BUF_SIZE];
   /*READING the resulting number, and afterwards analyze if the children 
	terminated successfully*/
    ssize_t bytes_read = read(fds2[READ], buf, BUF_SIZE);    
    printf("LOG: bytes_read = %d\n", (int)bytes_read);
    if(bytes_read == -1) {
	perror("Error while reading the result from fds[2]\n");
	return READ_ERR;
    }


    int writer_status;
    int reader_status;
    if (writer != -1 && waitpid(writer, &writer_status, 0) == -1) {
        perror("Error waiting writer termination\n");
	return WAIT_ERR;
    }
    if (reader != -1 && waitpid(reader, &reader_status, 0) == -1) {
        perror("Error waiting reader termination\n");
	return WAIT_ERR;
    }

    if(WIFEXITED(reader_status)){
	printf("Reader terminated with status: %d\n", WEXITSTATUS(reader_status));
    }
    else if(WIFSIGNALED(reader_status)){
	printf("Reader terminated by signal, signumb: %d\n", WTERMSIG(reader_status));
    }
    if(WIFEXITED(writer_status)){
	printf("Writer terminated with status: %d\n", WEXITSTATUS(writer_status));
    }
    else if(WIFSIGNALED(writer_status)){
	printf("Writer terminated by signal, signumb: %d\n", WTERMSIG(writer_status));
    }
        
    int i = 0;
    for(i = 0; i<bytes_read; ++i)
	printf("%c", buf[i]);
//    printf("%.2s\n", buf);//"%s\n", buf);//(int)bytes_read);
    return 0;
}
