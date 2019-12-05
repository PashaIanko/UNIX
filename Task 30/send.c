#include <sys/msg.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>



#define BUF_SIZE 32
#define ERR_MSGGET -10
#define ERR_MSGCTL -11

struct msgbuf {
    long mtype;
    char mtext[BUF_SIZE];
};

int main() {
    int message = msgget(getuid(), IPC_CREAT | 0660);
    if (message == -1) {
        perror("Error while msgget");
        return ERR_MSGGET;
    }
    struct msgbuf buf;
    buf.mtype = 1;
    while (1) {
        scanf("%s", buf.mtext);
        if (msgsnd(message, &buf, strlen(buf.mtext) + 1, 0) == -1) {
            perror("Error while sending message\n");
            break;
        }
    }
    if (msgctl(message, IPC_RMID, NULL) == -1) {
        perror("Error while msgctl");
        return ERR_MSGCTL;
    }
    return 0;
}
