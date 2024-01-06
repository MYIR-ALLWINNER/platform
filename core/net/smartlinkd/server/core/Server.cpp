#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include "Server.h"
#include "log.h"
#include "smartlink_util.h"
//#include "Proto.h"

#include "platform.h"
#ifndef ANDROID_ENV
#include "TinaWifiNetwork.h"
#define UNIX_DOMAIN "/tmp/UNIX.domain"
#else
#include "AndroidWifiNetwork.h"
#define UNIX_DOMAIN "/data/misc/wifi/UNIX.domain"
#endif

#define MAX_FD 1
#define MAX_EVENT_NUMBER 10

#define ET 1
#define LT 0

#define IS_DATA 0
#define IS_CMD 1

#define MAX_DET_NUM 20

int socket_fd;
int epoll_fd;
int app_fd;
int proto_fd;
int proto;
const int epoll_type = LT;

static int write_inner(int fd, void *buf, size_t len)
{
    size_t bytes;
    int bytes_w = 0;

write_again:
    bytes = write(fd,buf,len);

    if(bytes <= 0)
    {
	if(bytes == 0)
	{
	    printf("sever %s: peer closed\n",__func__);
	    return bytes; //return bytes having write
	}

	if(errno == EINTR)
	{
	    printf("=============sever %s:%s==============\n",__func__,strerror(errno));
	    goto write_again;
	}
	else
	{
	    printf("=============sever %s:%s==============\n",__func__,strerror(errno));
	    return bytes; //return error
	}
    }

    bytes_w += bytes;

    if(bytes < len)
    {
	len -= bytes;
	buf = (char *)buf+bytes;
	goto write_again;
    }

    return bytes_w;
}

static int read_inner(int fd, void *buf, size_t len)
{
    size_t bytes = 0;
    int bytes_r = 0;

read_again:
    bytes = read(fd,buf,len);

    if(bytes <= 0)
    {
	if(bytes == 0)
	{
	    printf("%s: peer closed\n",__func__);
	    return bytes;
	}

	if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
	{
		sleep(1);
	    printf("=============%s:%s==============\n",__func__,strerror(errno));
	    goto read_again;
	}
	else
	{
	    printf("=============%s:%s with return value %d===============\n",__func__,strerror(errno),bytes);
	    return bytes;
	}
    }

    bytes_r += bytes;
    if(bytes < len)
    {
	len -= bytes;
	buf = (char *)buf + bytes;
	goto read_again;
    }

    return bytes_r;
}

void localsigroutine(int dunno){
	LOGE("sig: %d coming!\n",dunno);
	switch(dunno){
		case SIGINT:
		case SIGQUIT:
		case SIGHUP:
		{
			close(socket_fd);
			close(epoll_fd);
			unlink(UNIX_DOMAIN);
			exit(0);
		}
		case SIGPIPE:
		{
			//When the client is closed after start scaning and parsing,
			//this signal will come, ignore it!
			LOGD("do nothings for PIPE signal\n");
		}
	}
}
int setnonblocking( int fd )
{
	int old_option = fcntl( fd, F_GETFL );
	int new_option = old_option | O_NONBLOCK;
	fcntl( fd, F_SETFL, new_option );
	return old_option;
}

void addfd( int epollfd, int fd, bool one_shot )
{
	epoll_event event;
	event.data.fd = fd;
	if(epoll_type == ET)
		event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	else
		event.events = EPOLLIN | EPOLLRDHUP;
	if( one_shot )
	{
		event.events |= EPOLLONESHOT;
	}
	epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
	setnonblocking( fd );
}

void removefd( int epollfd, int fd )
{
	epoll_ctl( epollfd, EPOLL_CTL_DEL, fd, 0 );
	close( fd );
}

void modfd( int epollfd, int fd, int ev )
{
	epoll_event event;
	event.data.fd = fd;
	event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
	epoll_ctl( epollfd, EPOLL_CTL_MOD, fd, &event );
}

static int server_system(const char* s){
	LOGD("sh: %s\n",s);
	return system(s);
}

static void printf_info(struct _cmd *c){
	printf("server: printf_info\n");
	LOGD("cmd: %d\n",c->cmd);
	LOGD("ssid: %s\n",c->info.base_info.ssid);
	LOGD("pasd: %s\n",c->info.base_info.password);
	LOGD("pcol: %d\n",c->info.protocol);
	if(c->info.protocol == AW_SMARTLINKD_PROTO_AKISS)
		LOGD("radm: %d\n",c->info.airkiss_random);
	if(c->info.protocol == AW_SMARTLINKD_PROTO_COOEE){
		LOGD("ip: %s\n",c->info.ip_info.ip);
		LOGD("port: %d\n",c->info.ip_info.port);
	}
}

/*
*return val:
*1: from applicant client, protocol header:5A 5A A5 A5
*2: from protocol client, protocol header:3C 3C C3 C3
*0: peer to read is closed
*-1: read error
*-2: protocol header detect failed, can try again
*/
static int protocol_header_detect(int fd)
{
    char buf;
    int i = 0, bytes = 0;
    int protocol = -1;
    int is_5a_0 = 0, is_5a = 0;
    int is_a5_0 = 0, is_a5 = 0;
    int is_3c_0 = 0, is_3c = 0;
    int is_c3_0 = 0, is_c3 = 0;

    while((bytes = read_inner(fd, &buf, 1))  && (i < MAX_DET_NUM))
    {
	if(bytes < 0)
	{
	    if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
		continue;
	}
	else if(bytes == 0)//peer is terminated
	{
	    protocol = -1;
	    break;
	}

	if((buf == 0x5A) || (buf == 0xA5))
	{
	    if(protocol == 2)
            {
		is_3c_0 = 0;
		is_3c = 0;
		is_c3_0 = 0;
		is_c3 = 0;
	    }
	    protocol = 1;

	}
	else if((buf == 0x3C) || (buf == 0xC3))
	{
	    if(protocol == 1)
            {
		is_5a_0 = 0;
		is_5a = 0;
		is_a5_0 = 0;
		is_a5 = 0;
	    }
	    protocol = 2;
	}
	else
	{
	    is_3c_0 = 0;
	    is_3c = 0;
	    is_c3_0 = 0;
	    is_c3 = 0;
	    is_5a_0 = 0;
	    is_5a = 0;
	    is_a5_0 = 0;
	    is_a5 = 0;
	    protocol = -2;
	    continue;
	}

	if(protocol == 1)
	{
	    if(buf == 0x5A)
	    {
		if(is_5a_0 == 0)
		{
		    is_5a_0 = 1;
		    continue;
		}
		else
		{
		    if(is_a5_0 == 1)//in case of "5A 5A A5 5A", then restart detection with one "5A"
		    {
		        is_5a_0 = 1;
			is_5a = 0;
			is_a5_0 = 0;
			is_a5 = 0;
			continue;
		    }
		    is_5a = 1;
		    continue;
		}
	    }
	    else if(buf == 0xA5)
	    {
		if(is_5a_0 == 0 || is_5a == 0)// is case of "A5 ..." or "5A A5 ..."
		{
		    is_5a_0 = 0;
		    is_5a = 0;
		    is_a5_0 = 0;
		    is_a5 = 0;
		    protocol = -2;
		    continue;
		}
		else
		{
		    if(is_a5_0 == 0)
		    {
			is_a5_0 = 1;
			continue;
		    }
		    else
		    {
			is_a5 = 1;
			printf("%s: recieve data from application!\n", __func__);
			break;
		    }
		}
	    }
	}
	else
	{
	    if(buf == 0x3C)
	    {
		if(is_3c_0 == 0)
		{
		    is_3c_0 = 1;
		    continue;
		}
		else
		{
		    if(is_c3_0 == 1)//in case of "3C 3C C3 3C", then restart detection with one "3C"
		    {
			is_3c_0 = 1;
			is_3c = 0;
			is_c3_0 = 0;
			is_c3 = 0;
			continue;
		    }
		    is_3c = 1;
		    continue;
		}
	    }
	    else if(buf == 0xC3)
	    {
		if(is_3c_0 == 0 || is_3c == 0)// in case of "C3 ..." or "3C C3 ..."
		{
		    is_3c_0 = 0;
		    is_3c = 0;
		    is_c3_0 = 0;
		    is_c3 = 0;
		    protocol = -2;
		    continue;
		}
		else
		{
		    if(is_c3_0 == 0)
		    {
			is_c3_0 = 1;
			continue;
		    }
		    else
		    {
			is_c3 = 1;
			printf("%s: recieve data from protocol!\n", __func__);
			break;
		    }
		}
	    }
	i++;
	}//end of protocol == 1
    }//end of while

    if((i >= MAX_DET_NUM) || (bytes <= 0) )
    {
	if(bytes <= 0)
	    protocol = bytes;
	else
	protocol = -2;
    }

    return protocol;
}


/*
*1: protocol header (4bytes)
*write to applicant client, protocol header:5A A5 5A A5
*write to protocol client, protocol header:3C C3 3C C3
*2: data_cmd (sizof(int))
*3: data/cmd length (sizof(size_t))
*4: data (bytes of 'length')
*/
static int write_to_client(int fd, char *buf, int len, int data_cmd)
{
    char w_buf[4];
    int i;

    if (app_fd == fd)
    {
	w_buf[0] =0x5A;
	w_buf[1] =0xA5;
	w_buf[2] =0x5A;
	w_buf[3] =0xA5;
    }
    else if(proto_fd == fd)
    {
	w_buf[0] =0x3C;
	w_buf[1] =0xC3;
	w_buf[2] =0x3C;
	w_buf[3] =0xC3;
    }
/*
    for(i=0; i< 4; i++)
    {
	if(write(fd, (w_buf+i), 1) != 1)
	{
	    printf("write_to_client: write protocol header error!\n");
	    return -1;
	}
    }
*/

    if(write_inner(fd, w_buf, sizeof(char)*4) != sizeof(char)*4)
    {
	printf("%s: write protocol header error!\n",__func__);
	return -1;
    }

    if(write_inner(fd, &data_cmd, sizeof(int)) != sizeof(int))
    {
	printf("%s: write data_cmd error!\n",__func__);
	return -1;
    }

    if(write_inner(fd, &len, sizeof(size_t)) != sizeof(size_t))
    {
	printf("%s: write data length error!\n",__func__);
	return -1;
    }

    if(write_inner(fd, buf, len) != len)
    {
	printf("%s: write data error!\n",__func__);
	return -1;
    }

    return 0;
}

static int reply_ok(int fd)
{
    char buf[3] = "OK";
    return (write_to_client(fd, buf, sizeof(buf), IS_CMD));
}

static int reply_fine(int fd)
{
    char buf[5] = "FINE";
    return (write_to_client(fd, buf, sizeof(buf), IS_CMD));
}

static int send_data(int fd, char *buf, size_t len)
{
    return (write_to_client(fd, buf, len, IS_DATA));
}
//----------------------------------------------------------------------------------

Server::Server(){
	mTargetfd = -1;
	mEpollfd = -1;
	mListenfd = -1;
	mConnectCount = 0;
	mWifiModule = -1;
	mProto = NULL;
}

Server::~Server(){

}
int Server::run(){

	Thread::run();
	epoll_event events[ MAX_EVENT_NUMBER ];
	//a connect request use accept
	while( true ){
		int number = epoll_wait( mEpollfd, events, MAX_EVENT_NUMBER, -1 );
		if ( ( number < 0 ) && ( errno != EINTR ) ){
			LOGE( "epoll failure\n" );
			break;
		}

		for ( int i = 0; i < number; i++ ){
			int sockfd = events[i].data.fd;
			if( sockfd == mListenfd ){
				struct sockaddr_in client_address;
				socklen_t client_addrlength = sizeof( client_address );
				int connfd = accept( mListenfd, ( struct sockaddr* )&client_address, &client_addrlength );
				if ( connfd < 0 ){
					LOGE( "errno is: %d\n", errno );
					continue;
				}

				if( mConnectCount >= MAX_FD ){
					LOGD("server busy\n");
					continue;
				}

				int error = 0;
				socklen_t len = sizeof( error );
				getsockopt( connfd, SOL_SOCKET, SO_ERROR, &error, &len );
				int reuse = 1;
				setsockopt( connfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );
				addfd( mEpollfd, connfd, true );

				LOGD("listen fd coming\n");
			}
			else if( events[i].events & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLOUT) ){
				LOGD("EPOLLRDHUP fd=%d\n",events[i].data.fd);
				removefd(mEpollfd,events[i].data.fd);
			}
			else{
				LOGD("append fd=%d\n",events[i].data.fd);
				append(events[i]);
			}
		}
	}
	close(mListenfd);
	close(mEpollfd);
	unlink(UNIX_DOMAIN);

	return 0;
}

int Server::init(){

	int ret;
	int len;

	LOGD("start to link\n");
	mListenfd = socket(PF_UNIX,SOCK_STREAM,0);
	if(mListenfd < 0){
		LOGE("cannot create communication socket\n");
		return -1;
	}

	//set server addr_param
	struct sockaddr_un srv_addr;
	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path+1,UNIX_DOMAIN);
	srv_addr.sun_path[0] = '\0';
	unlink(UNIX_DOMAIN);
	int size = offsetof(struct sockaddr_un,sun_path)+strlen(UNIX_DOMAIN)+1;
	//bind socket fd and addr
	ret = bind(mListenfd,(struct sockaddr*)&srv_addr,size);
	if( ret == -1 ){
		LOGE("cannot bind server socket: %s\n",strerror(errno));
		close(mListenfd);
		unlink(UNIX_DOMAIN);
		return -1;
	}
	chmod(UNIX_DOMAIN,0777);
	//listen socket fd
	ret = listen(mListenfd,5);
	if(ret == -1){
		LOGE("cannot listen the client connect request: %s\n",strerror(errno));
		close(mListenfd);
		unlink(UNIX_DOMAIN);
		return -1;
	}

	//epoll
	mEpollfd = epoll_create( 5 );
	if(mEpollfd < 0){
		LOGE("cannot create epoll\n");
		return -1;
	}
	addfd( mEpollfd, mListenfd, false );

	//signal
	//TODO: use server sigroutine
	//signal(SIGHUP,(void (*)(int))&Server::sigroutine);
	//signal(SIGQUIT,(void (*)(int))&Server::sigroutine);
	//signal(SIGINT,(void (*)(int))&Server::sigroutine);
	socket_fd = mListenfd;
	epoll_fd = mEpollfd;
	signal(SIGHUP,localsigroutine);
	signal(SIGQUIT,localsigroutine);
	signal(SIGINT,localsigroutine);
	signal(SIGPIPE,localsigroutine);
	return 0;
}

static int lt_read(int fd, char* buf, int size)
{
    int bytes;
    int bytes_r = 0;

recv_again:
    bytes = recv(fd,buf,size,0);

    if(bytes < 0)
    {
	if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
	{
		sleep(1);
	    printf("%s:%s",__func__,strerror(errno));
	    goto recv_again;
	}
	else
	{
	    printf("%s:%s",__func__,strerror(errno));
	    return bytes;
	}
    }
    else if (bytes == 0)
    {
	return bytes;
    }

    bytes_r += bytes;

    if(bytes < size)
    {
	size -= bytes;
	buf += bytes;
	goto recv_again;
    }

    return bytes_r;
}

static int et_read(int fd, char* buf, int size){
	int m_read_idx = 0;
	int bytes_read = 0;
	while( true ){
		bytes_read = recv( fd, buf + m_read_idx, size - m_read_idx, 0 );
		if ( bytes_read == -1 )	{
			if( errno == EAGAIN || errno == EWOULDBLOCK ){
				return m_read_idx;
			}
		}
		else if ( bytes_read == 0 ){
			return 0;
		}

		m_read_idx += bytes_read;
	}
	return 0;
}

int Server::read(int fd, void* buf, int size){
	if(epoll_type == ET)
		return et_read(fd, (char *)buf, size);
	return lt_read(fd, (char *)buf, size);
}
int Server::write(int fd, void* buf, int size){
	int ret = send(fd,buf,size,0);
	if(ret == -1)
		LOGE("send failed!!\n");
	return ret;
}

int Server::responseFail(int fd){
	struct _cmd c;
	c.cmd = AW_SMARTLINKD_FAILED;
	write(fd,(char*)&c,sizeof(c));
	modfd(mEpollfd,fd,EPOLLOUT);
	return 0;
}
int Server::handleRequest(int fd, char* buf, int length){
	struct _cmd *c = (struct _cmd *)buf;
	Proto *p = NULL;
#ifndef ANDROID_ENV
	OSWifiNetwork* t = new TinaWifiNetwork;
#else
	OSWifiNetwork* t = new AndroidWifiNetwork;
#endif
	//LOGD("request cmd %d\n",c->cmd);
	switch(c->cmd){

	case AW_SMARTLINKD_START:
		p = ProtoFactory::getInstance()->createProto(c->info.protocol);
		if(p != NULL ) {
			//mConnectCount++;
			mProto = p;
			mProto->startNoBlock();
			mTargetfd = fd;
		}else
			responseFail(fd);
		break;
        case AW_SMARTLINKD_STOP:
		//release
		if(mProto != NULL){
			//mConnectCount--;
			int ret = mProto->stop();
                if(ret == -1)
			LOGE("Proto stop fail(%s)",strerror(errno));
			ProtoFactory::getInstance()->releaseProto(mProto);
			mProto = NULL;
		}
		break;
	case AW_SMARTLINKD_FINISHED:
		printf_info(c);
//		write(mTargetfd,buf,length);
		send_data(mTargetfd,buf,(size_t)length);
		sleep(1);

//		modfd(mEpollfd,fd,EPOLLOUT);
//		modfd(mEpollfd,mTargetfd,EPOLLOUT);
//		if(0 == proto)
//			app_fd = -1;
//		else
//			proto_fd = -1;
//		mTargetfd = -1;
#ifndef ANDROID_ENV
		//if no fail
		if(c->info.protocol == AW_SMARTLINKD_PROTO_AKISS){
			//t->startLink(&c->info);
			//t->requestIP();
			//mProto->respProto((char*)(&c->info.airkiss_random));
		}
#endif
		//release
		if(mProto != NULL){
			//mConnectCount--;
			ProtoFactory::getInstance()->releaseProto(mProto);
			mProto = NULL;
		}

		break;
	default:
		;
	}
	delete t;
	return 0;
}
int Server::process(epoll_event event){

	if( event.events & EPOLLIN ){
		//LOGD("EPOLLIN fd=%d\n",event.data.fd);
//		char buf[1024];
		char *buf = NULL;
		size_t data_len = 0;
		int data_cmd;

		proto = protocol_header_detect(event.data.fd);

		if(1 == proto)  //TO ADD:in case app_fd != -1, already have client in it
			app_fd = event.data.fd;
		else if(2 == proto)
			proto_fd = event.data.fd;
		else
		{
			LOGE("sever::process: cannot detect supported protocol of client!\n");
			modfd(mEpollfd,event.data.fd,EPOLLOUT);

			if(-1 == proto) //communication error ocurred
			    return Thread::THREAD_EXIT;
			else
			    return Thread::THREAD_CONTINUE;
		}
		size_t bytes = read(event.data.fd, &data_cmd, sizeof(int));
		if (bytes <= 0){
			LOGE("sever::process: failed to distinguish data from cmd!\n");
			modfd(mEpollfd,event.data.fd,EPOLLOUT);
			if(0 == proto)
				app_fd = -1;
			else
				proto_fd = -1;

			if(bytes < 0)
				return Thread::THREAD_EXIT;
			else
				return Thread::THREAD_CONTINUE;
		}

		if(data_cmd == IS_CMD) //if recieve cmd, such as replied "OK", ignore
		{
			LOGE("sever::process: recieve the OK from app, finish sevice for it!\n");

			if(reply_fine(event.data.fd) != 0)
			    LOGE("sever::process: fail to reply fine\n");

			modfd(mEpollfd,event.data.fd,EPOLLOUT);
			modfd(mEpollfd,mTargetfd,EPOLLOUT);
			if(0 == proto)
				app_fd = -1;
			else
				proto_fd = -1;
			mTargetfd = -1;

			return Thread::THREAD_CONTINUE;//For this moment, just have recieved the "OK" from app, implying it has got the message
		}
		//TO ADD: after send message to app and exceeds the time to wait, resend the message for several times

		bytes = read(event.data.fd, &data_len, sizeof(size_t));
		if (bytes <= 0){
			LOGE("sever::process: failed to get the data length!\n");
			modfd(mEpollfd,event.data.fd,EPOLLOUT);
			if(0 == proto)
				app_fd = -1;
			else
				proto_fd = -1;

			if(bytes < 0)
				return Thread::THREAD_EXIT;
			else
				return Thread::THREAD_CONTINUE;
		}

		if(data_len > 1024)
			buf = (char *)malloc(data_len);
		else
			buf = (char *)malloc(1024);

		if(NULL == buf){
			LOGE("sever::process: malloc faild\n");
		}

		bytes = read_inner(event.data.fd, buf, data_len);

		if(bytes == data_len)
		{
			if(reply_ok(event.data.fd) != 0)
			    LOGE("sever::process: fail to reply OK\n");
		}
		else
		{
			LOGE("sever::process: fail to read %d data_len data, having read %d bytes\n", data_len, bytes);
			free(buf);
			modfd(mEpollfd,event.data.fd,EPOLLOUT);
			if(0 == proto)
				app_fd = -1;
			else
				proto_fd = -1;

			if(bytes < 0)
				return Thread::THREAD_EXIT;
			else
				return Thread::THREAD_CONTINUE;
		}

//		int bytes = read(event.data.fd, buf, 1024);
		//LOGD("bytes_read: %d\n",bytes);
		if(bytes > 0){
			handleRequest(event.data.fd, buf,bytes);
		}else{
			LOGE("read failed!! close fd, process thread exit!");
			modfd(mEpollfd,event.data.fd,EPOLLOUT);
			free(buf);
			if(0 == proto)
				app_fd = -1;
			else
				proto_fd = -1;
			if(bytes < 0)
				return Thread::THREAD_EXIT;
			else
				return Thread::THREAD_CONTINUE;
		}

		modfd(mEpollfd,event.data.fd,EPOLLIN);
		free(buf);
	}
	return Thread::THREAD_CONTINUE;
}
void Server::sigroutine(int dunno){
	switch(dunno){
		case SIGINT:
		case SIGQUIT:
		case SIGHUP:
		{
			LOGE("sig: %d\n coming!",dunno);
			close(mListenfd);
			close(mEpollfd);
			unlink(UNIX_DOMAIN);
			exit(0);
		}
	}
}
