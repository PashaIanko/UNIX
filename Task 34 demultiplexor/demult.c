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

#define BIG_SIZE 3072
#define BUF_SIZE 500
#define BACKLOG 3
#define FALSE_ 0
#define TRUE_ 1
#define FRAME_DELIM "0111111b"

char* 	echo_path = "tmp_echo_file";
typedef struct sock_pair {
	int sock_from_msg;
	int serv_sock;
} sock_pair;

typedef struct sock_id_msg {
	int client_id;
	char* msg;
	int socket;
} sock_id_msg;

typedef struct msg_len_pair {
	size_t len;
	char* msg;
} msg_len_pair;

sock_id_msg unpack_frame(msg_len_pair pair){
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
	sock_id_msg result;
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

int create_session() {
	
	int echo_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if(echo_sock < 0) {
		perror("Error while socket()\n");
		return SOCKET_ERR;
	}
	struct sockaddr_un address;	
	memset(&address, 0, sizeof(address));
	
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, echo_path);
	int addrlen = sizeof(address);
	int if_connected = connect(echo_sock, (struct sockaddr*)&address,
					 (socklen_t)addrlen);
	if(if_connected < 0) {
		perror("Connection to echo server error\n");
		unlink(echo_path);
		return CONNECT_ERR;
	}
	printf("LOG: create_session: new echo_sock = %d\n", echo_sock);
	return echo_sock;
}

int main (int argc, char* argv[]) {

	fd_set	readfds;

	int listener_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if(listener_socket < 0) {
		perror("Error while creating socket\n");
		return SOCKET_ERR;
	}

	struct sockaddr_un address;
	/*null struct*/
	memset(&address, 0, sizeof(address));

	/*address family*/
	address.sun_family = AF_UNIX;
	
	char * path = "tmp_demult";
	strcpy(address.sun_path, path); /*temp file for communication*/

	int if_binded = bind(listener_socket, (struct sockaddr*)&address, sizeof(address));
	if(if_binded < 0) {
		perror("Error while binding\b");
		unlink(path);
		return BIND_ERR;
	}
	
	printf("LOG: demultiplexor listens\n");
	int listen_code = listen(listener_socket, BACKLOG);
	if(listen_code < 0) {
		perror("Error while listening\n");
		unlink(path);
		return LISTEN_ERR;
	}

	int active_fd;
	int addrlen = sizeof(address);
	int max_sd = listener_socket;
	while(1) {

		FD_ZERO(&readfds);
		FD_SET(max_sd, &readfds);
		printf("LOG: Demultiplexor waiting for clients, listens at max_sd=%d\n", max_sd);
		active_fd = select(max_sd + 1, &readfds, NULL, NULL, NULL);	
		if(active_fd < 0 && (errno != EINTR)) {
			printf("Select error!\n");
			unlink(path);
			return SELECT_ERR;	
		}
		int incoming_socket;
		if(FD_ISSET(listener_socket, &readfds)){
			printf("LOG: Demultiplexor caught new socket\n");
			if((incoming_socket = accept(
						listener_socket, 
						(struct sockaddr*)&address,
						(socklen_t*)&addrlen)) < 0) {
				perror("Accept error\n");
				unlink(path);
				return ACCEPT_ERR;
			}
			printf("LOG: Multiplexor accepted new socket = %d\n", incoming_socket);
			if(incoming_socket > max_sd) {
				max_sd = incoming_socket;
			}
		}
		if(FD_ISSET(max_sd, &readfds)){
			printf("LOG: Demultiplexor caught data to read from forwarder\n");
			msg_len_pair result_msg = full_read(max_sd); 
	
//			printf("LOG: Demultiplexor got str:\n");
//			write(STDIN_FILENO, result_msg.msg, result_msg.len);
//			pause();
			sock_id_msg result_info = unpack_frame(result_msg);
			int client_id = result_info.client_id;
			char* msg = result_info.msg;
			printf("LOG:Demultiplexor creates session with server\n");
			int echo_sock = create_session();
			if(echo_sock == SOCKET_ERR || echo_sock == CONNECT_ERR){
				printf("Error while creating session\n");
			}
			result_info.socket = echo_sock;
			printf("LOG:result info:\nclient_id=%d\nmsg=%s\nsock with server=%d\n",
				client_id, msg, echo_sock);
		}
	}
	unlink(echo_path);
	return 0;
}

