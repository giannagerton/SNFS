#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include "clientSNFS.h"
#include "fileops.h"

int port;
char server_hostname[255];
char* mount;

static int client_open(const char* path, struct fuse_file_info* info);
static int client_getattr(const char* path, struct stat* st);
static int client_readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);
static int client_create(const char* path, mode_t mode, struct fuse_file_info* info);

static struct fuse_operations operations = {
	.getattr = client_getattr,
	.readdir = client_readdir,
	//.create = client_create,
	//.open = client_open,
};

static int get_host_ip(char* host_ip) {
	struct addrinfo h;
	struct addrinfo *info;
	struct addrinfo *p;
	struct sockaddr_in *server_address;
	int addr_check;

	memset(&h, 0, sizeof(h));
	h.ai_family = AF_UNSPEC;
	h.ai_socktype = SOCK_STREAM;
	
	if ((addr_check = getaddrinfo(server_hostname, "http", &h, &info)) != 0) {
		printf("getaddrinfo: %s\n", gai_strerror(addr_check));
		return 0;
	}

	for (p = info; p != NULL; p = p->ai_next) {
		server_address = (struct sockaddr_in*)p->ai_addr;
		strcpy(host_ip, inet_ntoa(server_address->sin_addr));
	}

	freeaddrinfo(info);
	return 1;
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
	char host_ip[100];
	memset(&server_addr, '\0', sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (get_host_ip(host_ip) != 1) {
		close(sockfd);
		exit(-1);
	}
	
	inet_pton(AF_INET, host_ip, &server_addr.sin_addr);
	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {	
		perror("could not connect to server");	
		printf("%s\n", server_hostname);
		printf("%d\n", port);
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

int send_message(int sockfd, char* buffer, int count) {
	int sent;
	if ((sent = send(sockfd, buffer, BUFFER_SIZE, 0)) < 0) {
		perror("send failed");
		close(sockfd);
		exit(-1);
	}
	return sent;
}

int recv_message(int sockfd, char* buffer) {
	int received;
	if ((received = recv(sockfd, buffer, BUFFER_SIZE, 0)) < 0) {
		perror("receive failed");
		close(sockfd);
		exit(-1);
	}
	return received;
}

static int client_create(const char* path, mode_t mode, struct fuse_file_info* info) {
	int sockfd, count, received;
	sockfd = create_connection();
	
	char buffer[BUFFER_SIZE];
	buffer[0] = CREATE;
	buffer[1] = SEPARATOR;
	count = 2;
	printf("create\n");
	count = add_param_to_buffer(buffer, (char*)path, strlen(path), count);
	count = add_param_to_buffer(buffer, (char*)&mode, sizeof(mode), count);

	count = send_message(sockfd, buffer, count);
	printf("sent message\n");

	while (1) {
		if (recv_message(sockfd, buffer) > 0) {
			break;
		}
	}

	count = 0;
	printf("here\n");
	close(sockfd);
	return buffer[0];

}

static int client_open(const char* path, struct fuse_file_info* info) {
	return 0;
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
	printf("getattr\n");
//:	count = add_param_to_buffer(buffer, (char*)path, strlen(path), count); 
//	count = add_param_to_buffer(buffer, (char*)&uid, sizeof(uid), count);
//	count = add_param_to_buffer(buffer, (char*)&gid, sizeof(gid), count);
	printf("hereA: %d\n", buffer[0]);
	count = send_message(sockfd, buffer, count);
	printf("sent message: %d\n", count);
	printf("sent message\n");
	while (1) {
		if (recv_message(sockfd, buffer) > 0) {
			break;
		}
	}	
	count = 0;
	printf("here: %c\n", buffer[0]);
	
	//while ((count < BUFFER_SIZE) && (count < received))  {
		//parse_message(*buffer);
	//	count++;
	//}
	printf("pathname = %s\n", path);
	get_attr(st, path, getuid(), getgid());
	close(sockfd);
	return buffer[0];
}

static int client_readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
	int sockfd, count, received;	
	sockfd = create_connection();
	char buff[BUFFER_SIZE];
	buff[0] = READDIR;
	buff[1] = SEPARATOR;
	count = 2;
	printf("readdir\n");
	count = send_message(sockfd, buff, count);
	while (1) {
		if (recv_message(sockfd, buffer) > 0) {
			break;
		}
	}
	filler(buffer, buff, NULL, 0);
	return 0;
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
	strcpy(server_hostname, argv[4]);
	if (strcmp(argv[5], "-mount") != 0) {
		perror("need to specify mount point");
		exit(-1);
	}
	fuse_opt_add_arg(&args, argv[0]);
	fuse_opt_add_arg(&args, "-f");
	fuse_opt_add_arg(&args, argv[6]);

	return fuse_main(args.argc, args.argv, &operations, NULL);
}
