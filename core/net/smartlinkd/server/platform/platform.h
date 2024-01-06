#ifndef __PLATFORM_H__
#define __PLATFORM_H__
#include "smartlink_util.h"
class OSWifiNetwork{
public:
	/**/
	virtual int startLink(struct _info *info) = 0;
	virtual int requestIP() = 0;
	virtual int configWifiInfo() = 0;
public:
	virtual ~OSWifiNetwork(){};
};
#endif
