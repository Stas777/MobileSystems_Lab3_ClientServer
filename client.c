#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
    int sock, portno, n;
    struct 
		sockaddr_in serv_addr;
    struct hostent *server;
    
    if (argc < 3) {
       printf("usage: %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
	
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("unable to create socket: %s\n", strerror(errno));
		exit(1);
	}
    
	server = gethostbyname(argv[1]);
    if (server == NULL) {
        printf("ERROR, no such host\n");
        exit(1);
    }
	
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    
	bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
		 
    serv_addr.sin_port = htons(portno);
	
    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("ERROR connecting\n");
		exit(1);
	}
	
	char fileName[256];
	bzero(fileName, 256);
	
    printf("Please enter the file name: ");  
	
	// get file name from console
    fgets(fileName, 255, stdin);	
    n = write(sock, fileName, strlen(fileName));
    if (n < 0) {
		printf("ERROR writing to socket\n");
		exit(1);
	}
	
	// ready to receive the file
	char buf[4096];
    FILE * f;
    f = fopen ("download.dat" , "wb");
	int len;

    while ((len = recv(sock, buf, sizeof(buf), 0 )) > 0) {
        fwrite(buf, 1, len, f);
    }
	fclose (f);	 
    if (len < 0) { 
         printf("ERROR reading from socket\n");
		 exit(1);
	}
	
    printf("File has been successfully received\n");
    close(sock);
    return 0;
}