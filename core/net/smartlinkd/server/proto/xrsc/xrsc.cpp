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

#include "xrsc.h"

xrsc::xrsc(){

}
xrsc::~xrsc(){

}

//return Thread::THREAD_EXIT to exit the loop
int xrsc::start(){

    char cmd[100];
    sprintf(cmd,"%s",SUNXI_XRSC);
    //todo: check the return value
    doSystem(cmd,true);

    return Thread::THREAD_EXIT;
}

int xrsc::stop(){
    LOGD("stop scan process....");
    return stopScanProcess(SUNXI_XRSC);
}
int xrsc::respProto(char* resp){
    return 0;
}
