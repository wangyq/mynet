#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

//here comes global variables
//
int g_server_port = 2777;

int main(int argc, const char* argv[])
{
	struct sockaddr_in sAddr;
	fd_set readset, testset;
	int listensock = 0;
	int newsock = 0;
	char buffer[32] = {0};

	int result = 0;
	int nread = 0;
	int x = 0, val = 0;

	//create socket
	if ( -1 == (listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)))  {
		perror("create socket error!");
		exit(1);
	}
	
	val = 1;
	result = setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &val,sizeof(val));
	if( result < 0 ){
		perror("set socket option error!");
		exit(1);
	}
	
	sAddr.sin_family = AF_INET;
	sAddr.sin_port = htons(g_server_port);
	sAddr.sin_addr.s_addr = INADDR_ANY;

	//now bind the address and port
	result = bind(listensock, (struct sockaddr*)&sAddr, sizeof(sAddr));
	if ( result < 0 ) {
		perror("bind error!");
		exit(1);
	}

	//print server ready info
	printf("Server get ready at port : %d \n", g_server_port);

	result = listen(listensock, 5);
	if( result < 0 ) {
		perror("listen error!");
		exit(1);
	}

	FD_ZERO(&readset);
	FD_SET(listensock, &readset);

	//now we will select the event
	//
	while(1) {
		testset = readset; 
		result = select(FD_SETSIZE, &testset, NULL,NULL,NULL);
		if( result <1 ){
			perror("select error!");
			return 0;
		}

		for( x = 0; x < FD_SETSIZE; x++ ) {
			if( FD_ISSET(x, &testset)) {
				if( x == listensock ) {
					newsock = accept(listensock, NULL,NULL);
					FD_SET(newsock, &readset);
				} else {
					nread = recv(x, buffer, sizeof(buffer)-1, 0);
					if( nread <= 0 ) {
						close(x);
						FD_CLR(x, &readset);
						printf("client on descriptor #%i disconnected. \n", x);
					} else {
						buffer[nread] = '\0';
						printf("%s \n",buffer);
						send(x, buffer,nread,0);
					}
				}//end of if
			}//end of if
		}//end of for
	}//end of while

}
