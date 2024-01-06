#ifndef __TINA_WIFI_NETWORK_H__
#define __TINA_WIFI_NETWORK_H__

#include "smartlink_util.h"
#include "platform.h"
#define TINA_WIFI_CONFIG "/etc/wifi/wpa.sh"
#define ifName "wlan0"

class TinaWifiNetwork : public OSWifiNetwork {
public:
	TinaWifiNetwork();
	~TinaWifiNetwork();

public:
	int startLink(struct _info *info);
	int requestIP();
	int configWifiInfo();
};
#endif
