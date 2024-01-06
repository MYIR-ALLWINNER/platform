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

#include "Adt.h"

Adt::Adt(){

}
Adt::~Adt(){

}

//return Thread::THREAD_EXIT to exit the loop
int Adt::start(){

    char cmd[100];
    sprintf(cmd,"%s",SUNXI_ADT);
    //todo: check the return value
    doSystem(cmd,true);

    return Thread::THREAD_EXIT;
}

int Adt::stop(){
    LOGD("stop scan process....");
    return stopScanProcess(SUNXI_ADT);
}
int Adt::respProto(char* resp){
    return 0;
}
