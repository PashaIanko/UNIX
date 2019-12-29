#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "url.h"
#include <sys/select.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>

#define CONSOLE_BUF_SIZE 10
#define BUF_SIZE 1024
#define USAGE_ERR -1
#define INVALID_SOCK -5
#define SEND_ERR -6
#define RECV_SIZE 100
#define RESPONSE_SIZE 2048
#define STRINGS_NUMB 10

void erase_string(char* ptr){
	if(ptr) {
		free(ptr);
	}
}

void free_url(Url* url){
	if(url!=NULL) {
		erase_string(url->scheme);
		erase_string(url->hostname);
		erase_string(url->port);
		erase_string(url->path);
	}
}

Url * url_parse(char *url_to_parse, size_t len) {
  
	Url *url = malloc(sizeof(Url));
	memset(url, 0, sizeof(Url));

	char* scheme_end_pos = strchr(url_to_parse, ':');
	size_t index = (size_t)(scheme_end_pos - url_to_parse);
	
	printf("LOG: index = %d\n", index);

	url->scheme = malloc(index);
	
	if(url->scheme == NULL) {
		free_url(url);
		return NULL;
	}
	strncpy(url->scheme, url_to_parse, index);
	printf("LOG: url->scheme = %s\n", url->scheme);

	url->port = malloc(strlen(DEFAULT_PORT));
	if(url->port) {
		strcpy(url->port,  DEFAULT_PORT);
		printf("LOG: url->port = %s\n", url->port);
	}



	/*parsing hostname*/
	char* tmp = scheme_end_pos + 1;
	printf("LOG: before:%s\nafter skipping //:%s\n", scheme_end_pos, tmp);
	char* hostname_end_pos = strchr(scheme_end_pos, '/');
	size_t hostname_len = (size_t)(hostname_end_pos - scheme_end_pos) - 1;

	printf("LOG: hostname len = %u\n", hostname_len);
	url->hostname = malloc(hostname_len);
	
	if(url->hostname == NULL) {
		free_url(url);
		return NULL;
	}
	strncpy(url->hostname, tmp, hostname_len);
	printf("LOG: url->hostname = %s\n", url->hostname);

	/*parsing path*/
	hostname_end_pos++;
	size_t path_len = strlen(hostname_end_pos);
	printf("LOG: path = %s\n", hostname_end_pos);

	url->path = malloc(path_len);
	if(url->path) {
		strcpy(url->path,  hostname_end_pos);
		printf("LOG: url->path = %s\n", url->path);
	}


	/*url is a particular case of uri - uniform resource identifier

		https://www.google.com/
		http - scheme
		":"
		//www.google.com - net path (absolute)
		/... - relative path (e.g. ya.ru/docs and google.com/docs)

		www.google.com - host name
	*/

    return url;
}

int init_connection(char *hostname, char *port, struct addrinfo **res)
{
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    return getaddrinfo(hostname, port, &hints, res);
}

int make_connection(struct addrinfo *res) { 
	int sockfd, status;

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	printf("LOG: sockfd = %d\n", sockfd);
	if(sockfd > 0) {
		printf("LOG:socket success\n");
		status = connect(sockfd, res->ai_addr, res->ai_addrlen);
		if(status == 0) {
			printf("LOG:connection success\n");
		}
	}
	return sockfd;
}

char * build_request(char *hostname, char *request_path) {
	char *request = NULL; 

	/*GET - HTTP method*/
	const char* GET = "GET ";
	const char* SPACE = " ";
	const char* HTTP_PROT = "HTTP/1.0\r\n";
	const char* pieces[] = 
	{
		GET,
		/*url goes here*/
		SPACE,
		HTTP_PROT
	};
	size_t req_len = strlen(request_path) + 
			strlen(GET) +
			strlen(SPACE) +
			strlen(HTTP_PROT);
	request = malloc(req_len);

	if(request != NULL) {
		memset(request, 0, strlen(request));		
		request = strcat(request, GET);
		request = strcat(request, request_path);
		request = strcat(request, SPACE);
		request = strcat(request, HTTP_PROT);

		printf("LOG: Request is :%s\n", request);
	}
	return request;
}

int make_request(int sockfd, char *hostname, char *request_path)
{
    char *request           = build_request(hostname, request_path);
    size_t bytes_sent       = 0;
    size_t total_bytes_sent = 0;
    size_t bytes_to_send    = strlen(request);

    while (1) {
        bytes_sent = send(sockfd, request, strlen(request), 0);
	if(bytes_sent < 0) {
		free(request);
		return SEND_ERR;
	}
        total_bytes_sent += bytes_sent;
        if (total_bytes_sent >= bytes_to_send) {
            break;
        }
    }

    free(request);

    return total_bytes_sent;
}

size_t count_strings(char* data, size_t size) {
	size_t i = 0;
	size_t counter = 0;
	for(; i< size; i++) {
		if(data[i] == '\n') {
			counter++;
		}
	}
	return counter;
}

size_t fetch_response(int sockfd, char *response, int response_size)
{
	size_t bytes_received;
	int status = 0;
	char data[RECV_SIZE];
	size_t how_much_received = 0;
	size_t numb_of_strings = 0;

	//fd_set	readfds;
	//FD_ZERO(&readfds);
	//FD_SET(STDIN_FILENO, &readfds);
	
	while (1) {

		printf("LOG: Inside while\n");
		bytes_received = recv(sockfd, data, RECV_SIZE, 0);
		printf("LOG: received %u bytes\n", bytes_received);
		if (bytes_received == -1) {
        		return -1;
        	} else if (bytes_received == 0) {
        		return 0;
        	}

        	if (bytes_received > 0) {
			how_much_received += bytes_received;
			data[bytes_received] = '\0';
			numb_of_strings += count_strings(data, strlen(data));
			printf("LOG: numb_of_strings = %u\n", numb_of_strings);
			printf("%s", data);
			if(numb_of_strings >= STRINGS_NUMB) {
				char console_buf[CONSOLE_BUF_SIZE] ;

				printf("\nPress space to scroll\n");
				fd_set	readfds;
				FD_ZERO(&readfds);
				FD_SET(STDIN_FILENO, &readfds);
				int changed_fds = select(STDIN_FILENO + 1, &readfds, NULL, NULL, NULL);
				if(FD_ISSET(STDIN_FILENO, &readfds)) {
					ssize_t bytes_read = read(STDIN_FILENO, console_buf, CONSOLE_BUF_SIZE);
					if(isspace(console_buf[0]) && bytes_read == 1) {
						printf("LOG: caught space\n");
						numb_of_strings = 0;
						continue;
					} 
					
				}
				
			}
		}
    }
    return strlen(response);
}

int main(int argc, char *argv[]) {

	Url *url;

	struct addrinfo *res = NULL;

	if (argc != 2) {
		perror("Usage: http <url>\n");
		return USAGE_ERR;
	}
	url = url_parse(argv[1], strlen(argv[1]));

	struct addrinfo* connect_info = NULL;
	int if_connected = init_connection(url->hostname, url->port, &connect_info);

	int sockfd = 0;
	if(!if_connected) {
		sockfd = make_connection(connect_info);
	}

	int status = 0;
	status = make_request(sockfd, url->hostname, url->path);

	if(status < 0) {
		printf("LOG: error while sending request\n");
		return SEND_ERR;
	}
	printf("LOG: status = %d\n", status);


	char* response = malloc(RESPONSE_SIZE);
	size_t received_bytes_numb = 0;
	if(response != NULL) {
		received_bytes_numb = fetch_response(sockfd, response, BUF_SIZE);
		printf("Received %d bytes\n", received_bytes_numb);
	}
	
	free_url(url);
	free(response);  	
	return 1;
}
