#ifndef __COOEE_H__
#define __COOEE_H__

#include "Proto.h"

#ifndef ANDROID_ENV
#define BROADCOM_COOEE "smartlinkd_setup"
#else
#define BROADCOM_COOEE "smartlinkd_setup"
#endif

class Cooee : public Proto {
public:
	Cooee();
	~Cooee();

public:
	int start();
    int stop();
	int respProto(char* resp);
};

#endif /* __COOEE_H__ */
