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
#include <fcntl.h>
#include "clientSNFS.h"
#include "fileops.h"

#define MAX_CLIENTS 10

static int port;
static int serverfd;
static char mount_path[300];
typedef struct client_thread {
	int sockfd;
} client_args;


int get_struct_parameter(char* buffer, int start_index, int size, void* dest) {
	memcpy(dest, buffer + start_index, size);
	return size + start_index;
}

int get_string_parameter(char* buffer, int start_index, char* dest) {
	int size = strlen(buffer + start_index);
	memcpy(dest, buffer + start_index, size);
	return size + start_index;
}

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
	buffer[0] = 0;
	return 0;
}

int server_readdir(char* buffer) {
	printf("reading dir\n");
	bzero(buffer, BUFFER_SIZE);
	strcpy(buffer, "we_got_this.c");
	return 0;
}

int server_create(char* buffer) {
	char file_name[BUFFER_SIZE];
	char final_path[BUFFER_SIZE];

	strcpy(final_path, mount_path);
	
	mode_t mode;
	int count;
	count = get_string_parameter(buffer, 2, file_name);
	count = get_struct_parameter(buffer, count, sizeof(mode_t), &mode);
	// do the things
	printf("%s\n", file_name);
	strcat(final_path, file_name);
	if (creat(final_path, mode) < 0) {
		perror("could not create");
		return -1;
	}
	return 0;
}

int server_open(char* buffer) {
	char file_name[BUFFER_SIZE];
	char final_path[BUFFER_SIZE];

	strcpy(final_path, mount_path);
	int count, fd;
	count = get_string_parameter(buffer, 2, file_name);
	strcat(final_path, file_name);
	printf("final path = %s\n", final_path);
	if ((fd = open(final_path, O_CREAT)) < 0) {
		perror("could not open");
		return -1;
	}
	memcpy(buffer, &fd, sizeof(int));
	return 0;
}

void* thread_runner(void* args) {
	client_args* thread_args;
	char buffer[BUFFER_SIZE];
	char function_id;
	int* send_back;
	int retval, new_socket;
	printf("successful connection\n");
	thread_args = (client_args*)args;
	printf("sockfd = %d\n", thread_args->sockfd);
	new_socket = thread_args->sockfd;
	while (1) {
		if (recv(new_socket, buffer, BUFFER_SIZE, 0) >= 0) {
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
		case CREATE:
			server_create(buffer);
			break;
		case OPEN:
			server_open(buffer);
			break;
		case READDIR:
			server_readdir(buffer);
			break;
		default:
			printf("default\n");
			retval = -1;
			break;
	}
	printf("%s\n", buffer);
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
	if (strcmp(argv[3], "-mount") == 0) {
		//strcpy(mount_path, argv[4]);
		if (realpath(argv[4], mount_path) == NULL) {
			perror("invalid path");
			close(serverfd);
			exit(-1);
		}
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
