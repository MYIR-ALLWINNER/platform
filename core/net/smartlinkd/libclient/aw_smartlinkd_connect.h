#ifndef _CONNECT_H_
#define _CONNECT_H_

#include "smartlink_util.h"

#define IFNAME "wlan0"
#define CHECK_WLAN_SHELL "ifconfig | grep "IFNAME

#define IS_DATA 0
#define IS_CMD 1

#define MAX_DET_NUM 20

#ifdef __cplusplus
extern "C" {
#endif
const int THREAD_INIT = -100;
const int THREAD_EXIT = 1;
const int THREAD_CONTINUE = 0;

/*********** app client API ***********/
void aw_smartlinkd_prepare();
int aw_smartlinkd_init(int fd,int(f)(char*,int));

int aw_startairkiss();
int aw_startcooee();

int aw_stopairkiss();
int aw_stopcooee();

int aw_startadt();
int aw_stopadt();

int aw_startxrsc();
int aw_stopxrsc();

int aw_startcomposite(int proto_mask);
int aw_stopcomposite();

/*********** no use for app client ***********/
int aw_easysetupfinish(struct _cmd *c);

/***************** for debug *****************/
void printf_info(struct _cmd *c);

int check_wlan_interface();

int convert_adt_str_to_info(char ssid[], char passwd[], char *string);

#ifdef __cplusplus
}
#endif
#endif /* _CONNECT_H_ */
