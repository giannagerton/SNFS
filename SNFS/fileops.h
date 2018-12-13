#ifndef FILEOPS_H
#define FILEOPS_H

#include "clientSNFS.h"

int getattr(struct stat* statbuf, char* path, uid_t uid, gid_t gid);
int get_struct_parameter(char* buffer, int start_index, int size, void* dest);
int get_string_parameter(char* buffer, int start_index, char* dest);

#endif
