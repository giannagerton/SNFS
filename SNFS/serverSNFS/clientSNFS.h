
#ifndef CLIENTSNFS_H
#define CLIENTSNFS_H
/*
 * FUNCTION IDENTIFIERS
 */
static char RELEASE = 3;
static char RELEASEDIR = 10;

#define CREATE 0
#define OPEN 1
#define FLUSH 2
#define TRUNCATE 4
#define GETATTR 5
#define READ 6
#define WRITE 7
#define READDIR 9
#define MKDIR 11
#define OPENDIR 8

static int BUFFER_SIZE = 256;
static char SEPARATOR = 0;

#endif
