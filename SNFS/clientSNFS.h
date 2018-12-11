
#ifndef CLIENTSNFS_H
#define CLIENTSNFS_H
/*
 * FUNCTION IDENTIFIERS
 */
static char CREATE = 0;
static char OPEN = 1;
static char FLUSH = 2;
static char RELEASE = 3;
static char TRUNCATE = 4;
static char READ = 6;
static char WRITE = 7;
static char OPENDIR = 8;
static char READDIR = 9;
static char RELEASEDIR = 10;
static char MKDIR = 11;

#define GETATTR 5

static int BUFFER_SIZE = 256;
static char SEPARATOR = 255;

#endif
