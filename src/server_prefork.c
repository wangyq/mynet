#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/wait.h>
#include <signal.h>

//global variable 
//
int g_server_port = 2777;

void sigchld_handler(int signo)
{
	while( waitpid(-1, NULL, WNOHANG) > 0 );
}

/**
 * main entry
 *
 */

int main( int argc, const char* argv[]) 
{
	struct sockaddr_in sAddr;
	int listensock = 0, newsock = 0;
	char buffer[32] = {0};
	int x=0, result = 0, nread = 0, pid = 0, val = 0;
	
	int nchildren = 1;
	
	if( argc <2 ){
		printf("Usage: %s nchildren \n", argv[0]);
		printf("       nchildren --  number of child process to create. \n");
		exit(-1);
	}
	
	nchildren = atoi(argv[1]);
	if( nchildren <=0 ) {
		printf("error parameter! nchildren = %d \n", nchildren);
		exit(-1);
	}

	if( -1 == (listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))) {
		perror(" create socket error!");
		exit(-1);
	}

	val = 1;
	result = setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
	if( result < 0 ){
		perror("set socket option error!");
		exit(-1);
	}
	
	sAddr.sin_family = AF_INET;
	sAddr.sin_port = htons(g_server_port);
	sAddr.sin_addr.s_addr = INADDR_ANY;

	result = bind(listensock, (struct sockaddr*)&sAddr, sizeof(sAddr));
	if( result < 0 ){
		perror("bind error!");
		exit(-1);
	}

	result = listen(listensock, 5);
	if( result < 0 ){
		perror("listen error!");
		exit(-1);
	}

	//signal( SIGCHLD, sigchld_handler);	
	//show server ready info
		
	printf("Server get ready for connection at port : %d \n", g_server_port);
	printf("Process poolsize = %d \n", nchildren);

	//now we create process pool to handle client connection!
	//
	for( x=0; x<nchildren; x++ ){
		if( (pid = fork()) == 0 ) { //child process!
			while(1){
				newsock = accept(listensock, NULL,NULL);
				printf("child process %i created. \n", getpid());
				nread = recv(newsock, buffer, sizeof(buffer)-1, 0);
				buffer[nread] = '\0';
				printf("%s \n", buffer);
				send(newsock, buffer, nread, 0);
				close(newsock);
				printf("child process %i finished. \n", getpid());
				//should not exit
				//exit(0);
			}//end of while
		}//end of if
		
		//here just server can reach, so we close it.because the socket remain open in child process.
	}//end of for

	wait(NULL);
	close(listensock);  //close socket
	return 0;
}
