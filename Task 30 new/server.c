#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>


#define BUF_SIZE 1000
#define BACKLOG 1

#define SOCKET_ERR -2
#define BIND_ERR -3
#define LISTEN_ERR -4
#define ACCEPT_ERR -5

void string_toupper(char* ptr, int len){
	if(ptr != NULL && len > 0) {
		int i = 0;		
		for(; i< len; i++) {
			ptr[i] = toupper(ptr[i]);
		}
		return;
	}
	return;
}

int main() {
	int socket_fd, client_fd, read_bytes;
	struct sockaddr_un address; /*in -internet, un - connection via OS objects*/
	int addrlen = sizeof(address);
	char buffer[BUF_SIZE] = {0};
	
	
	/*AF_UNIX - unix domain, SOCK_STREAM - TCP connection
	in four steps - create, bind, listen, accept*/
	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(socket_fd < 0) {
		perror("Error while creating socket\n");
		return SOCKET_ERR;
	}
	
	/*null struct*/
	memset(&address, 0, sizeof(address));
	
	/*address family*/
	address.sun_family = AF_UNIX;
	
	char * path = "tmp_file";
	strcpy(address.sun_path, path); /*temp file for communication*/

	int if_binded = bind(socket_fd, (struct sockaddr*)&address, sizeof(address));
	if(if_binded < 0) {
		perror("Error while binding\b");
		unlink(path);
		return BIND_ERR;
	}

	int listen_code = listen(socket_fd, BACKLOG);
	if(listen_code < 0) {
		perror("Error while listening\n");
		unlink(path);
		return LISTEN_ERR;
	}

	struct sockaddr_un client_address;
	socklen_t client_address_len = sizeof(client_address);
	client_fd = accept(socket_fd, 
				(struct sockaddr*)&client_address, (socklen_t*)&client_address_len);
	if(client_fd < 0) {
		unlink(path);		
		perror("Error while accepting\n");
		return ACCEPT_ERR;
	}
	
	read_bytes = read(client_fd, buffer, BUF_SIZE);
	string_toupper(&buffer, read_bytes);
	printf("%s\n", buffer);
	unlink(path);
	

	return 0;
	
}
