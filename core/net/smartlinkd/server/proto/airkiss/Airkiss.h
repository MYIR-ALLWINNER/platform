#ifndef __AIRKISS_H__
#define __AIRKISS_H__

#include "Proto.h"

#ifndef ANDROID_ENV
#define BROADCOM_AIKISS "smartlinkd_setup"
#define REALTEK_AIKISS "rtw_ak"
#define XR819_AIKISS "smartlinkd_xrairkiss"
#else
#define BROADCOM_AIKISS "smartlinkd_setup"
#define XR819_AIKISS "smartlinkd_xrairkiss"
#define REALTEK_AIKISS "rtw_ak"
#endif
#define ACK_DEST_PORT   10000
#define SEND_ACK_TIMES  60

class Airkiss : public Proto {
public:
	Airkiss();
	~Airkiss();

public:
	int start();
	int stop();
	int sendProtoBoardcast(int random);
	int respProto(char* resp);
};

#endif /* __AIRKISS_H__ */
