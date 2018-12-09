#ifndef FILEOPS_H
#define FILEOPS_H

int getattr(struct stat* statbuf, char* path, uid_t uid, gid_t gid);

#endif
