#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>

#define BUF_SIZE 50

#define PIPE_ERR -1
#define FORK_ERR -2
#define CLOSE_ERR -3
#define READ_ERR -4
#define WRITE_ERR -5
#define USAGE_ERR -6

int write_in_pipe(int* fd, size_t size) {
	printf("LOG:in write_in_pipe\n");
	if(size != 2){
		perror("write_in_pipe - only 2-item array!\n");
		return USAGE_ERR;	
	}
		
	//printf ("LOG: writing children\n");
	printf("Enter string:\n");
	char buf[BUF_SIZE];
	
	printf("LOG: fd[0] = %d\nfd[1] = %d\n", fd[0], fd[1]);
	if(close(fd[0]) == -1) {
		perror("Error: close fd[0] in child\n");
		return CLOSE_ERR;
	}
	
	ssize_t numb_bytes = 0;
	numb_bytes = read(STDIN_FILENO, buf, BUF_SIZE);
	if(numb_bytes == -1){
		perror("ERROR: reading in child\n");
		return READ_ERR;			
	}

		//writing
	if(write(fd[1], buf, numb_bytes) == -1) {
		perror("ERROR: writing n child\n");
		return WRITE_ERR;
	}
		
	//closing last file descriptor in child process
	if(close(fd[1]) == -1) {
		perror("Error: close fd[1] in child\n");
		return CLOSE_ERR;
	}
	exit(0);
}

int read_from_pipe(int *fd, size_t size) {
	printf("LOG:in read_from_pipe\n");
	if(size != 2) {
		perror("size can only equal 2\n");
		return USAGE_ERR;
	}
	char buf[BUF_SIZE];
	ssize_t bytes_read;
	if(close(fd[1]) == -1) {
		perror("Error: close fd[1] in child\n");
		return CLOSE_ERR;
	}

	bytes_read = read(fd[0], buf, BUF_SIZE);
	if(bytes_read <= 0) {
		perror("ERROR: trying to read from pipe\n");
		return READ_ERR;
	}
	int i = 0;
	for(;i<bytes_read; ++i) {
		printf("%c", toupper(buf[i]));
	}

	if(close(fd[0]) == -1) {
		perror("Error: close fd[0] in child\n");
		return CLOSE_ERR;
	}
	exit(0);
}

int main() {

	int fd[2]; //pipefd[1] - write, pipefd[0] - read
	if(pipe(fd) == -1) {
		perror("Error pipe\n");
		return PIPE_ERR;
	}

	pid_t writer = fork();
	if(writer == -1) {
		perror("Error fork\n");
		return FORK_ERR;
	}

	if(writer == 0) {
		printf("LOG:writer pricess works\n");
		int status = write_in_pipe(fd, 2);
		if(status) {
			return status;
		}		
	}
	else if(writer > 0) {
	
		//reading process
		int writer_status;
		waitpid(writer, &writer_status, 0);
		if(WIFEXITED(writer_status)) {
			printf("LOG: exit status of writer = %d\n", writer_status);
		}		

		pid_t reader = fork();
		if(reader == -1) {
			perror("ERROR: reader fork\n");
			return FORK_ERR;
		}
		else if (reader == 0) {
			printf("LOG: reader process works\n");
			int status = read_from_pipe(fd, 2);
			if(status) {
				return status;			
			}
		}

		else if(reader > 0) {
			int reader_status;
			waitpid(reader, &reader_status, 0);
			if(WIFEXITED(reader_status)) {
				printf("LOG: exit status of reader = %d\n", reader_status);
			}
			
			if(close(fd[0]))
				return CLOSE_ERR;
			if(close(fd[1]))
				return CLOSE_ERR;
			return 0;
		} 

		
	}


}

