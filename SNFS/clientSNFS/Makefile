all: clientSNFS

clientSNFS: clientSNFS.c fileops.c
	gcc clientSNFS.c fileops.c -o clientSNFS `pkg-config fuse --cflags --libs`

clean:
	rm -rf clientSNFS
	fusermount -u /tmp/fuse
