#define TAG "smartlinkd-client"
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stddef.h>
#include "aw_smartlinkd_connect.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

static int(*func)(char*,int) = NULL;

#ifndef ANDROID_ENV
#define UNIX_DOMAIN "/tmp/UNIX.domain"
#else
#define UNIX_DOMAIN "/data/misc/wifi/UNIX.domain"
#endif
static int sockfd = -1;
static int initialized = 0;

static int app_finish_rec_msg_flag = 0;

static char *app_msg_buf;
static int app_msg_len;

//indicate the client user is app(0) or protocol(1), default for protocol use
static int proto_use = 1;

static void clear_for_app_use(void)
{
    proto_use = 0;
}

static int which_user(void)
{
    return proto_use;
}

static int write_inner(int fd, void *buf, size_t len)
{
    int bytes;
    int bytes_w = 0;

write_again:
    bytes = write(fd,buf,len);

    if(bytes <= 0)
    {
	if(bytes == 0)
	    return bytes_w;

	if(errno == EINTR)
	    goto write_again;
	else
	    return bytes;
    }

    bytes_w += bytes;

    if(bytes < len)
    {
	len -= bytes;
	buf = (char *)buf + bytes;
	goto write_again;
    }

    return bytes_w;
}

static int read_inner(int fd, void *buf, size_t len)
{
    int bytes;
    int bytes_r = 0;

read_again:
    bytes = read(fd,buf,len);

    if(bytes <= 0)
    {
	if(bytes == 0)
	    return bytes_r;

	if(errno == EINTR)
	    goto read_again;
	else
	    return bytes;
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

void printf_info(struct _cmd *c){
	LOGD("cmd: %d\n",c->cmd);
	if(c->cmd == AW_SMARTLINKD_FAILED){
		LOGD("response failed\n");
		return;
	}
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

static int init_socket(){
	int ret;
	struct sockaddr_un srv_addr;
	//creat unix socket
	if(sockfd != -1){
		LOGE("another smartlinkd client exsit, please wait!!");
		return -1;
	}
	sockfd=socket(PF_UNIX,SOCK_STREAM,0);
	if(sockfd<0)
	{
		LOGE("cannot create communication socket");
		return -1;
	}
	srv_addr.sun_family=AF_UNIX;
	//strcpy(srv_addr.sun_path,UNIX_DOMAIN);
	strcpy(srv_addr.sun_path+1,UNIX_DOMAIN);
	srv_addr.sun_path[0] = '\0';
	int size = offsetof(struct sockaddr_un,sun_path) + strlen(UNIX_DOMAIN)+1;
	//connect server
	ret=connect(sockfd,(struct sockaddr*)&srv_addr,size);
	if(ret==-1)
	{
		LOGE("cannot connect to the server: %s",strerror(errno));
		close(sockfd);
	        sockfd = -1;
		return -1;
	}
	return 0;
}

void aw_smartlinkd_prepare(){
	//stop wpa_supplicant
	system("/etc/wifi/wifi stop");
	//up the wlan
	system("ifconfig wlan0 up");
}
void aw_release(){
    close(sockfd);
    sockfd = -1;
}

static int send_protocol_header(int fd, size_t len, int data_cmd)
{
    char w_buf[4];

    if(0 == which_user())
    {
	w_buf[0] = 0x5A;
	w_buf[1] = 0x5A;
	w_buf[2] = 0xA5;
	w_buf[3] = 0xA5;
    }
    else
    {
	w_buf[0] = 0x3C;
	w_buf[1] = 0x3C;
	w_buf[2] = 0xC3;
	w_buf[3] = 0xC3;
    }

    if(write_inner(fd, w_buf, 4) != 4)
    {
	LOGE("%s: send protocol header faild!\n", __func__);
	return -1;
    }

    if(write_inner(fd, &data_cmd, sizeof(int)) != sizeof(int))
    {
	LOGE("%s: write data_cmd error!\n", __func__);
	return -1;
    }

    if(write_inner(fd, &len, sizeof(size_t)) != sizeof(size_t))
    {
	LOGE("%s: write data length error!\n", __func__);
	return -1;
    }

    printf("===========client %s: send header successed!=================\n",__func__);
    return 0;

}


/*
*recieve protocol header:
*for app recieve: 0x5A 0xA5 0x5A 0xA5
*for protocol recieve: 0x3C 0xC3 0x3C 0xC3
*/
static int protocol_header_detect(int fd)
{
    char buf;
    int i = 0, bytes = 0;
    int is_5a_0 = 0, is_a5_0 = 0;
    int is_5a = 0, is_a5 = 0;
    int is_3c_0 = 0, is_c3_0 = 0;
    int is_3c = 0, is_c3 = 0;

    while((bytes =  read_inner(fd, &buf, 1)) && (i < MAX_DET_NUM))
    {

	if(bytes < 0)
	{
	    if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
		continue;
	}

        if(0 == which_user())
	{
	    if(buf == 0x5A)
	    {
		if(is_5a_0 == 0)
		{
		    is_5a_0 = 1;
		    continue;
		}
		else if(is_5a_0 == 1)
		{
		    if(is_a5_0 == 0 || is_5a == 1)//in case of "5A 5A ..." or "5A A5 5A 5A"
		    {
		        is_5a_0 = 1;
		        is_a5_0 = 0;
		        is_5a = 0;
		        is_a5 = 0;
		        continue;
		    }
		    is_5a = 1;
		    continue;
		}
	    }
	    else if(buf == 0xA5)
	    {
		if(is_5a_0 == 1)
		{
		    if(is_5a == 0)
		    {
		        //in case of "5A A5 A5"
		        if(is_a5_0 == 1)
		        {
		            is_5a_0 = 0;
		            is_a5_0 = 0;
		            continue;
		        }
		        is_a5_0 = 1;
		        continue;
		    }
		    else
		    {
			is_a5 = 1;
			return 0;
		    }
		}
	        else //in case of starting with A5: "A5 ..."
		{
		    is_5a_0 = 0;
		    is_a5_0 = 0;
		    is_5a = 0;
		    is_a5 = 0;
		    continue;
		}
	    }
	}
	else if(1 == which_user())
	{
	    if(buf == 0x3C)
	    {
		if(is_3c_0 == 0)
		{
		    is_3c_0 = 1;
		    continue;
		}
		else if(is_3c_0 == 1)
		{
		    if(is_c3_0 == 0 || is_3c == 1)//in case of "3C 3C ..." or "3C C3 3C 3C"
		    {
		        is_3c_0 = 1;
		        is_c3_0 = 0;
		        is_3c = 0;
		        is_c3 = 0;
		        continue;
		    }
		    is_3c = 1;
		    continue;
		}
	    }
	    else if(buf == 0xC3)
	    {
		if(is_3c_0 == 1)
		{
		    if(is_3c == 0)
		    {
		        //in case of "3C C3 C3"
		        if(is_c3_0 == 1)
		        {
		            is_3c_0 = 0;
		            is_c3_0 = 0;
		            continue;
		        }
		        is_c3_0 = 1;
		        continue;
		    }
		    else
		    {
			is_c3 = 1;
			return 0;
		    }
		}
	        else //in case of starting with C3: "C3 ..."
		{
		    is_3c_0 = 0;
		    is_c3_0 = 0;
		    is_3c = 0;
		    is_c3 = 0;
		    continue;
		}
	    }

	}
    i++;
    }//end of while

    return -1;

}//end of protocol_header_detect

static int client_reciever(int fd, char **buf, size_t *len_to_recieve, int *data_cmd)
{
    int ret = 0;
    int bytes;
    int length = *len_to_recieve;
    char *rec_buf = (char *)(*buf);
    char *new_buf = NULL;

    ret = protocol_header_detect(fd);
    if(0 != ret)
    {

	if(app_finish_rec_msg_flag == 1)
	    return 0;
	else
	{
	    printf("%s: protocol header detection failed!\n", __func__);
	    return -1;
	}
    }

    bytes = read_inner(fd, data_cmd, sizeof(int));
    if(bytes <= 0)
    {
	printf("%s: recieve data_cmd faild!\n", __func__);
	return bytes;
    }


    bytes = read_inner(fd, len_to_recieve, sizeof(size_t));
    if(bytes <= 0)
    {
	printf("%s: recieve data/cmd length faild!\n", __func__);
	return bytes;
    }

    if(*len_to_recieve > length) //if the lenth of data to recieve is larger than the room allocated originally
    {
	printf("realloc buf!\n");
	new_buf = (char *)realloc(rec_buf, *len_to_recieve);
	if(NULL != new_buf)
	    rec_buf = new_buf;
	*buf = rec_buf;
    }

    bytes = read_inner(fd, rec_buf, *len_to_recieve);
  //  free(buf_rec);
    if(bytes <= 0)
    {
	printf("%s: recieve data/cmd faild!\n", __func__);
	return bytes;
    }

    return bytes;

}

static int reply_cmd(char *buf)
{
    size_t len;

    len = strlen(buf)+1;
    send_protocol_header(sockfd,len,IS_CMD);
    if(write_inner(sockfd, buf, len) != len)
    {
	LOGE("%s: reply cmd error!\n", __func__);
	return -1;
    }

    return 0;
}

static void * readthread(void* arg){
//	char buf[1024];
	char *buf;
	size_t bytes_to_recieve;
	int ret;
	int data_cmd;

	buf = (char *)calloc(1,1024);
	if(func == NULL || func(buf,THREAD_INIT) == THREAD_EXIT) //test
		goto exit_thread;
//	bytes_to_recieve = sizeof(buf);
	bytes_to_recieve = 1024;
	while(1){

		ret = client_reciever(sockfd, &buf, &bytes_to_recieve, &data_cmd);
		if(ret == -1)
		{
			if( errno == EAGAIN || errno == EWOULDBLOCK ){
				continue;
			}
			LOGE("recv failed(%s)\n",strerror(errno));
//			func(buf,ret);
			break;
		}
		else if(ret == 0)
		{
			if(app_finish_rec_msg_flag == 1)
				printf("%s:app finish recieve msg\n", __func__);
			else
				printf("%s:server close the connection\n", __func__);
//			func(buf,ret);
			break;
		}

		if((int)data_cmd == IS_DATA) //for app to recieve the data
		{
		    reply_cmd("OK");
		    app_msg_buf = (char *)calloc(1,ret);
		    app_msg_len = ret;
		    memcpy(app_msg_buf,buf,ret);
		    printf("===========%s: app have saved the msg =================\n",__func__);
		}
		else
		{
		    if(strcmp(buf,"OK") == 0)
		    {
			if(1 == which_user())//for protocol to recieve reply;for app to ignore it
			{
			    printf("===========%s: protocol recieve \"OK\"=================\n",__func__);
			    if( func(buf,ret) == THREAD_EXIT)
				break;
			}
		    }
		    else if(strcmp(buf,"FINE") == 0)
		    {
			if(0 == which_user())//for app to recieve reply "FINE";for protocol to ignore it
			{
			    printf("===========%s: app recieve \"FINE\"=================\n",__func__);
			    app_finish_rec_msg_flag = 1;
			    if( func(app_msg_buf,app_msg_len) == THREAD_EXIT)
			    {
				free(app_msg_buf);
				break;
			    }
			}
		    }
		}

	}

exit_thread:
	initialized = 0;
	free(buf);
	LOGD("thread exit....");
	close(sockfd);
	sockfd = -1;
	return ((void *)0);
}
static int init_thread(){
	pthread_t tid;
	if(pthread_create( &tid, NULL, readthread, (void*)0 ) != 0){
		LOGE("create read thread failed!\n");
		return -1;
	}
	if( pthread_detach( tid ) ){
		LOGE("detach read thread failed!\n");
		return -1;
	}
	return 0;
}
int aw_smartlinkd_init(int fd,int(*f)(char*,int)){

    func = f;
    if(init_socket() == 0 && init_thread() == 0){
        initialized = 1;
        return 0;
    }
    return -1;
}


static int send_package(int fd, char *buf, size_t len, char data_cmd)
{
    int ret;

    if(send_protocol_header(fd, len, data_cmd) != 0)
    {
	LOGE("%s: send protocol header error!\n", __func__);
	return -1;
    }

    ret = send(fd, buf, len, 0);
    if(ret != len)
    {
	printf("%s:send package failed, %d package to send, sent %d\n",__func__,len,ret);
    }

    return ret;
}

static int startprotocol(int protocol){
    struct _cmd c;
    c.cmd = AW_SMARTLINKD_START;
    c.info.protocol = protocol;
    LOGD("protocol = %d\n",protocol);
    clear_for_app_use();//set the user as app
    if(initialized == 0)
        aw_smartlinkd_init(0,func);

    return send_package(sockfd,(char *)&c, sizeof(c), IS_DATA);

//  return send(sockfd,(char*)&c,sizeof(c),0);
}
static int stopprotocol(){
    struct _cmd c;
    c.cmd = AW_SMARTLINKD_STOP;
    return send_package(sockfd,(char *)&c, sizeof(c), IS_DATA);
    //return send(sockfd,(char*)&c,sizeof(c),0);
}
int aw_startairkiss(){
    return startprotocol(AW_SMARTLINKD_PROTO_AKISS);
}
int aw_stopairkiss(){
    return stopprotocol();
}

int aw_startcooee(){
    return startprotocol(AW_SMARTLINKD_PROTO_COOEE);
}
int aw_stopcooee(){
    return stopprotocol();
}

int aw_startadt(){
    return startprotocol(AW_SMARTLINKD_PROTO_ADT);
}

int aw_stopadt(){
    return stopprotocol();
}
int aw_startxrsc(){
	return startprotocol(AW_SMARTLINKD_PROTO_XRSC);
}
int aw_stopxrsc(){
	return stopprotocol();
}
int aw_startcomposite(int proto_mask){
    return startprotocol(proto_mask);
}
int aw_stopcomposite(){
    return stopprotocol();
}

int aw_easysetupfinish(struct _cmd *c){
	//printf_info(c);
    c->cmd = AW_SMARTLINKD_FINISHED;
    return send_package(sockfd,(char *)c, sizeof(struct _cmd), IS_DATA);
    //return send(sockfd,(char*)c,sizeof(struct _cmd),0);
}

int check_wlan_interface(){
    int bytes;
    char buf[10];
    FILE *stream;
    stream = popen(CHECK_WLAN_SHELL, "r");
    if(!stream)
	return -1;
    bytes = fread(buf, sizeof(char), sizeof(buf), stream);
    pclose(stream);
    if(bytes > 0){
	printf("%s is up\n", IFNAME);
	return 0;
    }else{
	printf("%s is down\n", IFNAME);
	return 1;
    }

}


int convert_adt_str_to_info(char ssid[], char passwd[], char *string)
{
    int i = 0;
    char *p = string;
    char *p_tr = NULL;
    char *p_div = "::div::";
    char buf[256] = {0};

    printf("The recieved string is %s\n",string);
    while(*(p+i) != '\0')
    {
	buf[i] = *(p+i);
	i++;
    }
    buf[i] = '\0';

    p_tr = strstr(buf, p_div);
    if( NULL != p_tr)
    {
	i = 0;
	while((buf+i) != p_tr)
	{
	    ssid[i] = buf[i];
	    i++;
	}
	ssid[i] = '\0';
	printf("convert_adt_str_to_info: ssid is %s\n", ssid);
    }
    else
    {
	printf("convert_adt_str_to_info: can't resolve the ssid from the %s\n", buf);
	return -1;
    }

    p_tr = p_tr + strlen(p_div);
    p = strstr(p_tr, p_div);
    if( NULL == p)
    {
	i = 0;
        while(*(p_tr+i) != '\0')
	{
	    passwd[i] = *(p_tr+i);
	    i++;
	}
	passwd[i] = '\0';
	printf("convert_adt_str_to_info: passwd is %s\n", passwd);
	if(strlen(passwd) < 8)
	{
	    printf("convert_adt_str_to_info:ERROR: WRONG FORMAT PASSWORD!\n");
	    return -1;
	}
    }

    return 0;

}

#ifdef __cplusplus
}
#endif
