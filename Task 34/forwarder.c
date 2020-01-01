#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <signal.h>

#define SOCKET_ERR -2
#define BIND_ERR -3
#define LISTEN_ERR -4
#define SELECT_ERR -5
#define ACCEPT_ERR -6
#define PIPE_ERR -7
#define CONNECT_ERR -8

#define MAX_CLIENTS 10
#define BACKLOG 3
#define SEC_TIMEOUT 3
#define BUF_SIZE 500
#define BIG_SIZE 3072
#define INVALID -1
#define FALSE_ 0
#define TRUE_ 1
#define READ 0
#define WRITE 1
#define PORTION 150

typedef struct sock_pair {
	int client_sock;
	int serv_sock;
} sock_pair;

void initialize_sockets(sock_pair* ptr, size_t size) {
	if(ptr) {
		size_t i = 0;
		for(; i< size; i++) {
			ptr[i].client_sock = INVALID;
			ptr[i].serv_sock = INVALID;
		}
	}
	return;
}

int set_valid_descriptors(sock_pair* client_socket_arr, size_t arr_size, fd_set* readfds) {
	/*returns highest socket descriptor*/
	size_t i = 0;
	int sd; //socket descriptor
	int max_sd = 0;
	int serv_sd = 0;
	for (; i < arr_size; i++) {
		sd = client_socket_arr[i].client_sock;
		serv_sd = client_socket_arr[i].serv_sock;
		
		if(sd > 0) {
			FD_SET(sd, readfds);
		}
		if(serv_sd > 0) {
			FD_SET(serv_sd, readfds);
		}
		int max_sd_pair = (sd > serv_sd) ? sd : serv_sd;
		if(max_sd_pair > max_sd) {
			max_sd = max_sd_pair;
		}
	}
	if(max_sd == 0){
		printf("LOG: forwarder has no valid descriptors\n");
	}
	return max_sd;
}

int all_invalid(sock_pair* client_sock_arr, size_t size) {
	size_t i = 0;
	for(; i< size; i++) {
		if(client_sock_arr[i].client_sock != INVALID) {
			return FALSE_;	
		}
	}
	return TRUE_;
}

size_t add_new_socket(int new_sd, sock_pair* client_socket, size_t size) {
	if(client_socket != NULL) {
		size_t i = 0;
		for(; i < size; i++) {
			if(client_socket[i].client_sock == INVALID) {
				client_socket[i].client_sock = new_sd;
				return i;
			}
		}
	}
}

int fd[2];
const char* const msg = "Going to quit!";
void quit(int signal) {
	/**/
	write(fd[WRITE], msg, strlen(msg)); 

}

void close_clients(sock_pair* client_socket, size_t size) {
	size_t i = 0;
	for(; i< size; i++) {
		if(client_socket[i].client_sock != INVALID) {
			close(client_socket[i].client_sock);
		}
		if(client_socket[i].serv_sock != INVALID) {
			close(client_socket[i].serv_sock);
		}
	}
}

int string_terminated(char* buffer, size_t size) {
	size_t i = 0;
	for(;i<size; i++) {
		if(buffer[i] == '\0') {

			return TRUE_;
		}
	}
	return FALSE_;
}

char* full_read(int sd){
	char buffer[BUF_SIZE];
	char* res_str = malloc(BUF_SIZE);
	
	if(res_str == NULL) {
		return NULL;
	}
	memset(res_str, 0, strlen(res_str));
	size_t res_len = BUF_SIZE;
	size_t how_much_read = 0;
	size_t bytes_read = 0;
	
	memset(&buffer, '1', BUF_SIZE);
	char res_big_buf[BIG_SIZE] = {'\0'}; 
	while(1) {
		bytes_read = read(sd, buffer, BUF_SIZE);
		//printf("LOG: read %u bytes\n", bytes_read);

		if(bytes_read == 0) {
			//printf("LOG: read 0 bytes\n");
			break;
		}
		how_much_read += bytes_read;
		buffer[bytes_read] = '\0';
		strncat(res_big_buf, buffer, bytes_read);

		if(string_terminated(buffer, bytes_read + 1)) {
			//printf("LOG: string terminated\n");
			break;
		}
	}

	char* res_ptr = malloc(strlen(res_big_buf));
	if(res_ptr != NULL) {
		strncpy(res_ptr, res_big_buf, strlen(res_big_buf));
	}
	//printf("LOG: return str = %s from full_read\n", res_ptr);
	return res_ptr;
}



int main (int argc, char* argv[]) {

	sock_pair	client_socket[MAX_CLIENTS];
	int 		master_socket;
	fd_set		readfds;
	

	if(pipe(fd) == -1) {
		perror("Error pipe\n");
		return PIPE_ERR;
	}

	initialize_sockets(&client_socket, MAX_CLIENTS);
	master_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if(master_socket < 0) {
		perror("Error while creating socket\n");
		return SOCKET_ERR;
	}

	struct sockaddr_un address;
	memset(&address, 0, sizeof(address));
	address.sun_family = AF_UNIX;
	char * path = "tmp_file";
	strcpy(address.sun_path, path); 

	int if_binded = bind(master_socket, (struct sockaddr*)&address, sizeof(address));
	if(if_binded < 0) {
		perror("Error while binding\b");
		unlink(path);
		return BIND_ERR;
	}
	
	int listen_code = listen(master_socket, BACKLOG);
	if(listen_code < 0) {
		perror("Error while listening\n");
		unlink(path);
		return LISTEN_ERR;
	}

	/*setting handler*/
	signal (SIGINT, quit);	

	printf("Forwarder waiting for clients\n");		
	int	max_sd; 
	int	sd; 
	int	active_fds;
	int 	incoming_socket;
	int	addrlen = sizeof(address);
	ssize_t	bytes_read;
	char	buffer[BUF_SIZE];
	int 	pipe_fd = fd[READ];
	char* 	echo_path = "tmp_echo_file";

	while(1) {

		FD_ZERO(&readfds);
		FD_SET(master_socket, &readfds);
		FD_SET(pipe_fd, &readfds);
		
		max_sd = master_socket;		

		int max_sd_tmp = set_valid_descriptors(&client_socket, MAX_CLIENTS, &readfds);
		if(max_sd_tmp > max_sd) {
			max_sd = max_sd_tmp;
		}		
		active_fds = select(max_sd + 1, &readfds, NULL, NULL, NULL);
		if(active_fds < 0 && (errno != EINTR)) {
			printf("Select error!\n");
			unlink(path);
			return SELECT_ERR;	
		}
			
		if(FD_ISSET(pipe_fd, &readfds)) {
			printf("LOG: caught signal - closing clients\n");
			close_clients(&client_socket, MAX_CLIENTS);
			unlink(path);
			return 0;
		}

		if(FD_ISSET(master_socket, &readfds)){
			printf("LOG: Forwarder accept new socket\n");
			if((incoming_socket = accept(
						master_socket, 
						(struct sockaddr*)&address,
						(socklen_t*)&addrlen)) < 0) {
				perror("Accept error\n");
				unlink(path);
				return ACCEPT_ERR;
			}
			/*adding incoming socket*/
			size_t add_idx = add_new_socket(incoming_socket, &client_socket, MAX_CLIENTS);
			printf("LOG: Forwarder added new socket,\
client_socket[%u].client_sock = %d\n", add_idx, client_socket[add_idx].client_sock);
			
			/*connecting to echo serv*/
			printf("LOG: Forwarder connecting to echo serv\n");
			int echo_sock = socket(AF_UNIX, SOCK_STREAM, 0);
			if(echo_sock < 0) {
				perror("Error while socket()\n");
				return SOCKET_ERR;
			}
			struct sockaddr_un echo_address;	
			memset(&address, 0, sizeof(address));
			
			address.sun_family = AF_UNIX;
			strcpy(address.sun_path, echo_path);

			int addrlen = sizeof(address);
			int if_connected = connect(echo_sock, (struct sockaddr*)&address,
									 (socklen_t)addrlen);
			if(if_connected < 0) {
				perror("Connection to echo server error\n");
				unlink(path);
				return CONNECT_ERR;
			}
			client_socket[add_idx].serv_sock = echo_sock;
			printf("LOG:Forwarder added echo socket client_sockets[%u].serv_sock=%d\n", 
				add_idx, client_socket[add_idx].serv_sock);

		}

		size_t counter = 0;
		for(; counter < MAX_CLIENTS; counter++) {
			sd = client_socket[counter].client_sock; 
			if(FD_ISSET(sd, &readfds)) {
				char* read_msg = full_read(sd); 
				printf("LOG:Forwarder got from client_socket[%d].client_sock=%d msg = %s\n", 
									counter, sd, read_msg);
				/*echo read_msg back*/
				
				int echo_sock = client_socket[counter].serv_sock;
				printf("LOG: forwarder will forward str = %s\nto echo server socket client_socket[%d].serv_sock=%d", 
						read_msg, counter, echo_sock);
			
				ssize_t sent_bytes = send(echo_sock, read_msg, strlen(read_msg), 0);
				printf("forwarder sent to echo socket client_socket[%d].serv_sock=%d %u bytes\n", 
								counter, echo_sock, sent_bytes);

				free(read_msg);
			}

			int serv_sd = client_socket[counter].serv_sock; 
			if(FD_ISSET(serv_sd, &readfds)) {
				char* read_msg = full_read(serv_sd); 
			
				/*echo read_msg back*/
				printf("LOG: forwarder got echo from server = %s\n", read_msg);
				int client_sock = client_socket[counter].client_sock;
				printf(
				"LOG: forwarder gonna send to client_socket[%d].client_sock=%d msg = %s\n", 
							counter, client_sock, read_msg);
				ssize_t sent_bytes = send(client_sock, read_msg, strlen(read_msg), 0);
				printf("forwarder sent to client_socket[%d].client_sock=%d %u bytes\n", 
					counter, client_sock, sent_bytes);
				free(read_msg);
	
				/*closing after sending back*/
		
				printf("forwarder closed client_socket[%u].client_sock=%d and client_socket[%u].serv_sock=%d after sending back to client\n", 
					counter, client_socket[counter].client_sock,
					counter, client_socket[counter].serv_sock);
				close(client_sock);
				close(serv_sd);
				client_socket[counter].client_sock = INVALID;
				client_socket[counter].serv_sock = INVALID;
	
			}
		}

			
	}
	printf("LOG: unlinking paths\n");
	unlink(path);
	unlink(echo_path);
	return 0;
}

