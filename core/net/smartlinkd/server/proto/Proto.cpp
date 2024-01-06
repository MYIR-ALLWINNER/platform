#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Proto.h"

#include "Airkiss.h"
#include "Cooee.h"
#include "Adt.h"
#include "xrsc.h"

Proto::Proto(){
	memset(mAESKey,0,sizeof(mAESKey));
	mIsUseAESKey = false;
	mForkpid = -1;
}

Proto::~Proto(){

}

int Proto::doSystem(char* cmd,bool scan_process){
    LOGD("sh: %s\n",cmd);
    return system(cmd);

    pid_t pid;
    int status;

    if(cmd == NULL){
        return (1); //如果cmdstring为空，返回非零值，一般为1
    }
    if((pid = fork())<0){
        status = -1; //fork失败，返回-1
    }
    else if(pid == 0){
        execl("/bin/sh", "sh", "-c", cmd, (char *)0);
        _exit(127); // exec执行失败返回127，注意exec只在失败时才返回现在的进程，成功的话现在的进程就不存在啦~~
    }
    else //父进程
    {
        if(scan_process)
            mForkpid = pid;
        while(waitpid(pid, &status, 0) < 0){
            if(errno != EINTR){
                status = -1; //如果waitpid被信号中断，则返回-1
                break;
            }
        }
    }

    return status; //如果waitpid成功，则返回子进程的返回状态

}

int Proto::stopScanProcess(const char* process){
    char cmd[30];
    sprintf(cmd,"killall %s",process);
    return doSystem(cmd,false);
}

int Proto::setAESKey(char *key){
	memset(mAESKey,0,sizeof(mAESKey));
	mIsUseAESKey = true;
	memcpy(mAESKey,key,sizeof(mAESKey)-1);
	mAESKey[sizeof(mAESKey)] = '\0';
	return 0;
}

int Proto::checkWifiModule(){
	int bytes;
	char buf[10];
	FILE   *stream;

	stream = popen(CHECK_XRADIO_SHELL,"r");
	if(!stream) return -1;
	bytes = fread( buf, sizeof(char), sizeof(buf), stream);
	pclose(stream);
	if(bytes > 0)
		return AW_SMARTLINKD_XR819;
	else {
		stream = popen(CHECK_WIFI_SHELL,"r");
		if(!stream) return -1;
		bytes = fread( buf, sizeof(char), sizeof(buf), stream);
		pclose(stream);
		if(bytes > 0){
			return AW_SMARTLINKD_BROADCOM;
		}else {
			return AW_SMARTLINKD_REALTEK;
		}
	}
}

int Proto::checkWlanInterface(){
	int bytes;
	char buf[10];
	FILE   *stream;
	stream = popen( CHECK_WLAN_SHELL, "r" ); //CHECK_WIFI_SHELL-->> FILE* stream
	if(!stream) return -1;
	bytes = fread( buf, sizeof(char), sizeof(buf), stream);
	pclose(stream);
	if(bytes > 0){
		LOGD("%s is up\n",IFNAME);
		return 0;
	}else {
		LOGD("%s is down\n",IFNAME);
		return 1;
	}
}
int Proto::startNoBlock(){
	return Thread::run();
}
int Proto::loop(){
	//start() return Thread::THREAD_EXIT to exit the thread
	return start();
}

//--------------------------------------------------------------
/* static */
ProtoFactory* ProtoFactory::instance = NULL;
locker* ProtoFactory::lock = new locker();

ProtoFactory* ProtoFactory::getInstance(){
	if(instance == NULL){
		locker::autolock _l(lock);
		if(instance == NULL){
			instance = new ProtoFactory();
		}
	}
	return instance;
}

Proto* ProtoFactory::createProto(int proto){
	//Proto* p = NULL;
	if(mCurProto != NULL) return NULL;
	switch(proto){
	case AW_SMARTLINKD_PROTO_AKISS:
		LOGD("Create Airkiss\n");
		mCurProto = new Airkiss();
		break;
	case AW_SMARTLINKD_PROTO_COOEE:
		LOGD("Create Cooee\n");
		mCurProto = new Cooee();
		break;
	case AW_SMARTLINKD_PROTO_ADT:
		LOGD("Create SunxiAdt\n");
		mCurProto = new Adt();
		break;
	case AW_SMARTLINKD_PROTO_XRSC:
		LOGD("Create xrsc\n");
		mCurProto=NULL;
		mCurProto = new xrsc();
		break;
	default:
		;
	}
	return mCurProto;
}

Proto* ProtoFactory::getCurProto(){
    return mCurProto;
}

void ProtoFactory::releaseProto(Proto* p){
	if(p == mCurProto)
		delete mCurProto;
	else
		delete p;
	mCurProto = NULL;
}
