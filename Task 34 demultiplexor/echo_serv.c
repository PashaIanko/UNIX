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

void initialize_sockets(int* ptr, size_t size) {
	if(ptr) {
		size_t i = 0;
		for(; i< size; i++) {
			ptr[i] = INVALID;
		}
	}
	return;
}

int set_valid_descriptors(int* client_socket_arr, size_t arr_size, fd_set* readfds) {
	/*returns highest socket descriptor*/
	size_t i = 0;
	int sd; //socket descriptor
	int max_sd = 0;
	for (; i < arr_size; i++) {
		sd = client_socket_arr[i];
		
		if(sd > 0) {
			printf("Echo serv has valid socket client_socket_arr[%u]=%d", i, sd);
			FD_SET(sd, readfds);
		}
		if(sd > max_sd) {
			max_sd = sd;
		}
	}
	if(max_sd == 0){
		printf("LOG: echo_serv has no valid descriptors\n");
	}
	return max_sd;
}

int all_invalid(int* client_sock_arr, size_t size) {
	size_t i = 0;
	for(; i< size; i++) {
		if(client_sock_arr[i] != INVALID) {
			return FALSE_;	
		}
	}
	return TRUE_;
}

size_t add_new_socket(int new_sd, int* client_socket, size_t size) {
	if(client_socket != NULL) {
		size_t i = 0;
		for(; i < size; i++) {
			if(client_socket[i] == INVALID) {
				client_socket[i] = new_sd;
				return i;
			}
		}
	}
	return (size_t)-1;
}

int fd[2];
const char* const msg = "Going to quit!";
void quit(int signal) {
	/**/
	write(fd[WRITE], msg, strlen(msg)); 

}

void close_clients(int* client_socket, size_t size) {
	size_t i = 0;
	for(; i< size; i++) {
		if(client_socket[i] != INVALID) {
			close(client_socket[i]);
		}
	}
}

int string_terminated(char* buffer, size_t size) {
	size_t i = 0;
	for(;i<size; i++) {
		if(buffer[i] == '\0') {
			printf("%c\n", buffer[i]);
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

	int	client_socket[MAX_CLIENTS];
	int 	master_socket;
	fd_set	readfds;
	

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
	
	char * path = "tmp_echo_file";
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

	printf("Echo server waiting for clients\n");		
	int	max_sd; 
	int	sd; 
	int	active_fds;
	int 	incoming_socket;
	int	addrlen = sizeof(address);
	ssize_t	bytes_read;
	char	buffer[BUF_SIZE];
	int 	pipe_fd = fd[READ];


	while(1) {

		FD_ZERO(&readfds);
		FD_SET(master_socket, &readfds);
		FD_SET(pipe_fd, &readfds);
		
		max_sd = master_socket;		

		/*set valid socket descriptors*/
		int max_sd_tmp = set_valid_descriptors(&client_socket, MAX_CLIENTS, &readfds);
		if(max_sd_tmp > max_sd) {
			max_sd = max_sd_tmp;
		}	
		printf("LOG: echo_serv waiting for clients");	
		active_fds = select(max_sd + 1, &readfds, NULL, NULL, NULL);
		if(active_fds < 0 && (errno != EINTR)) {
			printf("Select error!\n");
			unlink(path);
			return SELECT_ERR;	
		}
			
		if(FD_ISSET(pipe_fd, &readfds)) {
			close_clients(&client_socket, MAX_CLIENTS);
			unlink(path);
			return 0;
		}

		if(FD_ISSET(master_socket, &readfds)){
			printf("LOG: Echo server caught new socket\n"); /*knocked to server*/
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
			printf("LOG: Echo ser added new sock, client_socket[%u]=%d\n",
						add_idx, client_socket[add_idx]);
		}


		size_t counter = 0;
		for(; counter < MAX_CLIENTS; counter++) {
			sd = client_socket[counter]; 
			if(FD_ISSET(sd, &readfds)) {
				printf("LOG: echo serv got data to read from client_sock[%u]=%d\n",
						counter, sd);
				char* read_msg = full_read(sd); 
			
				/*echo read_msg back*/
				printf("Echo server will echo str = %s\n to sd = %d\n", read_msg, sd);
				ssize_t sent_bytes = send(sd, read_msg, strlen(read_msg), 0);
				printf("Echo server sent %u bytes\n", sent_bytes);
				close(sd);
				client_socket[counter] = INVALID;
				free(read_msg);
			}
		}

			
	}
	unlink(path);
	return 0;
}

