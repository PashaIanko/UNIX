#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

const int BUFFER_SIZE = 100; /*made sure that buffer > string lengths on average
				(the task is specified for short strings)*/

typedef struct{
    int fd;
    size_t lines_count;
    size_t offset[50];
    size_t size[50];
} OffsetTable;



OffsetTable* read_file(const char* name){
    OffsetTable* file = (OffsetTable*) malloc(sizeof(OffsetTable));
    if (!file) {
        return NULL;
    }
    file->fd = open(name, O_RDONLY);

    if (file->fd == -1) {
        free(file);
        return NULL;
    }

    file->lines_count = 1;
    file->offset[0] = 0;
    char buf[BUFFER_SIZE];
    size_t i = 0, line_offset = 0;
    int nbytes = read(file->fd, buf, BUFFER_SIZE);

    while (nbytes > 0) {
	const char* ch = NULL;
        for (ch = buf; ch < buf + BUFFER_SIZE; ch++)
        {
            i++;
            if (*ch == '\n') {
                file->size[file->lines_count-1] = i;
                line_offset += i;
                file->offset[file->lines_count] = line_offset;
                file->lines_count++;
                i = 0;
            }

        }
        nbytes = read(file->fd, buf, BUFFER_SIZE);
    }   

    if (nbytes == -1) {
        close(file->fd);
        free(file);
        return NULL;

    }
    return file;
}


int main(int argc, char** argv)

{
    if (argc != 2){
        printf("Wrong input please full filename\n");
        return 0;
    }

    OffsetTable* file = read_file(argv[1]);
    if (file == NULL) {
        perror("READ_FILE: unsucceeded reading\n");
        return 0;
    }
    size_t string_number;
    do {  
        printf("Line number: ");
        scanf("%d", &string_number);

        if (file->lines_count < string_number) {
            printf("Line not found\n");
            break;
        }
        else{

            size_t len = file->size[string_number-1];

	    /*setting to the beginning of the file*/ 
	    lseek(file->fd, 0, SEEK_SET);

	    char buffer[BUFFER_SIZE];
	    size_t full_offset = file->offset[string_number - 1];
	    if( lseek(file->fd, full_offset, SEEK_SET) == -1 || read(file->fd, buffer, len) == -1) {
	    	perror("Reading error");
                close(file->fd);
                free(file);
                return 1;
	    }
	    else {
		
		write(STDIN_FILENO, buffer, len);

	    }
	   
        }

    }while (string_number > 0);
    close(file->fd);
    free(file);
    return 0;
}
