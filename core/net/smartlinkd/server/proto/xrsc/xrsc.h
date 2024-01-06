#ifndef __XRSC_H__
#define __XRSC_H__

#include "Proto.h"


#define SUNXI_XRSC "smartlinkd_xrsc"

class xrsc : public Proto {
public:
	xrsc();
	~xrsc();

public:
	int start();
    int stop();
	int respProto(char* resp);
};

#endif /* __COOEE_H__ */
