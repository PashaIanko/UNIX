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
	
	while(1) {
		bytes_read = read(sd, buffer, BUF_SIZE);
		printf("LOG: read %u bytes\n", bytes_read);
		if(bytes_read == 0) {
			break;
		}
		how_much_read += bytes_read;
		
		if(how_much_read > res_len)
		{
			printf("inside realloc\n");
			res_len = res_len + PORTION;
			res_str = realloc(res_str, res_len);
		}
		buffer[bytes_read] = '\0';
		strncat(res_str, &buffer, bytes_read);
		printf("res_str = %s\n", res_str);
		if(bytes_read <= BUF_SIZE)
			break;
	}
	return res_str;
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
	/*null struct*/
	memset(&address, 0, sizeof(address));

	/*address family*/
	address.sun_family = AF_UNIX;
	
	char * path = "tmp_file";
	strcpy(address.sun_path, path); /*temp file for communication*/

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
	int	max_sd; //for the highest fd (for select() argument)
	int	sd; //socket_descriptor
	int	active_fds;
	int 	incoming_socket;
	int	addrlen = sizeof(address);
	struct 	timeval t_wait;
	ssize_t	bytes_read;
	char	buffer[BUF_SIZE];
	int 	pipe_fd = fd[READ];

	//printf("LOG: pipe_fd = %d\nmaster socket = %d\n", pipe_fd, master_socket);


	while(1) {

		FD_ZERO(&readfds);
		FD_SET(master_socket, &readfds);
		FD_SET(pipe_fd, &readfds);
		
		max_sd = master_socket;		

		t_wait.tv_sec = SEC_TIMEOUT; 
		t_wait.tv_usec = 0;

		/*set valid socket descriptors*/
		int max_sd_tmp = set_valid_descriptors(&client_socket, MAX_CLIENTS, &readfds);
		if(max_sd_tmp > max_sd) {
			max_sd = max_sd_tmp;
		}		
		active_fds = select(max_sd + 1, &readfds, NULL, NULL, NULL/*&t_wait*/);
		if(active_fds < 0 && (errno != EINTR)) {
			printf("Select error!\n");
			unlink(path);
			return SELECT_ERR;	
		}
			
		if(FD_ISSET(pipe_fd, &readfds)) {
			//printf("LOG: caught signal - closing clients\n");
			close_clients(&client_socket, MAX_CLIENTS);
			unlink(path);
			return 0;
		}

		if(FD_ISSET(master_socket, &readfds)){
			printf("LOG: Forwarder accept new socket\n"); /*knocked to server*/
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

			/*connecting to echo serv*/
			printf("LOG: Connecting to echo serv\n");
			int echo_sock = socket(AF_UNIX, SOCK_STREAM, 0);
			if(echo_sock < 0) {
				perror("Error while socket()\n");
				return SOCKET_ERR;
			}
			struct sockaddr_un echo_address;	
			memset(&address, 0, sizeof(address));
			char* echo_path = "tmp_echo_file";
			address.sun_family = AF_UNIX;
			strcpy(address.sun_path, echo_path);

			int addrlen = sizeof(address);
			int if_connected = connect(echo_sock, (struct sockaddr*)&address,
									 (socklen_t)addrlen);
			if(if_connected < 0) {
				perror("Connection error\n");
				return CONNECT_ERR;
			}
			printf("LOG: adding echo socket %u\n", add_idx);
			client_socket[add_idx].serv_sock = echo_sock;
		}

		size_t counter = 0;
		for(; counter < MAX_CLIENTS; counter++) {
			sd = client_socket[counter].client_sock; //socket_id
			if(FD_ISSET(sd, &readfds)) {
				char* read_msg = full_read(sd); //allocs memory for string
			
				/*echo read_msg back*/
				printf("LOG: forwarder will forward str = %s\n", read_msg);
				int echo_sock = client_socket[counter].serv_sock;
				printf("LOG: echo_sock = %d\n", echo_sock);
				ssize_t sent_bytes = send(echo_sock, read_msg, strlen(read_msg), 0);
				printf("forwarder sent to %d echo socket %u bytes\n", 
								echo_sock, sent_bytes);

				free(read_msg);
			}

			int serv_sd = client_socket[counter].serv_sock; //socket_id
			if(FD_ISSET(serv_sd, &readfds)) {
				char* read_msg = full_read(serv_sd); //allocs memory for string
			
				/*echo read_msg back*/
				printf("LOG: forwarder got echo = %s\n", read_msg);
				int client_sock = client_socket[counter].client_sock;
				printf("LOG: forwarder gonna send to client socket = %d\n", client_sock);
				ssize_t sent_bytes = send(client_sock, read_msg, strlen(read_msg), 0);
				printf("forwarder sent to client socket %d %u bytes\n", 
					client_sock, sent_bytes);
				free(read_msg);
			}
		}

			
	}
	unlink(path);
	return 0;
}

