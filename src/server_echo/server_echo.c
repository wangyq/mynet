#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_EVENTS   10
#define MAX_ECHO_BUF 16*1024

//here comes global variables
//
int g_server_port = 2777;
int epfd = 0;

//echo data structure!
struct echo_data{
	int fd;
	char* buffer;
	char* rptr,*wptr; //pointer for read and write
	unsigned int size; //total memory capacity
};

/**
 * malloc a echo_data 
 */
struct echo_data* alloc_echo_data(int fd, int size)
{
	if( size<=0 ) return NULL;
	 
	struct echo_data* ptr = (struct echo_data*) malloc(sizeof(struct echo_data));
	if( NULL == ptr ){
		perror("mallo echo_data error!");
	} else {
		ptr->buffer = (char*)malloc(size);
		if( NULL == ptr ){
			free(ptr);
			perror("malloc echo_data size error");
			ptr = NULL;
		} else { //init element!
			ptr->fd = fd;
			ptr->size = size;
			ptr->rptr = ptr->wptr = ptr->buffer; //init pointer
		}

	}
	return ptr;
}

void free_echo_data(struct echo_data* ptr)
{
	if( NULL == ptr ) return;
	free( ptr->buffer );
	free(ptr);
}

void close_free_echo_data(struct echo_data* ptr)
{
	close(ptr->fd);
	free_echo_data(ptr);
}

/**
 * set nonblocking file descriptor
 *
 */
int setnonblocking(int sock)
{
	int opts = fcntl(sock,F_GETFL);
	if(opts<0){
		perror("fcntl(sock,GETFL)");
		return -1;
	}
	opts = opts|O_NONBLOCK;
	if(fcntl(sock,F_SETFL,opts)<0){
		perror("fcntl(sock,SETFL,opts)");
	        return -1;
	}
	return 0;
}


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
	struct echo_data* pecho_data;

	int newsock = 0;
	do{//edge-trigger
		newsock = accept(fd,NULL,NULL);
		//if( newsock <=0 ){
		if( newsock == -1 ){ //error occur
			if( (errno == EAGAIN) || (errno == EWOULDBLOCK) ) break;
			else{
				perror("accept error");
				return -1;
			}
		} else {
			printf("Client accept with descriptor #%d\n" ,newsock);
			pecho_data = alloc_echo_data(newsock, MAX_ECHO_BUF);
			if( NULL == pecho_data ){
				perror("malloc error");
				return -1;
			}

			setnonblocking(newsock); //set nonblocking 
			ev.data.ptr = pecho_data;

			ev.events = EPOLLIN|EPOLLPRI|EPOLLET; //edge-trigger
			if( -1 == epoll_ctl(epfd, EPOLL_CTL_ADD, newsock, &ev) ){
				perror("epoll_ctl error!");
				return -1;
			}
		}
	}while(1);

	return 0;
}
int handle_read(void* p)
{
	int nread = 0, be_close = 0;
	struct epoll_event ev;
	struct echo_data* ptr = (struct echo_data*)p;
	int fd = ptr->fd;
	char* buf = ptr->rptr;
	char ch;
	int unused = 0;

	//already full, return. and in write mode, to change the trigger
	if( ptr->wptr > ptr->rptr ){//unused space!
		unused = ptr->wptr - ptr->rptr -1; //one byte not used!
	} else {
		unused = ptr->size - (ptr->rptr - ptr->buffer) ;
		if( ptr->wptr == ptr->buffer ){
			unused -= 1; //last one byte not used!
		}
	}
	buf = ptr->rptr;

	while(unused>0){
		nread = recv(fd, buf, unused, 0);
		if( nread == -1 ) { //error occur
			if( (errno == EAGAIN) || (errno == EWOULDBLOCK) ) { //no more data to read!
				printf("\n");
			}
			else{
				perror("recv error!");
				//return -1;
				be_close = 1; //error!
			}
			break;
		}
		else if( nread ==0 ){//peer already close
			be_close = 1; //use to write , so do not close it!
			//printf("recv() and get peer closed!\n ");
			break;
		} else {
			ch = buf[nread-1];
			buf[nread-1] = '\0';
			printf("%s%c",buf,ch);
			buf[nread-1] = ch;
			
			//adjuct rptr
			if( ptr->rptr + nread >= ptr->buffer + ptr->size ){
				ptr->rptr = ptr->buffer;
			} else {
				ptr->rptr += nread;
			}
			buf = ptr->rptr; //
			if( ptr->wptr > ptr->rptr ){
				unused = ptr->wptr - ptr->rptr -1; //one byte not used!
			} else {
				unused = ptr->size - (ptr->rptr - ptr->buffer);
				if( ptr->wptr == ptr->buffer ){
					unused -= 1; //last one byte not used!
				}
			}
		}

	}; //end of while

	if( be_close ) {
		printf("Client with descriptor #%d disconnected! in recv() \n",fd);
		close_free_echo_data(ptr);
		//epoll_ctl(epfd, EPOLL_CTL_DEL,fd,NULL); //close will automatically remove it from epoll instance!
	} else {
		ev.data.ptr = ptr;
		ev.events = EPOLLOUT|EPOLLET;
		if( -1 == epoll_ctl(epfd, EPOLL_CTL_MOD, fd,&ev)) {
			perror("epoll_ctl mod error");
			return -1;
		}
	}
	 
	return 0;
}
int handle_write(void* p)
{
	int be_close = 0;
	struct echo_data* ptr = (struct echo_data*)p;
	struct epoll_event ev;
	int nwrite = 0, ntotal=0;
	char *ptrCur;
	int fd = ptr->fd;

	if( NULL == p ) return -1;

	ptrCur = ptr->wptr;
	if( ptr->wptr < ptr->rptr ){
		ntotal = ptr->rptr - ptr->wptr;
	} else {
		ntotal = ptr->size - (ptr->wptr - ptr->buffer);
		ntotal %= ptr->size; //avoid empty!
	}

	while(ntotal>0){
		nwrite = send(fd, ptrCur, ntotal, 0);
		if( nwrite == -1 ) {//no more to write!
			if( (errno == EAGAIN) || (errno == EWOULDBLOCK) ){
				//break;
			} else {
				perror("send error!");
				//return -1;
				be_close = 1;
			}
			break;
		}
		if( ptr->wptr + nwrite >= ptr->buffer + ptr->size ){
			ptr->wptr = ptr->buffer; //move to first
		} else {
			ptr->wptr += nwrite;
		}
		ptrCur = ptr->wptr; //move forward to write!
		if( ptr->wptr == ptr->rptr ){
			ntotal = 0;
		}else if( ptr->wptr < ptr->rptr ){
			ntotal = ptr->rptr - ptr->wptr;
		} else {
			ntotal = ptr->size - (ptr->wptr - ptr->buffer);
			ntotal %= ptr->size;
		}

	}//end of while
	if( be_close ){
		close_free_echo_data(ptr);
		printf("Client with descriptor #%d disconnected! error in send() \n",fd);
	} else {
		ev.data.ptr = ptr;
		ev.events = EPOLLIN|EPOLLPRI|EPOLLET; //edge trigger
		if( -1 == epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) ){
			perror("epoll_ctl mod read error");
			return -1;
		}
	}

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
	
	setnonblocking(listensock); //set nonblocking

	ev.events = EPOLLIN|EPOLLET; //edge-trigged
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
				printf("Get a intr signal!\n");
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
			if( evlist[i].events & (EPOLLIN|EPOLLPRI) ){
				if( evlist[i].data.fd == listensock ){
					handle_accept(evlist[i].data.fd);
				} else {
					handle_read(evlist[i].data.ptr);
				}

			}
			else if( evlist[i].events & EPOLLOUT){
				handle_write(evlist[i].data.ptr);
			}
			else if( evlist[i].events &(EPOLLHUP|EPOLLERR)){
				printf("Client on descriptor #%d disconnetcted. on epoll_wait() \n", evlist[i].data.fd);
				close_free_echo_data(evlist[i].data.ptr);//free echo data	
			}
		}//end of for ready
	}//end of while

	return 0;

}
