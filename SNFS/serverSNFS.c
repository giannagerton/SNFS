#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include "clientSNFS.h"
#include "fileops.h"

#define MAX_CLIENTS 10

static int port;
static int serverfd;
typedef struct client_thread {
	int sockfd;
} client_args;

int server_getattr(char* buffer) {
	char* pathname;
	uid_t uid;
	gid_t gid;
	struct stat* statbuf;
	printf("getattr\n");
	// get args
	// convert pathname (if needed)
	//if (stat(pathname, statbuf) != 0) {
	//	perror("getattr error");
	//	exit(-1);
	//}
	//return get_attr(statbuf, pathname, uid, gid);
	return 0;
}

void* thread_runner(void* args) {
	client_args* thread_args;
	char buffer[BUFFER_SIZE];
	char function_id;
	int* send_back;
	int retval;
	printf("successful connection\n");
	thread_args = (client_args*)args;
	printf("sockfd = %d\n", thread_args->sockfd);
	while (1) {
		if (recv(thread_args->sockfd, buffer, BUFFER_SIZE, 0) > 0) {
			printf("got something\n");
			break;
		}
	}
	printf("got something\n");
	function_id = buffer[0];
	switch (function_id) {
		case GETATTR:
			server_getattr(buffer);
			break;
		default:
			printf("default\n");
			retval = -1;
			break;
	}
	buffer[0] = 'a';
	send(thread_args->sockfd, buffer, BUFFER_SIZE, 0);
	close(thread_args->sockfd);
}

void handle_sigint(int sig) {
	close(serverfd);
	printf("\nclosed successfuly\n");
	exit(-1);
}

int main(int argc, char *argv[]) {
	
	int val, client_fd;
	struct sockaddr_in address;
	struct sockaddr_in new_address;
	socklen_t addr_size;
	socklen_t addrlen = sizeof(address);
	
	signal(SIGINT, handle_sigint);

	if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("failed to create socket");
		exit(-1);
	}

	if (argc < 5) {
		perror("not enough arguments");
		close(serverfd);
		exit(-1);
	}
	
	if (strcmp(argv[1], "-port") == 0) {
		port = atoi(argv[2]);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(serverfd, (struct sockaddr *)&address, addrlen) < 0) {
		perror("bind failure");
		close(serverfd);
		exit(-1);
	}
	
	if (listen(serverfd, MAX_CLIENTS) < 0) {
		perror("listen failure");
		close(serverfd);
		exit(-1);
	}
	while (1) {
		if ((client_fd = accept(serverfd, (struct sockaddr*) &new_address, &addr_size)) < 0) {
			close(serverfd);
			printf("dont fail!\n");
			exit(1);
		}
		printf("%d\n", client_fd);
		pthread_t thread;
		client_args args;
		args.sockfd = client_fd;
		pthread_create(&thread, NULL, &thread_runner, &args);
		// create thread
	}
	close(serverfd);
	return 0;
}
