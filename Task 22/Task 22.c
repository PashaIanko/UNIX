#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>



#define TIME_OUT 4000
#define BUFSIZE 10

#define MALLOC_ERR -2
#define POLL_ERR -3

#define CLOSED -1

int *files = NULL;


void open_files(int argc, char* argv[]) {
     int arg_counter = 0;
     for (; arg_counter < argc; arg_counter++) {
	printf("%s\n", argv[arg_counter]);
        files[arg_counter] = open(argv[arg_counter], O_RDONLY);
     }
}

int count_closed_procs(int* fds, int files_numb) {
     printf("LOG:counting closed procs\n");
     int counter = 0;
     int closed_proc_numb = 0;
     if(fds != NULL) {
	 for(; counter<files_numb;counter++) {
	         if(fds[counter] == CLOSED){
        	        closed_proc_numb++;
        	 }
	 }
     }
     return closed_proc_numb;
}

void close_opened_files(int* fds, int numb) {
   
    if(fds != NULL) {
	int counter = 1;
        for (; counter < numb; counter++) {
        	if(files[counter] >= 0){
            		close(files[counter]);
       		}
        }
    }
}

void refresh_buf(char* ptr, size_t size) {
	int i = 0;
	for(; i< size; ++i) {
		ptr[i] = '\0';
	}
}



int main(int argc, char *argv[])
{

    if(argc <= 1) {
        printf("USAGE: ./Task\ 28 File1.txt File2.txt\n");
        return MALLOC_ERR;
    }

    char    buf[BUFSIZE] = {'\0'};
    int files_numb = argc - 1;
    files = (int*)malloc(sizeof(int) * files_numb);

    if(files == NULL){
        perror("Memory allocation error\n");
        return EXIT_FAILURE;
    }

   
    printf("File opening\n");
    open_files(files_numb, ++argv);
    
    int count_not_alive = 0;   
    while(1) {
        count_not_alive = 0;
	count_not_alive = count_closed_procs(files, files_numb);
	printf("not alive: %d\n", count_not_alive);
	if(count_not_alive >= (argc - 1)) {
	    break;
	}

	int i = 0;
	for (; i < files_numb; i++) {
	    refresh_buf(&buf, BUFSIZE);
            struct pollfd fds;
            fds.fd = files[i];
            fds.events = POLLIN; /*requested occurence - readable data*/
            fds.revents = 0; /*no returned events, just data*/
            int returned = poll(&fds, 1, TIME_OUT);
            
	    if(returned > 0 && (fds.revents & POLLIN)) {
		printf("LOG: caught data to read, fd = %d\n", fds.fd);
		
		int temp = read(fds.fd, buf, BUFSIZE);
		printf("LOG: temp = %d\n", temp);
                if(temp == 0){
                    close(files[i]);
                    files[i] = CLOSED;
                    printf("LOG:Closed %d:\n", fds.fd);
                }
                else{
                    printf("LOG: Reading from file descriptor %d: %s\n", fds.fd, buf);
                }
            }
            if(returned < 0) {
		printf("LOG: caught poll return <0\n");
				
		if(errno == EINVAL){ 
		    /*invalid*/
		    printf("LOG: errno EINVAL: fd = %d", files[i]);
		}
		
		if(POLLNVAL) { /*current fd is closed*/
			files[i] = CLOSED; /*fds.revents are not modified after return*/
		}
              
            }
        }
    }

    close_opened_files(files, argc-1);
    free(files);

    return 0;
}
