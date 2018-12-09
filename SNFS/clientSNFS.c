#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <time.h>
#include "clientSNFS.h"
#include "fileops.h"

int port;
char server_address[255];
char* mount;

static int client_getattr(const char* path, struct stat* st);

static struct fuse_operations operations = {
	.getattr = client_getattr,
};

static int get_host_ip(char* hostname, char* host_ip) {
	
}

// create a connection to the server and return the socket
static int create_connection() {
	int sockfd;
	struct sockaddr_in server_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("could not create socket");
		exit(-1);
	}
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(server_address);
	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
		perror("could not connect to server");	
		close(sockfd);
		exit(-1);
	}
	return sockfd;
}

// returns offset
int add_param_to_buffer(char* buffer, char* param, int param_size, int offset) {
	memcpy(buffer + offset, param, param_size);
	offset += param_size;
	buffer[offset] = SEPARATOR;
	return offset + 1;
}

void send_message(int sockfd, char* buffer, int count) {
	if (send(sockfd, buffer, count, 0) < 0) {
		perror("send failed");
		close(sockfd);
		exit(-1);
	}
}

int recv_message(int sockfd, char* buffer) {
	int received;
	if ((received = recv(sockfd, buffer, BUFFER_SIZE, 0)) <= 0) {
		perror("receive failed");
		close(sockfd);
		exit(-1);
	}
	return received;
}

static int client_getattr(const char* path, struct stat* st) {
	int sockfd, count, received;
	uid_t uid = getuid();
	gid_t gid = getgid();
	sockfd = create_connection();
	
	// receive data from server
	char buffer[BUFFER_SIZE];
	buffer[0] = GETATTR;
	buffer[1] = SEPARATOR;
	count = 2;
	
	count = add_param_to_buffer(buffer, (char*)path, strlen(path), count); 
	count = add_param_to_buffer(buffer, (char*)&uid, sizeof(uid), count);
	count = add_param_to_buffer(buffer, (char*)&gid, sizeof(gid), count);

	send_message(sockfd, buffer, count);
	recv_message(sockfd, buffer);	
	count = 0;

	while ((count < BUFFER_SIZE) && (count < received))  {
		//parse_message(*buffer);
		count++;
	}
	close(sockfd);
	return buffer[0];
}

int main(int argc, char *argv[]) {
	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
	if (argc < 7) {
		printf("not enough args\n");
		exit(-1);
	}
	if (strcmp(argv[1], "-port") != 0) {
		perror("need to specify port");
		printf("%s\n", argv[1]);
		exit(-1);
	}
	port = atoi(argv[2]);
	if (strcmp(argv[3], "-address") != 0) {
		perror("need to specify address");
		exit(-1);
	}
	strcpy(server_address, argv[4]);
	if (strcmp(argv[5], "-mount") != 0) {
		perror("need to specify mount point");
		exit(-1);
	}
	fuse_opt_add_arg(&args, argv[0]);
	fuse_opt_add_arg(&args, "-f");
	fuse_opt_add_arg(&args, argv[6]);

	return fuse_main(args.argc, args.argv, &operations);
}
