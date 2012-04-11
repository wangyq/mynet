#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/wait.h>
#include <signal.h>

//global variable 
//
int g_server_port = 2777;

void* thread_proc(void*arg);

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
	pthread_t thread_id ;

	int listensock = 0, newsock = 0;
	int result = 0,  val = 0;

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
	
	while(1){
		newsock = accept(listensock, NULL,NULL);
		result = pthread_create(&thread_id, NULL, thread_proc, (void*) newsock);
		if( result !=0 ){
			printf("Cound not create thread.\n");
			continue;
		}
		pthread_detach(thread_id);
		sched_yield();
	}//end of while

	//nener reach here
	close(listensock);
	return 0;
}//end of main

void* thread_proc(void*arg)
{
	int newsock = 0;
	char buffer[32] = {0};
	int nread = 0;

	newsock = (int)arg;
	
	printf("child thread #%i with pid=%i created. \n", (int)pthread_self(),getpid());
	nread = recv(newsock, buffer, sizeof(buffer)-1, 0);
	buffer[nread] = '\0';
	printf("%s \n", buffer);
	send(newsock, buffer, nread, 0);
	close(newsock);
	printf("child thread #%i with pid=%i finished. \n", (int)pthread_self(),getpid());
	return 0;
}
