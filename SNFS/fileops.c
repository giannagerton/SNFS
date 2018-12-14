#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fileops.h"

int get_attr(struct stat *statbuf, char* path, uid_t uid, gid_t gid) {
	statbuf->st_uid = uid;
	statbuf->st_gid = gid;
	statbuf->st_atime = time(NULL);
	statbuf->st_mtime = time(NULL);
	if (strcmp(path, "/") == 0) {
		statbuf->st_mode = S_IFDIR | 0755;
		statbuf->st_nlink = 2;
	}
	else {
		statbuf->st_mode = S_IFREG | 0644;
		statbuf->st_nlink = 1;
		statbuf->st_size = 1024;
	}
	return 0;
}

int do_read(char* path, char* buffer, size_t size, off_t offset) {
	return 0;
}

int get_struct_parameter(char* buffer, int start_index, int size, void* dest) {
	memcpy(dest, buffer + start_index, size);
	return size + start_index;
}

int get_string_parameter(char* buffer, int start_index, char* dest) {
	int size = strlen(buffer + start_index) + 1;
	if ((buffer[start_index]) == SEPARATOR) {
		return -1;
	}
	memcpy(dest, buffer + start_index, size + 1);
	return size + start_index;
}

int add_param_to_buffer(char* buffer, char* param, int param_size, int offset) {
	memcpy(buffer + offset, param, param_size);
	offset += param_size;
	return offset;
}
