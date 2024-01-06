#include <stdio.h>
#include <stdlib.h>

#include "TinaWifiNetwork.h"
#include "log.h"

TinaWifiNetwork::TinaWifiNetwork(){};
TinaWifiNetwork::~TinaWifiNetwork(){};

int TinaWifiNetwork::startLink(struct _info *info){
	char cmdstr[150];
	char* key_mgmt;
	if(info->base_info.security == AW_SMARTLINKD_SECURITY_NONE)
		key_mgmt = "NONE";
	else if(info->base_info.security == AW_SMARTLINKD_SECURITY_WPA || info->base_info.security == AW_SMARTLINKD_SECURITY_WPA2)
		key_mgmt = "WPA-PSK";
	else
		key_mgmt = "WPA-EAP";
	sprintf(cmdstr,"%s %s %s %s %s",TINA_WIFI_CONFIG, \
									info->base_info.ssid, \
									info->base_info.password, \
									key_mgmt, \
									ifName);
	LOGD("sh %s\n",cmdstr);
	return system(cmdstr);
}

int TinaWifiNetwork::requestIP(){
	system("killall udhcpc");
	return system("udhcpc -i "ifName" -b");
}

int TinaWifiNetwork::configWifiInfo(){
	//TODO: this function will use to config the wifi configuration
    return 0;
}
