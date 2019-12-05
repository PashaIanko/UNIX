#include <sys/msg.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define BUF_SIZE 32
#define ERR_MSGGET -1


struct msgbuf {
    long mtype;
    char mtext[BUF_SIZE];
};

int main() {
    int queue = msgget(getuid(), 0);
    if (queue == -1) {
        perror("Error while getting message\n");
        return ERR_MSGGET;
    }
    struct msgbuf buf;
    ssize_t res;
    while (1) {
        if ((res = msgrcv(queue, &buf, BUF_SIZE, 1, 0)) == -1){
		break;
	}
	int i = 0;
	for(;i<res; ++i) {
		buf.mtext[i] = toupper(buf.mtext[i]);
	}
        printf("%s\n", buf.mtext);
    }
    return 0;
}
