#ifndef __ADT_H__
#define __ADT_H__

#include "Proto.h"

#ifndef ANDROID_ENV
#define SUNXI_ADT "smartlinkd_soundwave"
#else
#define SUNXI_ADT "smartlinkd_setup"
#endif

class Adt : public Proto {
public:
	Adt();
	~Adt();

public:
	int start();
    int stop();
	int respProto(char* resp);
};

#endif /* __COOEE_H__ */
