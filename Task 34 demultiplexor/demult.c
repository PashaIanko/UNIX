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
#define CREATE_SESSION_ERR -9

#define MAX_CONNECTIONS 256
#define BIG_SIZE 3072
#define BUF_SIZE 500
#define BACKLOG 3
#define FALSE_ 0
#define TRUE_ 1
#define INVALID -1



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

int set_valid_descriptors(sock_id_msg* client_socket_arr, size_t arr_size, fd_set* readfds) {
	/*returns highest socket descriptor*/
	size_t i = 0;
	sock_id_msg sd; 
	int max_sd = 0;
	for (; i < arr_size; i++) {
		sd = client_socket_arr[i];
		int cur_sock = sd.socket;
		if(cur_sock > 0) {
			printf("Demultiplexor has valid socket arr[%u]=%d\n", i, cur_sock);
			FD_SET(cur_sock, readfds);
		}
		if(cur_sock > max_sd) {
			max_sd = cur_sock;
		}
	}
	if(max_sd == 0){
		printf("LOG: Demultiplexor has no valid descriptors\n");
	}
	return max_sd;
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

int create_session(char*path) {
	
	int echo_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if(echo_sock < 0) {
		perror("Error while socket()\n");
		return SOCKET_ERR;
	}
	struct sockaddr_un address;	
	memset(&address, 0, sizeof(address));
	
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, path);
	int addrlen = sizeof(address);
	int if_connected = connect(echo_sock, (struct sockaddr*)&address,
					 (socklen_t)addrlen);
	if(if_connected < 0) {
		perror("Connection to echo server error\n");
		unlink(path);
		return CONNECT_ERR;
	}
	printf("LOG: create_session: new sock = %d\n", echo_sock);
	return echo_sock;
}

size_t add_new_socket(sock_id_msg info_to_add, sock_id_msg* connections, size_t size){
	size_t i = 0;
	for(;i<size;i++){
		if(connections[i].socket == INVALID &&
		connections[i].client_id == INVALID &&
		connections[i].msg == NULL){
			connections[i] = info_to_add;
			return i;
		}
	}
	printf("LOG: add_new_socket - no room for clients\n");
}


void init_connections(sock_id_msg* connections, size_t size){
	size_t i =0;
	for(; i<size; i++){
		connections[i].socket = INVALID;
		connections[i].msg = NULL;
		connections[i].client_id = INVALID;		
	}
}

int find_id(sock_id_msg* connections, size_t size, int sock){
	size_t i = 0;
	for(;i<size;i++){
		sock_id_msg temp = connections[i];
		if(temp.socket == sock){
			return temp.client_id;
		}
	}
	printf("Could not find the client id\n");
	return -1;
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
	char* 	echo_path = "tmp_echo_file";
	int forwarder_socket=0;
	sock_id_msg connections[MAX_CONNECTIONS];
	init_connections(connections, MAX_CONNECTIONS);
	while(1) {

		FD_ZERO(&readfds);
		FD_SET(listener_socket, &readfds);
		
		max_sd = listener_socket;		
		/*set valid socket descriptors*/
		int max_sd_tmp = set_valid_descriptors(&connections, MAX_CONNECTIONS, &readfds);
		if(max_sd_tmp > max_sd) {
			max_sd = max_sd_tmp;
		}

		printf("LOG: Demultiplexor waiting for clients, listens at max_sd=%d\nlisten_sock=%d\n", 						max_sd, listener_socket);
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
			printf("LOG: Demultiplexor accepted new socket = %d\n", incoming_socket);
			if(incoming_socket > max_sd) {
				max_sd = incoming_socket;
			}
			sock_id_msg info_to_add;
			info_to_add.msg = NULL;
			info_to_add.socket = incoming_socket;
			info_to_add.client_id = INVALID;
			size_t add_idx = add_new_socket(info_to_add, &connections, MAX_CONNECTIONS);
			printf("LOG: demultiplexor added new client at connections[%u].socket=%d\n",
				add_idx, connections[add_idx].socket);
			forwarder_socket = incoming_socket;
		}


		size_t counter = 0;
		for(; counter < MAX_CONNECTIONS; counter++) {
			int sd = connections[counter].socket; 
			if(FD_ISSET(sd, &readfds)) {
				printf("LOG: Demultiplexor got data to read from connections[%u]=%d\n",
						counter, sd);

				if(connections[counter].msg==NULL && 
					connections[counter].client_id == INVALID){
					printf("LOG:this msg is from forwarder\n");
					//pause();

					msg_len_pair result_msg = full_read(sd);  

					sock_id_msg result_info = unpack_frame(result_msg);
					int client_id = result_info.client_id;
					char* msg = result_info.msg;
					printf("LOG:Demultiplexor creates session with server\n");
					int echo_sock = create_session(echo_path);
					if(echo_sock == SOCKET_ERR || echo_sock == CONNECT_ERR){
						printf("Error while creating session\n");
						return CREATE_SESSION_ERR;
					}
					result_info.socket = echo_sock;
					printf("Res info:\nclient_id=%d\nmsg=%s\nsock with server=%d\n",
						client_id, msg, echo_sock);
					size_t added_idx = add_new_socket(result_info, &connections, 								MAX_CONNECTIONS);
					printf(
					"Added new connection with server, connections[%u].socket=%d\n", 					
					added_idx, connections[added_idx].socket);
					printf("Demultiplexor sends to echo_sock=%d message %s\n",
											echo_sock, msg);
					ssize_t sent_bytes = send(echo_sock, msg, strlen(msg), 0);
					printf("Demultiplexor sent %u bytes\n", sent_bytes);
					//close(sd);
					//client_socket[counter] = INVALID;
					free(msg);
				}
				else{
					printf("LOG:this msg is from server\n");
					msg_len_pair result_msg = full_read(sd);  
					printf("Demult got msg=%s from server\n", result_msg.msg);
					int client_id = find_id(connections, MAX_CONNECTIONS, sd);
					printf("Demult will make frame for client_id=%d\n", client_id);
					msg_len_pair res = prepare_msg(result_msg.msg, client_id);
					ssize_t sent_bytes = send(forwarder_socket, res.msg, res.len, 0);
					printf("Demult sent %u bytes to forwarder\n", sent_bytes);
					//pause();
					//close(sd);
					connections[counter].socket = INVALID;
					connections[counter].client_id = INVALID;
					free(connections[counter].msg);
					connections[counter].msg = NULL;
				}
				
			}
		}
		
	}
	unlink(echo_path);
	return 0;
}

