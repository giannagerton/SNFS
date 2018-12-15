all: serverSNFS

serverSNFS: serverSNFS.c
	gcc serverSNFS.c fileops.c -pthread -o serverSNFS
clean:
	rm -rf serverSNFS
