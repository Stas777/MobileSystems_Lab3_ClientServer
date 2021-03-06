#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <linux/limits.h>

void processing(int connection);

int main(int argc, char **argv)
{
	int port = 1234;           /* default port number to use */
	int sock;                  /* socket desciptor */
	int connection;            /* file descriptor for socket */	
	struct sockaddr_in addr;   /* socket parameters for bind */
	struct sockaddr_in addr1;  /* socket parameters for accept */
	int    addrlen;            /* argument to accept */
	int rc;                    /* holds return code of system calls */
	int pid;

	/* check command line arguments, handling an optional port number */
	if (argc == 2) {
		port = atoi(argv[1]);
		if (port <= 0) {
			printf("invalid port: %s\n", argv[1]);
			exit(1);
		}
	} else if (argc != 1) {
		printf("usage: %s [port]\n", argv[0]);
		exit(1);
	}

	/* create Internet domain socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		printf("unable to create socket: %s\n", strerror(errno));
		exit(1);
	}

	// clear address structure
    bzero((char *) &addr, sizeof(addr));
	addr.sin_family = AF_INET;
	// automatically be filled with current host's IP address
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	/* bind socket to the address */
	rc =  bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (rc == -1) {
		printf("unable to bind to socket: %s\n", strerror(errno));
		exit(1);
	}

	/* listen for clients on the socket */
	rc = listen(sock, 5);
	if (rc == -1) {
		printf("listen failed: %s\n", strerror(errno));
		exit(1);
	}

	while (1) {

		/* wait for a client to connect */
		connection = accept(sock, (struct sockaddr *) &addr1, &addrlen);
		if (connection == -1) {
			printf("accept failed: %s\n", strerror(errno));
			exit(1);
		}	

		/* Create child process */
		pid = fork();
        if (pid < 0) {
            printf("ERROR on fork: %s\n", strerror(errno));
			exit(1);
        }
		
		if (pid == 0) {
			// the child process will now execute this code
            processing(connection);
            exit(0);
        }
        else {
			/* close socket descriptor */
            close(connection);
        }
	} // end of while
}

//-------------------------------------------------------------------------------------
void processing(int connection) {

	char filename[PATH_MAX];   /* filename to send */
	off_t offset = 0;          /* file offset */
	int fd;                    /* file descriptor for file to send */
	struct stat stat_buf;      /* argument to fstat */
	int rc;

	/* get the file name from the client */
	rc = recv(connection, filename, sizeof(filename), 0);
	if (rc == -1) {
		printf("recv failed: %s\n", strerror(errno));
		exit(1);
	}	

	/* null terminate and strip any \r and \n from filename */
	if (filename[strlen(filename)-1] == '\n')
		filename[strlen(filename)-1] = '\0';
	if (filename[strlen(filename)-1] == '\r')
		filename[strlen(filename)-1] = '\0';

	printf("received request to send file '%s'\n", filename);

	/* open the file to be sent */
	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		printf("unable to open '%s': %s\n", filename, strerror(errno));
		exit(1);
	}

	/* get the size of the file to be sent */
	fstat(fd, &stat_buf);

	/* copy file using sendfile */
	offset = 0;
	rc = sendfile (connection, fd, &offset, stat_buf.st_size);
	if (rc == -1) {
		printf("error from sendfile: %s\n", strerror(errno));
		exit(1);
	}
	if (rc != stat_buf.st_size) {
		printf("incomplete transfer from sendfile: %d of %d bytes\n", rc, (int)stat_buf.st_size);
		exit(1);
	}	

	printf("file '%s' has been sent successfully\n", filename);

	/* close descriptor for file that was sent */
	close(fd);
} // end of processing