all: clean server_pfs client_pfs

server_pfs: server_PFS.c
	gcc -Wall server_PFS.c -o server_pfs

client_pfs: client_PFS.c
	gcc -Wall client_PFS.c -o client_pfs

clean:
	rm -f server_pfs client_pfs
