#include <unistd.h>
#include <stdio.h>
#include <errno.h>
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
#include <errno.h>
#include <dirent.h>

#define MAX_CLIENTS 10

static int port;
static int serverfd;
static char mount_path[300];
typedef struct client_thread {
	int sockfd;
} client_args;

/*
int get_struct_parameter(char* buffer, int start_index, int size, void* dest) {
	memcpy(dest, buffer + start_index, size);
	printf("size = %d\n", size);
	printf("start index = %d\n", start_index);
	printf("%d\n", *&buffer[start_index]);
	return size + start_index;
}
/
int get_string_parameter(char* buffer, int start_index, char* dest) {
	int size = strlen(buffer + start_index) + 1;
	memcpy(dest, buffer + start_index, size + 1);
	return size + start_index;
}
*/

int server_getattr(char* buffer) {
	char pathname[BUFFER_SIZE];
	char final_path[BUFFER_SIZE];
	struct stat* statbuf;
	DIR* dir;
	int return_value;
	printf("getattr\n");
	strcpy(final_path, mount_path);
	get_string_parameter(buffer, 2, pathname); 
	strcat(final_path, pathname);
	bzero(buffer, BUFFER_SIZE);
	/*
	printf("%s\n", pathname);
	close(serverfd);
	exit(-1);
	if (stat(pathname, statbuf) != 0) {
		perror("getattr error");
		close(serverfd);
		return -1;
	}
	*/
	//return get_attr(statbuf, pathname, uid, gid);
	//add_param_to_buffer(buffer, (char*)statbuf, sizeof(struct stat), 0);
	if (access(final_path, F_OK) != -1) {
		return_value = 0;
		dir = opendir(final_path);
		if (dir) {
			closedir(dir);
			return_value = 1;
		}
	}
	else {
		return_value = -ENOENT;
		//dir = opendir(final_path);
		//if (dir) {
		//	closedir(dir);
		//	printf("opened directory\n");
		//	return_value = 0;
		//}
	}
	printf("return value for getattr = %d\n", return_value);
	memcpy(buffer, &return_value, sizeof(return_value));
	return 0;
}

int server_readdir(char* buffer) {
	printf("reading dir\n");
	char final_path[BUFFER_SIZE];
	char directory[BUFFER_SIZE];
	strcpy(final_path, mount_path);
	int count = 2;
	count = get_string_parameter(buffer, count, directory);
	strcat(final_path, directory);
	bzero(buffer, BUFFER_SIZE);
	DIR* open_dir = opendir(final_path);
	struct dirent* read_dir;
	count = 0;
	bzero(buffer, BUFFER_SIZE);
	while ((read_dir = readdir(open_dir)) != NULL) {
		count = add_param_to_buffer(buffer, read_dir->d_name, strlen(read_dir->d_name) + 1, count);
	}
	return 0;
}

int server_opendir(char* buffer){
	printf("opening directory\n");
	bzero(buffer, BUFFER_SIZE);
	return 0;
}

int server_create(char* buffer) {
	char file_name[BUFFER_SIZE];
	char final_path[BUFFER_SIZE];
	int fd;
	strcpy(final_path, mount_path);
	
	mode_t mode;
	int count;
	count = get_string_parameter(buffer, 2, file_name);
	count = get_struct_parameter(buffer, count, sizeof(mode_t), &mode);
	// do the things
	printf("%s\n", file_name);
	strcat(final_path, file_name);
	if ((fd = creat(final_path, mode)) < 0) {
		perror("could not create");
		return -1;
	}
	close(fd);
	return 0;
}

int server_mkdir(char* buffer){
	char dir_name[BUFFER_SIZE];
	char final_path[BUFFER_SIZE];
	
	strcpy(final_path, mount_path);

	mode_t mode;
	int count;
	count = get_string_parameter(buffer,1, dir_name);
	count = get_struct_parameter(buffer, count, sizeof(mode_t), &mode);
	printf("in mkdir\n");	
	printf("%s\n", dir_name);
	strcat(final_path, dir_name);
	if(mkdir(final_path, mode) < 0){
		perror("could not create directory");
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
	
	if ((fd = open(final_path, O_CREAT, 0777)) < 0) {
		perror("could not open");
		return -1;
	}
	close(fd);
	memcpy(buffer, &fd, sizeof(int));
	return 0;
}

// TODO: figure out what this is supposed to actually do (if anything)
int server_flush(char* buffer) {
	char file_name[BUFFER_SIZE];
	char final_path[BUFFER_SIZE];
	strcpy(final_path, mount_path);
	int count;
	count = get_string_parameter(buffer, 2, file_name);
	strcat(final_path, file_name);
	
	return 0;
}

int server_truncate(char* buffer) {
	char file_name[BUFFER_SIZE];
	char final_path[BUFFER_SIZE];
	strcpy(final_path, mount_path);
	int count;
	off_t offset;
	count = get_string_parameter(buffer, 2, file_name);
	printf("count = %d\n", count);
	printf("%s\n", file_name);
	count = get_struct_parameter(buffer, count, sizeof(off_t), &offset);
	strcat(final_path, file_name);
	printf("trunc offset = %d\n", offset);
	printf("trunc path = %s\n", final_path);
	if (truncate(final_path, offset) != 0) {
		perror("could not truncate");
		return -1;
	}
	return 0;
}

int server_read(char* buffer) {

	char file_name[BUFFER_SIZE];
	char final_path[BUFFER_SIZE];
	strcpy(final_path, mount_path);
	int count, fd, read;
	size_t size;
	off_t offset;
	count = get_string_parameter(buffer, 2, file_name);
	count = get_struct_parameter(buffer, count, sizeof(size_t), &size);
	count = get_struct_parameter(buffer, count, sizeof(off_t), &offset);
	strcat(final_path, file_name);
	bzero(buffer, BUFFER_SIZE);
	fd = open(final_path, 0);
	read = pread(fd, buffer + sizeof(int), size, offset);
	close(fd);
	add_param_to_buffer(buffer, (char*)&read, sizeof(int), 0);
	printf("%s\n", buffer + sizeof(int));
	return 0;
}

int server_write(char* buffer) {
	char write_file[BUFFER_SIZE];
	char write_file_path[BUFFER_SIZE];
	char write_buf[BUFFER_SIZE];
	strcpy(write_file_path, mount_path);
	
	int count, readfd, writefd, written;
	size_t size;
	off_t offset;
	count = get_string_parameter(buffer, 2, write_file);
	count = get_struct_parameter(buffer, count, sizeof(size_t), &size);
	count = get_struct_parameter(buffer, count, sizeof(off_t), &offset);
	count = get_string_parameter(buffer, count, write_buf);
	strcat(write_file_path, write_file);
	bzero(buffer, BUFFER_SIZE);
	printf("write file = %s\n", write_file_path);
	printf("write buf = %s\n", write_buf);
	if ((writefd = open(write_file_path, O_RDWR)) < 0) {
		perror("could not open");
		return -1;
	}
	printf("fd = %d\n", writefd);
	printf("size = %d\n", size);
	printf("offset = %d\n", offset);
	written = pwrite(writefd, write_buf, size, offset);
	
	printf("written = %d\n", written);
	add_param_to_buffer(buffer, (char*)&written, sizeof(int), 0);
	close(writefd);
	
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
		case FLUSH:
			server_flush(buffer);
			break;
		case TRUNCATE:
			server_truncate(buffer);
			break;
		case READ:
			server_read(buffer);
			break;
		case WRITE:
			server_write(buffer);
		case MKDIR:
			server_mkdir(buffer);
			break;
		default:
			printf("default\n");
			retval = -1;
			break;
	}
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
		printf("here in server\n");
		pthread_create(&thread, NULL, &thread_runner, &client_fd);
		// create thread
	}
	close(serverfd);
	return 0;
}
