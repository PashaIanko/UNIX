#include <stdlib.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 1000
#define NUMB_SENDS 3
#define RECV_SIZE 1024

#define SOCKET_ERR -2
#define CONNECT_ERR -3
#define CLOSE_ERR -4


int main () {
	struct sockaddr_un address;
	int sock = 0, bytes_written = 0;

	char * msg = "I am FIRST client sending this message!";
	char * path = "tmp_file"; /*analogical to pipe - communication
					between server and client*/
	char buffer[BUF_SIZE] = {0};

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock < 0) {
		perror("Error while socket()\n");
		return SOCKET_ERR;
	}

	memset(&address, 0, sizeof(address));
	
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, path);

	int addrlen = sizeof(address);
	int if_connected = connect(sock, (struct sockaddr*)&address, (socklen_t)addrlen);
	if(if_connected < 0) {
		perror("Connection error\n");
		return CONNECT_ERR;
	}

	printf ("LOG: client sending\n");
	send(sock, msg, strlen(msg), 0);

	printf("LOG: receiving\n");
	char data[RECV_SIZE];

	ssize_t bytes_received = recv(sock, data, RECV_SIZE, 0);
	data[bytes_received] = '\0';
	printf("Client received back, msg = %s\n", data);
	
	if(close(sock) < 0){
		perror("Error while closing the client socket\n");
		return CLOSE_ERR;
	}
	return 0;
}
