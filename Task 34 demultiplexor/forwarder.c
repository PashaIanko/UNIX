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
#define SOCKET_NUMB_LEN 10
#define NUMB_FRAME_LEN 10
#define CLIENTS_MAX_NUMB 256

typedef struct client_id_sock{
	int client_id;
	int client_sock;
} client_id_sock;

typedef struct client_id_msg{
	int client_id;
	char* msg;
} client_id_msg;

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
			printf("Forwarder has valid socket client_socket_arr[%u]=%d\n", i, sd);
			FD_SET(sd, readfds);
		}
		if(sd > max_sd) {
			max_sd = sd;
		}
	}
	if(max_sd == 0){
		printf("LOG: Forwarder has no valid descriptors\n");
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

typedef struct msg_len_pair {
	size_t len;
	char* msg;
} msg_len_pair;

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

client_id_msg unpack_frame(msg_len_pair pair){
	char*frame = pair.msg;
	char* frame_start = strchr(frame, 0x7e);
	if(frame_start == frame + strlen(frame)) {
		printf("LOG:msg_unpack:invalid frame!\n");
	}
	int client_id;
	char* offset = frame + 1 + sizeof(int);
	memcpy(&client_id, frame +1, sizeof(int));
	printf("LOG: client_id = %d\n", client_id);

	char* frame_end = strchr(offset, 0x7e);
	size_t msg_len = frame_end - offset;
	printf("LOG: msg_len = %u", msg_len);

	char* msg = malloc(msg_len*sizeof(char));
	if(msg!=NULL){
		memcpy(msg, offset, msg_len);
		printf("unpack_frame: msg = %s\n", msg);
	}
	client_id_msg result;
	result.client_id = client_id;
	result.msg = msg;
	return result;
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

char* full_read_client(int sd){
	char buffer[BUF_SIZE];
	size_t res_len = BUF_SIZE;
	size_t how_much_read = 0;
	size_t bytes_read = 0;
	
	memset(&buffer, '1', BUF_SIZE);
	char res_big_buf[BIG_SIZE] = {'\0'}; 
	while(1) {
		bytes_read = read(sd, buffer, BUF_SIZE);
		if(bytes_read == 0) {
			break;
		}
		how_much_read += bytes_read;
		buffer[bytes_read] = '\0';
		strncat(res_big_buf, buffer, bytes_read);

		if(string_terminated(buffer, bytes_read + 1)) {
			printf("LOG: string terminated\n");
			break;
		}
	}

	char* res_ptr = malloc(strlen(res_big_buf));
	if(res_ptr != NULL) {
		strncpy(res_ptr, res_big_buf, strlen(res_big_buf));
	}
	return res_ptr;
}

msg_len_pair full_read(int sd){
	char buffer[BUF_SIZE];
	size_t res_len = BUF_SIZE;
	size_t how_much_read = 0;
	size_t bytes_read = 0;
	
	memset(&buffer, '1', BUF_SIZE);
	char res_big_buf[BIG_SIZE] = {'\0'}; 
	char* buf_start = res_big_buf;
	while(1) {
		bytes_read = read(sd, buffer, BUF_SIZE);
		printf("full read: read %u bytes\n", bytes_read);
		if(bytes_read == 0) {
			break;
		}
		how_much_read += bytes_read;
		memcpy(res_big_buf, buffer, bytes_read);

		if(string_terminated(buffer, bytes_read + 1)) {
			printf("LOG: string terminated\n");
			break;
		}
		buf_start += bytes_read;
	}

	printf("LOG: full read: overall, read %u bytes\n", how_much_read);
	char* res_ptr = malloc(how_much_read);
	if(res_ptr != NULL) {
		memcpy(res_ptr, res_big_buf, how_much_read);
	}
	printf("LOG: full read: final message:\n");
	size_t i = 0;
	for(;i<how_much_read;i++){
		printf("%c\n", res_ptr[i]);
	}

	msg_len_pair result;
	result.msg = res_ptr;
	result.len = how_much_read;
	return result;
}


int find_id(client_id_sock* arr, size_t size, int socket){
	size_t i=0;
	for(;i<size;i++){
		if(arr[i].client_sock == socket){
			return arr[i].client_id;
		}
	}
}

int find_socket(client_id_sock* client_id_arr, size_t size, int client_id) {
	size_t i = 0;
	for(;i<size;i++){
		client_id_sock temp = client_id_arr[i];
		if(temp.client_id == client_id){
			return temp.client_sock;
		}
	}
	return INVALID;
}

msg_len_pair prepare_msg(char*msg, int client_id) {
	if(client_id<0 || msg == NULL) {
		msg_len_pair null;
		null.msg = NULL;
		null.len = 0;
		return null;
	}
	char * buf = malloc(sizeof(char)*BIG_SIZE);
	size_t i = 0;
	for (; i<BIG_SIZE;i++){
		buf[i] = '\0';
	}
	
	
	buf[0] = 0x7e;
	printf("LOG: before adding int buf=%s\n", buf);
	char* temp = memcpy(buf + 1, &client_id, sizeof(int));
	printf("LOG: after adding int buf = %s, buf[0]=%c\n", buf, buf[0]);	

	strcat(&buf[5], msg);
	buf[1+sizeof(int) + strlen(msg)] = 0x7e;
	//printf("final frame, buf = %s\n", buf);

	for(i=0;i<2+sizeof(int) + strlen(msg);++i){
		printf("%c\n", buf[i]);
	}

	size_t msg_len=2+sizeof(int) + strlen(msg);
	printf("strlen(msg) = %u, msg_len = %u\n", strlen(msg), msg_len);
	//free(msg);
	struct msg_len_pair res;
	res.len = msg_len;
	res.msg = buf;
	return res;
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

	int	max_sd; 
	int	sd; 
	int	active_fds;
	int 	incoming_socket;
	int	addrlen = sizeof(address);
	ssize_t	bytes_read;
	char	buffer[BUF_SIZE];
	int 	pipe_fd = fd[READ];
	client_id_sock client_id_arr[MAX_CLIENTS];
	/*connecting to demultiplexor*/
	printf("LOG:forwarder connects to demultiplexor\n");
	int demult_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if(demult_sock < 0) {
		perror("Error while socket()\n");
		return SOCKET_ERR;
	}
	char * demult_path = "tmp_demult";
	struct sockaddr_un demult_address; 
	memset(&demult_address, 0, sizeof(address));

	demult_address.sun_family = AF_UNIX;
	strcpy(demult_address.sun_path, demult_path);
	int demult_addrlen = sizeof(demult_address);
	int if_connected = connect(demult_sock, (struct sockaddr*)&demult_address, (socklen_t)demult_addrlen);
	if(if_connected < 0) {
		perror("Connection to demultiplexor error\n");
		return CONNECT_ERR;
	}
	
	printf("Forwarder: successful connection to demultipl., demult_sock=%d\n", demult_sock);
	static int client_counter = 0;
	while(1) {

		FD_ZERO(&readfds);
		FD_SET(master_socket, &readfds);
		FD_SET(pipe_fd, &readfds);
		FD_SET(demult_sock, &readfds);
		
		max_sd = master_socket;		

		/*set valid socket descriptors*/
		int max_sd_tmp = set_valid_descriptors(&client_socket, MAX_CLIENTS, &readfds);
		if(max_sd_tmp > max_sd) {
			max_sd = max_sd_tmp;
		}	
		printf("LOG: Forwarder waiting for clients\n");	
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
			printf("LOG: Forwarder caught new socket\n"); /*knocked to server*/
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
			printf("LOG: Forwarder added new sock, client_socket[%u]=%d\n",
						add_idx, client_socket[add_idx]);
			
			client_id_arr[client_counter].client_id = client_counter;
			client_id_arr[client_counter].client_sock = incoming_socket;
			client_counter++;
		}

		if(FD_ISSET(demult_sock, &readfds)){
			printf("LOG: Forwarder caught message from demultiplexor\n"); 
			msg_len_pair read_msg = full_read(demult_sock);
			client_id_msg result_info = unpack_frame(read_msg);
			printf("Client id = %d", result_info.client_id);
			int client_sock = find_socket(client_id_arr, MAX_CLIENTS, result_info.client_id);
			printf("Forwarder will send msg=%s to client_sock=%d, bytes=%u\n",
				result_info.msg, client_sock, strlen(result_info.msg));
			ssize_t bytes_sent = send(client_sock, result_info.msg, 
								strlen(result_info.msg),0);
			printf("Forwarder sent %u bytes\n", bytes_sent);
			pause();
		}


		size_t counter = 0;
		for(; counter < MAX_CLIENTS; counter++) {
			sd = client_socket[counter]; 
			if(FD_ISSET(sd, &readfds)) {
				printf("LOG: Forwarder got data to read from client_sock[%u]=%d\n",
						counter, sd);
//				msg_len_pair read_res = full_read(sd);
				char* read_msg = full_read_client(sd); 
			

				printf("Forwarder will pass str = %s\nto demult_sock=%d,\n", 
									read_msg, demult_sock);
				
				int client_id = find_id(&client_id_arr, MAX_CLIENTS, sd);
				
				msg_len_pair res = prepare_msg(read_msg, client_id);
				
				printf("Gonna send %u bytes\n", res.len);
				write(STDIN_FILENO, res.msg, res.len);
				ssize_t sent_bytes = send(demult_sock, res.msg, res.len, 0);
				printf("Forwarder sent %u bytes\n from %u", sent_bytes, res.len);
				//close(sd);
				//client_socket[counter] = INVALID;
				free(read_msg);
			}
		}

			
	}
	unlink(path);
	return 0;
}

