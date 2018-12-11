
#ifndef CLIENTSNFS_H
#define CLIENTSNFS_H

/*
 * FUNCTION IDENTIFIERS
 */
static char FLUSH = 2;
static char RELEASE = 3;
static char TRUNCATE = 4;
static char WRITE = 7;
static char OPENDIR = 8;
static char RELEASEDIR = 10;
static char MKDIR = 11;


#define CREATE 0
#define OPEN 1
#define GETATTR 5
#define READ 6
#define READDIR 9

static int BUFFER_SIZE = 256;
static char SEPARATOR = 255;

#endif
