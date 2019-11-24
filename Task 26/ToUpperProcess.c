#include <string.h>
#include <stdio.h> 
#include <ctype.h> 
#include <stdlib.h>

#define MSG_SIZE 100
#define FGETS_ERR

char* to_upper_case(char * const msg) {
    size_t len = strlen(msg);
    size_t i = 0;
    for (; i < len; i++) {
        msg[i] = toupper(msg[i]);
    }
    return msg;
}

int main(int argc, char *const argv[]) {

    printf("LOG: In ToUpperProcess\n"); 

    char* msg = malloc(MSG_SIZE*sizeof(char));//[MSG_SIZE];
    if (msg == NULL || fgets(msg, MSG_SIZE, stdin) == NULL) {
	free(msg);
        perror("Error while getting from stdin\n");
        return FGETS_ERR;
    }

    printf("LOG: msg = %s\n", msg);
    msg = to_upper_case(msg);
    printf("%s\n", msg);
    free(msg);
    return 0;    

}
