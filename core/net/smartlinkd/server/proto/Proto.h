#ifndef __PROTO_H__
#define __PROTO_H__

#include "Thread.h"
#include "log.h"
#include "smartlink_util.h"

/* ------------------Proto------------------ */
class Proto : public Thread< int >{
public:
	Proto();
	virtual ~Proto();

public:
	/* main start function, it maybe block the calling thread*/
	virtual int start() = 0;

    /* stop current proto*/
    virtual int stop() = 0;

	/* new a thread to call the start()*/
	virtual int startNoBlock();

	/* set AES key */
	virtual int setAESKey(char* key);

	/* now we only can check Broadcom and Realtek wifi module*/
	virtual int checkWifiModule();

	/* now we only can check one wlan interface, such as: "wlan0" or "wlan1"*/
	virtual int checkWlanInterface();

	/* respond as the proto defined */
	virtual int respProto(char* resp) = 0;

	int doSystem(char* cmd,bool scan_process);
	int stopScanProcess(const char* process);

public:
	virtual int loop();

protected:
	char mAESKey[KEY_LEN+1];
	bool mIsUseAESKey;
	int mForkpid;
};

/* ---------------ProtoFactory------------------ */
class ProtoFactory {
public:
	static ProtoFactory* getInstance();

private:
	ProtoFactory():mUsing(false),mCurProto(NULL){};
	~ProtoFactory();

public:
	Proto* createProto(int proto);
    Proto* getCurProto();
	void releaseProto(Proto* p);

private:
	static ProtoFactory* instance;
	static locker *lock;
	bool mUsing;
	Proto* mCurProto;
};
#endif /* __PROTO_H__ */
