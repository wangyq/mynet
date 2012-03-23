
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

void child_func(int childnum);

//server port to listen
int g_server_port = 2777;
//server ip addr
const char* g_server_addr = "127.0.0.1";


/**
 *
 *
 */
void usage(const char* argv[])
{
	printf("usage: %s session \n", argv[0]);
	printf("       where session is the number of client sessions. and session greater or than one!\n ");
}


int main(int argc, const char* argv[])
{
	int nchildren = 1;
	int pid = 0;
	int x = 0;
	
	if (argc <2 ) {
		usage(argv);
		exit(0);
	}	
	nchildren = atoi(argv[1]);
	
	//parameter check!
	if( nchildren <= 0 ){
		printf("invalid parameter! session = %d \n", nchildren);
		exit(1);
	}

	//now create children process
	for(x = 0; x<nchildren; x++){
		if ( (pid = fork()) == 0 ) { //child process!
			child_func(x+1);
			exit(0);
		}
	}

	// waiting all children to exit
	wait(NULL);
	return 0;
}

/**
 * child function to access to server
 *
 */

void child_func(int childnum)
{
	int data_num = 0;
	int sock = 0;
	struct sockaddr_in sAddr;
	char buffer[32] = {0};
	
	//create client socket and bind it to a local port
	memset( (void*)&sAddr, 0, sizeof(struct sockaddr_in));
	sAddr.sin_family = AF_INET;
	sAddr.sin_addr.s_addr = INADDR_ANY;
	sAddr.sin_port = 0;

	//create socket!
	if( -1 == (sock = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP)) ){
		perror("error on create socket! \n");
		return;
	}
	//bind socket to address and port
	bind(sock, (const struct sockaddr*)&sAddr, sizeof(sAddr));

	//set server addr and port number	
	sAddr.sin_addr.s_addr = inet_addr(g_server_addr);
	sAddr.sin_port = htons(g_server_port);

	if ( connect(sock, (const struct sockaddr*)&sAddr, sizeof(sAddr)) != 0 ) {
		perror("error can not connect to server!");
		return ;
	}

	snprintf(buffer, 128, "data from client #%i. ", childnum);
	
	sleep(1);
	data_num = send(sock, buffer, strlen(buffer),0);  //send to server!
	printf("child #%i send %i chars. \n", childnum, data_num);

	sleep(1);
	data_num = recv(sock, buffer, sizeof(buffer)-1, 0);
	buffer[data_num] = '\0';
	printf("child #%i received %i chars. \n", childnum,data_num);

	sleep(1);
	close(sock);
	
}

