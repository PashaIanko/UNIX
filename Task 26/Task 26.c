#include <string.h>
#include <stdio.h> 
#include <ctype.h> 

#define BUF_SIZE 100
#define POPEN_ERR -1
#define PCLOSE_ERR -2
#define FGETS_ERR -3
#define USAGE_ERR -4

int main(int argc, char *const argv[]) {
    if(argc != 2) {
	perror("Usage: ./Task\ 26 Test StRiNg You would like to UpperCase\n");
        return USAGE_ERR;
    }
    FILE* pipe = popen("./ToUpperProcess", "w");

    if (pipe == NULL) {
        perror("Error while trying popen\n");
        return POPEN_ERR;
    }

    char* arg = argv[1];
    fprintf(pipe, "%s", argv[1]);

    if (pclose(pipe)) {
        perror("Error while trying pclose\n");
        return PCLOSE_ERR;
    }

    return 0;
}
