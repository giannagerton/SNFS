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
#include <errno.h>
#include "clientSNFS.h"
#include "fileops.h"

int port;
char server_hostname[255];
char* mount;

static int client_open(const char* path, struct fuse_file_info* info);
static int client_getattr(const char* path, struct stat* st);
static int client_readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);
static int client_create(const char* path, mode_t mode, struct fuse_file_info* info);
static int client_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* info);
static int client_flush(const char* path, struct fuse_file_info* info);
static int client_truncate(const char* path, off_t offset);
static int client_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* file);
static int client_mkdir(const char* path, mode_t mode);

static struct fuse_operations operations = {
	.getattr = client_getattr,
	.readdir = client_readdir,
	.create = client_create,
	.open = client_open,
	.read = client_read,
	.flush = client_flush,
	.truncate = client_truncate,
	.write = client_write,
	.mkdir = client_mkdir,
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
	printf("in client_create\n");
	int sockfd, count, received;
	sockfd = create_connection();
	
	char buffer[BUFFER_SIZE];
	buffer[0] = CREATE;
	buffer[1] = SEPARATOR;
	count = 2;
	printf("create\n");
	count = add_param_to_buffer(buffer, (char*)path, strlen(path)+1, count);
	count = add_param_to_buffer(buffer, (char*)&mode, sizeof(mode), count);
	count = send_message(sockfd, buffer, count);
	while (1) {
		if (recv_message(sockfd, buffer) > 0) {
			break;
		}
	}
	close(sockfd);
	return 0;

}

static int client_mkdir(const char* path, mode_t mode){
	printf("in mkdir\n");
	int sockfd, count, received;
	sockfd = create_connection();
	char buffer[BUFFER_SIZE];
	buffer[0] = MKDIR;
	count = 1;
	count = add_param_to_buffer(buffer, (char*)path, strlen(path)+1, count);
	count = add_param_to_buffer(buffer, (char*)&mode, sizeof(mode), count);
	count = send_message(sockfd, buffer, count);
	while(1){
		if(recv_message(sockfd, buffer) > 0){
			break;
		}
	}
	close(sockfd);
	return 0;
}

static int client_open(const char* path, struct fuse_file_info* info) {
	printf("in client_open\n");
	int sockfd, count, received;
	sockfd = create_connection();
	
	char buffer[BUFFER_SIZE];
	buffer[0] = OPEN;
	buffer[1] = SEPARATOR;
	count = 2;
	printf("open\n");
	count = add_param_to_buffer(buffer, (char*)path, strlen(path) + 1, count);
	send_message(sockfd, buffer, count);
	while (1) {
		if (recv_message(sockfd, buffer) > 0) {
			break;
		}
	}

	close(sockfd);
	count = *(int*)buffer;
	return 0;
}


static int client_getattr(const char* path, struct stat* st) {
	printf("in client_getattr\n");
	int sockfd, count, received;
	uid_t uid = getuid();
	gid_t gid = getgid();
	sockfd = create_connection();
	
	char buffer[BUFFER_SIZE];
	//char* buffer = "this is stupid";
	buffer[0] = GETATTR;
	buffer[1] = SEPARATOR;
	count = 2;
	printf("getattr\n");
	count = add_param_to_buffer(buffer, (char*)path, strlen(path) + 1, count); 
//	count = add_param_to_buffer(buffer, (char*)&uid, sizeof(uid), count);
//	count = add_param_to_buffer(buffer, (char*)&gid, sizeof(gid), count);
	printf("%s\n", path);
	count = send_message(sockfd, buffer, count);
	while (1) {
		if (recv_message(sockfd, buffer) > 0) {
			break;
		}
	}
	//get_attr(st, (char*)path, uid, gid);
	memcpy(&received, buffer, sizeof(int));
	close(sockfd);
	printf("received = %d\n", received);
	if (received == 1) {
		received = 0;
		strcat((char*)path, "/");
	}
	get_attr(st, (char*)path, uid, gid);
	return received;
}

static int client_readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
	printf("in client_readdir\n");
	int sockfd, count, received;	
	sockfd = create_connection();
	char buff[BUFFER_SIZE];
	buff[0] = READDIR;
	buff[1] = SEPARATOR;
	count = 2;
	printf("readdir\n");
	count = send_message(sockfd, buff, count);
	while (1) {
		if (recv_message(sockfd, buff) > 0) {
			break;
		}
	}
	char file[BUFFER_SIZE];
	count = 0;
	while (((count = get_string_parameter(buff, count, file)) <= BUFFER_SIZE) && (count >= 0)) {
		filler(buffer, file, NULL, 0);
	}
	close(sockfd);
	return 0;
}

static int client_opendir(const char* path){
	printf("in client_opendir\n");
	int sockfd;
	char buff[BUFFER_SIZE];
	buff[0] = OPENDIR;
	buff[1] = SEPARATOR;
	printf("opendir\n");
//	send_message(sockfd, buff);
	return 0;
}

static int client_flush(const char* path, struct fuse_file_info* fi) {
	printf("in client_flush\n");
	int sockfd, count, received;
	sockfd = create_connection();
	char buffer[BUFFER_SIZE];
	buffer[0] = FLUSH;
	buffer[1] = SEPARATOR;
	count = 2;
	printf("flush\n");
	count = add_param_to_buffer(buffer, (char*)path, strlen(path) + 1, 2);
	count = send_message(sockfd, buffer, count);
	
	while (1) {
		if (recv_message(sockfd, buffer) > 0) {
			break;
		}
		printf("uh oh\n");
	}
	close(sockfd);
	return 0;
}

static int client_truncate(const char* path, off_t size) {
	printf("in client_truncate\n");
	int sockfd, count, received;
	sockfd = create_connection();
	char buffer[BUFFER_SIZE];
	buffer[0] = TRUNCATE;
	buffer[1] = SEPARATOR;
	count = 2;
	printf("trunc path = %s\n", path);

	count = add_param_to_buffer(buffer, (char*)path, strlen(path) + 1, count);
	printf("count = %d\n", count);
	count = add_param_to_buffer(buffer, (char*)&size, sizeof(off_t), count);
	printf("%d\n", size);
	printf("off size = %d\n", sizeof(off_t));
	printf("%d\n", *&buffer[count - sizeof(off_t)]);
	count = send_message(sockfd, buffer, count);
	while (1) {
		if (recv_message(sockfd, buffer) > 0) {
			break;
		}
	}
	close(sockfd);
	return 0;
}

static int client_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* info) {
	printf("client_read\n");
	int sockfd, count, received;
	sockfd = create_connection();
	char buffer[BUFFER_SIZE];
	buffer[0] = READ;
	buffer[1] = SEPARATOR;
	count = 2;
	printf("read\n");
	printf("size = %d\n", size);
	printf("%d\n", offset);
	
	count = add_param_to_buffer(buffer, (char*)path, strlen(path) + 1, count);
	count = add_param_to_buffer(buffer, (char*)&size, sizeof(size_t), count);
	count = add_param_to_buffer(buffer, (char*)&offset, sizeof(off_t), count);
	
	count = send_message(sockfd, buffer, count);
	printf("read2\n");
	while (1) {
		if (recv_message(sockfd, buffer) > 0) {
			break;
		}
	}
	received = *(int*)&buffer;
	printf("read3\n");
	if (received > BUFFER_SIZE) {
		received = BUFFER_SIZE;
	}
	memcpy(buf, buffer + sizeof(int), received);
	printf("read4\n");
	printf("%s\n", buffer + sizeof(int));
	close(sockfd);
	return received;
}

static int client_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
	int sockfd, count, received;
	sockfd = create_connection();
	char buffer[BUFFER_SIZE];
	buffer[0] = WRITE;
	buffer[1] = SEPARATOR;
	count = 2;
	printf("write\n");
	
	count = add_param_to_buffer(buffer, (char*)path, strlen(path) + 1, count);
	count = add_param_to_buffer(buffer, (char*)&size, sizeof(size_t), count);
	count = add_param_to_buffer(buffer, (char*)&offset, sizeof(off_t), count);
	count = add_param_to_buffer(buffer, (char*)buf, strlen(buf) + 1, count);
	
	count = send_message(sockfd, buffer, count);
	while (1) {
		if (recv_message(sockfd, buffer) > 0) {
			break;
		}
	}
	received = *(int*)&buffer;
	printf("%d\n", received);
	if (received > BUFFER_SIZE) {
		received = BUFFER_SIZE;
	}
	close(sockfd);
	return received;
}

int main(int argc, char *argv[]) {
	printf("in main\n");
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
	printf("end of main\n");
	return fuse_main(args.argc, args.argv, &operations, NULL);
}
