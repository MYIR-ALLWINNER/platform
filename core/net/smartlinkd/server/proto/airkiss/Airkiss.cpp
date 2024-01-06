#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <memory.h>

#include <netinet/ether.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/ioctl.h>

#include <net/if_arp.h>

#include "Airkiss.h"

Airkiss::Airkiss(){

}
Airkiss::~Airkiss(){

}

//return Thread::THREAD_EXIT to exit the loop
int Airkiss::start(){

	//reset the wlan
#ifndef ANDROID_ENV
	//doSystem("ifconfig "IFNAME" down",false);
	//doSystem("ifconfig "IFNAME" up",false);
#else
	//doSystem("svc wifi disable");
	//doSystem("ifconfig "IFNAME" up");
	//doSystem("sleep 1");
#endif
	char cmd[100];
	int wifimodule = checkWifiModule();
	if(wifimodule == AW_SMARTLINKD_BROADCOM){
		if(mIsUseAESKey){
			sprintf(cmd,"%s -d -p 4 -v %s",BROADCOM_AIKISS,mAESKey);
		}else{
			sprintf(cmd,"%s -d -p 4 ",BROADCOM_AIKISS);
		}
	}else if(wifimodule == AW_SMARTLINKD_REALTEK){
		if(mIsUseAESKey){
			sprintf(cmd,"%s -d -p 4 -v %s",REALTEK_AIKISS,mAESKey);
		}else{
			sprintf(cmd,"%s -d -i wlan0 -c /tmp/tmp.conf",REALTEK_AIKISS);
		}
	}else if(wifimodule == AW_SMARTLINKD_XR819){
		if(mIsUseAESKey){
			sprintf(cmd,"%s %s",XR819_AIKISS,mAESKey);
		}else{
			sprintf(cmd,"%s",XR819_AIKISS);
		}
	}
	//todo: check the return value
	doSystem(cmd,true);

	return Thread::THREAD_EXIT;
}
int Airkiss::stop(){
    LOGD("stop scan process....");
    return stopScanProcess(BROADCOM_AIKISS);
}
int Airkiss::respProto(char* resp){
	return sendProtoBoardcast(*(int*)resp);
}
int Airkiss::sendProtoBoardcast(int random){
	struct sockaddr_in s;
	int sockfd, addr_len, broadcast = 1;
	unsigned int self_ipaddr = 0, i;
	unsigned char payload = 0;
	char smac[6];

	//sleep(10);
	LOGD("Broadcast airkiss UDP ack\n ");
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		LOGE("new socket failed\n");
		return -1;
	}

	// this call is what allows broadcast packets to be sent:
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast) == -1) {
		LOGE("setsockopt (SO_BROADCAST) failed\n");
		return -1;
	}

	//get_addr(sockfd, smac, &self_ipaddr);
	//self_ipaddr |= htonl(0x000000FF);
	//bzero(&s,sizeof(struct sockaddr_in));

	s.sin_family = AF_INET;
	s.sin_port = htons(ACK_DEST_PORT);
	s.sin_addr.s_addr = htonl(INADDR_BROADCAST);//inet_addr("255.255.255.255");//self_ipaddr;   //htonl(INADDR_BROADCAST);
	payload = random;
	addr_len = sizeof(struct sockaddr);
	for(i=0; i < SEND_ACK_TIMES; i++){
		int ret = sendto(sockfd,
				(unsigned char *)&payload,
				sizeof(unsigned char),
				0,
				(struct sockaddr *)&s,
				addr_len);
		if(ret == -1){
			LOGE("ret = %d,%s\n",ret,strerror(errno));
			usleep(50000);
		}
		LOGD("send broadcoast ret = %d\n",ret);
		usleep(20000);
	}
	close(sockfd);

	return 0;
}
