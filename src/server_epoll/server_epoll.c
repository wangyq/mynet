#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_EVENTS 5
#define MAX_BUF    32
//here comes global variables
//
int g_server_port = 2777;
int epfd = 0;

/**
 * init epoll instance
 * return value:
 *        0 - success
 *        other - failed
 */
int init_epoll()
{
	int ret = 0;
	epfd = epoll_create(MAX_EVENTS);
	if( epfd == -1 ){
		perror("epoll created error!");
		ret = -1;
	}
	return ret;
}
int handle_accept(int fd)
{
	struct epoll_event ev;

	int newsock = accept(fd,NULL,NULL);
	if( newsock == -1 ){
		perror("accept error");
		return -1;
	}
	printf("Client accept with descriptor #%d\n." ,newsock);
	ev.data.fd = newsock;
	ev.events = EPOLLIN;
	if( -1 == epoll_ctl(epfd, EPOLL_CTL_ADD, newsock, &ev) ){
		perror("epoll_ctl error!");
		return -1;
	}
	return 0;
}
int handle_read(int fd)
{
	int nread = 0;
	char  buf[MAX_BUF+1] = {0};
	struct epoll_event ev;

	nread = recv(fd, buf, MAX_BUF, 0);
	if( nread <=0 ){
		close(fd);
		//epoll_ctl(epfd, EPOLL_CTL_DEL,fd,NULL); //close will automatically remove it from epoll instance!
		printf("can not read more! ");
	} else {
		buf[nread] = '\0';
		printf("%s \n",buf);
		//prepare to write!
		ev.data.fd = fd;
		ev.events = EPOLLOUT;
		if( -1 == epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev)){
			perror("epoll_ctl ctl_mod error");
			return -1;
		}
	}
	return 0;
}
int handle_write(int fd)
{
	int nwrite = 0;
	char buf[MAX_BUF+1] = {0};
	snprintf(buf, MAX_BUF,"hello, client #%i",fd);

	nwrite = send(fd, buf, sizeof(buf), 0);
	return 0;
}

int main(int argc, const char* argv[])
{
	struct sockaddr_in sAddr;
	struct epoll_event ev;
	struct epoll_event evlist[MAX_EVENTS];
	int ready, i;

	int listensock = 0;

	int result=0, val = 0;

	//init epoll
	if( init_epoll() ){
		exit(-1);
	}
	printf("epoll instance created success! \n");

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

	ev.events = EPOLLIN;
        ev.data.fd = listensock;
        if( epoll_ctl(epfd, EPOLL_CTL_ADD, listensock, &ev) == -1 ){
		perror("epoll_ctl add error!");
	        exit(-1);
	}

	//now we will epoll_wait the event
	//
	while(1) {
		//Fetch up to MAX_EVENTS items from the ready list
                //printf("About to epoll_wait() \n");
                ready = epoll_wait(epfd, evlist, MAX_EVENTS, -1);
		if( ready == -1 ){
			if( errno == EINTR ){
				continue;
			} else {
				perror("epoll_wait error!");
				exit(-1);
			}

		}

		for(i=0;i<ready;i++){
		/*	printf("fd=%d, events: %s%s%s%s \n",evlist[i].data.fd,
				(evlist[i].events & EPOLLIN)? "EPOLLIN ":"",
				(evlist[i].events & EPOLLOUT)? "EPOLLOUT ":"",
				(evlist[i].events & EPOLLHUP)? "EPOLLHUP ":"",
				(evlist[i].events & EPOLLERR)? "EPOLLERR ":"");
		*/
			//deal with events
			if( evlist[i].events & EPOLLIN ){
				if( evlist[i].data.fd == listensock ){
					handle_accept(evlist[i].data.fd);
				} else {
					handle_read(evlist[i].data.fd);
				}

			}
			else if( evlist[i].events & EPOLLOUT){
				handle_write(evlist[i].data.fd);
			}
			else if( evlist[i].events &(EPOLLHUP|EPOLLERR)){
				printf("Client on descriptor #%d disconnetcted. \n", evlist[i].data.fd);

				if( close( evlist[i].data.fd ) == -1 ){
					perror("close fd error!");
					exit(-1);
				}
			}
		}//end of for ready
	}//end of while

	return 0;

}
