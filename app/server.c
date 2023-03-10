#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define RESP_SIMPLE_STRING '+'
#define RESP_ERROR '-'
#define RESP_INTEGER ':'
#define RESP_BULK_STRING '$'
#define RESP_ARRAY '*'

void send_resp_string(int client_fd, const char *msg) {
    char resp[1024];
    snprintf(resp, sizeof(resp), "%c%s\r\n", RESP_SIMPLE_STRING, msg);
    send(client_fd, resp, strlen(resp), 0);
}

void *client_handler(void *arg) {
	int client_fd = *(int *)arg;

	for (;;) {
		char buf[1024];
		int len = recv(client_fd, buf, sizeof(buf), 0);
		if (len <= 0) {
			printf("client %d disconnected\n", client_fd);
			break;
		}
		
		buf[len] = '\0';

		send_resp_string(client_fd, "PONG");
	}
	close (client_fd);
	pthread_exit(NULL);
}


int main() {
	// Disable output buffering
	setbuf(stdout, NULL);

	printf("Logs from your program will appear here!\n");
	
	int server_fd, client_fd, client_addr_len;
	struct sockaddr_in client_addr;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEPORT failed: %s \n", strerror(errno));
		return 1;
	}
	
	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(6379),
									 .sin_addr = { htonl(INADDR_ANY) },
									};
	
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	
	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}
	

	for (;;) {
		client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
		printf("Client connected\n");

		pthread_t tid;
		if (pthread_create(&tid, NULL, client_handler, &client_fd) != 0) {
			printf("Thread creation failed: %s \n", strerror(errno));
			return 1;
		}
		pthread_detach(tid);
	}
	close(server_fd);
	return 0;
}