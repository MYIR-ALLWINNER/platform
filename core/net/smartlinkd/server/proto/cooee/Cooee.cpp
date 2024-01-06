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

#include "Cooee.h"

Cooee::Cooee(){

}
Cooee::~Cooee(){

}

//return Thread::THREAD_EXIT to exit the loop
int Cooee::start(){

    //reset the wlan
#ifndef ANDROID_ENV
    //doSystem("ifconfig "IFNAME" down");
    //doSystem("ifconfig "IFNAME" up");
#else
    //doSystem("svc wifi disable");
    //doSystem("ifconfig "IFNAME" up");
    //doSystem("sleep 1");
#endif
    char cmd[100];
    int wifimodule = checkWifiModule();
    if(wifimodule == AW_SMARTLINKD_BROADCOM){
        if(mIsUseAESKey){
            sprintf(cmd,"%s -d -p 1 -v %s",BROADCOM_COOEE,mAESKey);
        }else{
            sprintf(cmd,"%s -d -p 1 ",BROADCOM_COOEE);
        }
    }
    //todo: check the return value
    doSystem(cmd,true);

    return Thread::THREAD_EXIT;
}

int Cooee::stop(){
    int ret = 0;
    LOGD("stop scan process....");
    int wifimodule = checkWifiModule();
    if(wifimodule == AW_SMARTLINKD_BROADCOM){
        ret = stopScanProcess(BROADCOM_COOEE);
    }
    return ret;
}
int Cooee::respProto(char* resp){
    return 0;
}
