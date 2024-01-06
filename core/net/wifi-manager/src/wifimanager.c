#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>

#include <wifi.h>
#include <wifi_event.h>
#include <wifi_state_machine.h>
#include <network_manager.h>
#include <wpa_supplicant_conf.h>
#include <wifi_intf.h>

#define WPA_SSID_LENTH  512

extern int is_ip_exist();

char netid_connecting[NET_ID_LEN+1] = {0};
int  disconnecting = 0;
int  connecting_ap_event_label = 0;
int  disconnect_ap_event_label = 0;
//tWIFI_STATE  gwifi_state = WIFIMG_WIFI_DISABLED;
extern tWIFI_STATE  gwifi_state;

static tWIFI_EVENT  event_code = WIFIMG_NO_NETWORK_CONNECTING;
static int aw_wifi_disconnect_ap(int event_label);
static char wpa_scan_ssid[WPA_SSID_LENTH];
static char wpa_conf_ssid[WPA_SSID_LENTH];
static int  ssid_contain_chinese = 0;

static int aw_wifi_add_event_callback(tWifi_event_callback pcb)
{
      return add_wifi_event_callback_inner(pcb);
}

/*
*get wifi connection state with AP
*return value:
*1: connected with AP(connected to network IPv4)
*2: connected with AP(connected to network IPv6)
*0: disconnected with AP
*/
static int aw_wifi_is_ap_connected(char *ssid, int *len)
{
    int ret = 0;
    tWIFI_MACHINE_STATE wifi_machine_state;

    if(gwifi_state == WIFIMG_WIFI_DISABLED){
        return -1;
    }

    wifi_machine_state = get_wifi_machine_state();
    if(wifi_machine_state != CONNECTED_STATE && wifi_machine_state != DISCONNECTED_STATE){
        printf("%s: wifi_busing, please try again later!\n", __func__);
        return -1;
    }

    /* pase scan thread */
    pause_wifi_scan_thread();

    if( 4 == wpa_conf_is_ap_connected(ssid, len))
	ret = 1;
    else if( 6 == wpa_conf_is_ap_connected(ssid, len))
	ret = 2;
    else
	ret = 0;

    /* resume scan thread */
    resume_wifi_scan_thread();

    return ret;
}


static int aw_wifi_scan(int event_label)
{
    int ret = 0;
    tWIFI_MACHINE_STATE wifi_machine_state;

    if(gwifi_state == WIFIMG_WIFI_DISABLED){
        return -1;
    }

    wifi_machine_state = get_wifi_machine_state();
    if(wifi_machine_state != CONNECTED_STATE && wifi_machine_state != DISCONNECTED_STATE){
        ret = -1;
        event_code = WIFIMG_DEV_BUSING_EVENT;
        goto end;
    }

    update_scan_results();

end:
    if(ret != WIFI_MANAGER_SUCCESS){
        call_event_callback_function(event_code, NULL, event_label);
    }

    return ret;
}

static int aw_wifi_get_scan_results(char *result, int *len)
{
    if(gwifi_state == WIFIMG_WIFI_DISABLED){
        return -1;
    }

    if(get_scan_results_inner(result, len) != 0)
    {
        printf("%s: There is a scan or scan_results error, Please try scan again later!\n", __func__);
        return -1;
    }
    else
        return 0;
}

/* check wpa/wpa2 passwd is right */
int check_wpa_passwd(const char *passwd)
{
    int result = 0;
    int i=0;

    if(!passwd || *passwd =='\0'){
        return 0;
    }

    for(i=0; passwd[i]!='\0'; i++){
        /* non printable char */
        if((passwd[i]<32) || (passwd[i] > 126)){
            result = 0;
            break;
        }
    }

    if(passwd[i] == '\0'){
        result = 1;
    }

    return result;
}

/* convert app ssid which contain chinese in utf-8 to wpa scan ssid */
static int ssid_app_to_wpa_scan(const char *app_ssid, char *scan_ssid)
{
    unsigned char h_val = 0, l_val = 0;
    int i = 0;
    int chinese_in = 0;

    if(!app_ssid || !app_ssid[0])
    {
        printf("Error: app ssid is NULL!\n");
        return -1;
    }

    if(!scan_ssid)
    {
        printf("Error: wpa ssid buf is NULL\n");
        return -1;
    }

    i = 0;
    while(app_ssid[i] != '\0')
    {
        /* ascii code */
        if((unsigned char)app_ssid[i] <= 0x7f)
        {
            *(scan_ssid++) = app_ssid[i++];
        }
        else /* covert to wpa ssid for chinese code */
        {
            *(scan_ssid++) = '\\';
            *(scan_ssid++) = 'x';
            h_val = (app_ssid[i] & 0xf0) >> 4;
            if((h_val >= 0) && (h_val <= 9)){
                *(scan_ssid++) = h_val + '0';
            }else if((h_val >= 0x0a) && (h_val <= 0x0f)){
                *(scan_ssid++) = h_val + 'a' - 0xa;
            }

            l_val = app_ssid[i] & 0x0f;
            if((l_val >= 0) && (l_val <= 9)){
                *(scan_ssid++) = l_val + '0';
            }else if((l_val >= 0x0a) && (l_val <= 0x0f)){
                *(scan_ssid++) = l_val + 'a' - 0xa;
            }
            i++;
            chinese_in = 1;
        }
    }
    *scan_ssid = '\0';

    if(chinese_in == 1){
        return 1;
    }

    return 0;
}

/* convert app ssid which contain chinese in utf-8 to wpa conf ssid */
static int ssid_app_to_wpa_conf(const char *app_ssid, char *conf_ssid)
{
    unsigned char h_val = 0, l_val = 0;
    int i = 0;
    //int chinese_in = 0;

    if(!app_ssid || !app_ssid[0])
    {
        printf("Error: app ssid is NULL!\n");
        return -1;
    }

    if(!conf_ssid)
    {
        printf("Error: wpa ssid buf is NULL\n");
        return -1;
    }

    i = 0;
    while(app_ssid[i] != '\0')
    {
        h_val = (app_ssid[i] & 0xf0) >> 4;
        if((h_val >= 0) && (h_val <= 9)){
            *(conf_ssid++) = h_val + '0';
        }else if((h_val >= 0x0a) && (h_val <= 0x0f)){
            *(conf_ssid++) = h_val + 'a' - 0xa;
        }

        l_val = app_ssid[i] & 0x0f;
        if((l_val >= 0) && (l_val <= 9)){
            *(conf_ssid++) = l_val + '0';
        }else if((l_val >= 0x0a) && (l_val <= 0x0f)){
            *(conf_ssid++) = l_val + 'a' - 0xa;
        }
        i++;
    }
    *conf_ssid = '\0';

    return 0;
}



/* connect visiable network */
static int aw_wifi_add_network(const char *ssid, tKEY_MGMT key_mgmt, const char *passwd, int event_label)
{
    int i=0, ret = -1, len = 0, max_prio = -1;
    char cmd[CMD_LEN+1] = {0};
    char reply[REPLY_BUF_SIZE] = {0}, netid1[NET_ID_LEN+1]={0}, netid2[NET_ID_LEN+1] = {0};
    int is_exist = 0;
    tWIFI_MACHINE_STATE wifi_machine_state;
    const char *p_ssid = NULL;
    tWIFI_MACHINE_STATE  state;
    tWIFI_EVENT_INNER    event;

    if(gwifi_state == WIFIMG_WIFI_DISABLED){
        return -1;
    }

    if(!ssid || !ssid[0]){
        printf("Error: ssid is NULL!\n");
        ret = -1;
        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        goto end;
    }

	/* pase scan thread */
    pause_wifi_scan_thread();

    wifi_machine_state = get_wifi_machine_state();
    if(wifi_machine_state != CONNECTED_STATE && wifi_machine_state != DISCONNECTED_STATE){
        ret = -1;
        event_code = WIFIMG_DEV_BUSING_EVENT;
        goto end;
    }

    /* connecting */
    set_wifi_machine_state(CONNECTING_STATE);

    /* set connecting event label */
    connecting_ap_event_label = event_label;

    /* remove disconnecting flag */
    disconnecting = 0;

    /* convert app ssid to wpa scan ssid */
    ret = ssid_app_to_wpa_scan(ssid, wpa_scan_ssid);
    if(ret < 0){
        ret = -1;
        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        goto end;
    }else if(ret > 0){
        ssid_contain_chinese = 1;
    }else {
        ssid_contain_chinese = 0;
    }

    /* has no chinese code */
    if(ssid_contain_chinese == 0){
        p_ssid = ssid;
    }else{
        ssid_app_to_wpa_conf(ssid, wpa_conf_ssid);
        p_ssid = wpa_conf_ssid;
    }

    /* check already exist or connected */
    len = NET_ID_LEN+1;
    is_exist = wpa_conf_is_ap_exist(p_ssid, key_mgmt, netid1, &len);

	/* add network */
	strncpy(cmd, "ADD_NETWORK", CMD_LEN);
	cmd[CMD_LEN] = '\0';
	ret = wifi_command(cmd, netid2, sizeof(netid2));
	if(ret){
		printf("do add network results error!\n");
		ret = -1;
		event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
		goto end;
	}

	/* set network ssid */
	if(ssid_contain_chinese == 0){
		sprintf(cmd, "SET_NETWORK %s ssid \"%s\"", netid2, p_ssid);
	}else{
		sprintf(cmd, "SET_NETWORK %s ssid %s", netid2, p_ssid);
	}

    ret = wifi_command(cmd, reply, sizeof(reply));
	if(ret){
		printf("do set network ssid error!\n");
		/* cancel saved in wpa_supplicant.conf */
		sprintf(cmd, "REMOVE_NETWORK %s", netid2);
		wifi_command(cmd, reply, sizeof(reply));
		/* save config */
		sprintf(cmd, "%s", "SAVE_CONFIG");
		wifi_command(cmd, reply, sizeof(reply));
		ret = -1;
		event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
		goto end;
	}

	/* no passwd */
	if (key_mgmt == WIFIMG_NONE){
		/* set network no passwd */
		sprintf(cmd, "SET_NETWORK %s key_mgmt NONE", netid2);
		ret = wifi_command(cmd, reply, sizeof(reply));
        if(ret){
		printf("do set network key_mgmt error!\n");
		/* cancel saved in wpa_supplicant.conf */
		sprintf(cmd, "REMOVE_NETWORK %s", netid2);
		wifi_command(cmd, reply, sizeof(reply));
		/* save config */
		sprintf(cmd, "%s", "SAVE_CONFIG");
		wifi_command(cmd, reply, sizeof(reply));
		ret = -1;
		event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
		goto end;
		}
	} else if(key_mgmt == WIFIMG_WPA_PSK || key_mgmt == WIFIMG_WPA2_PSK){
		/* set network psk passwd */
		sprintf(cmd,"SET_NETWORK %s key_mgmt WPA-PSK", netid2);
		ret = wifi_command(cmd, reply, sizeof(reply));
        if(ret){
            printf("do set network key_mgmt WPA-PSK error!\n");
            /* cancel saved in wpa_supplicant.conf */
            sprintf(cmd, "REMOVE_NETWORK %s", netid2);
            wifi_command(cmd, reply, sizeof(reply));
	        /* save config */
		    sprintf(cmd, "%s", "SAVE_CONFIG");
		    wifi_command(cmd, reply, sizeof(reply));
            ret = -1;
            event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
            goto end;
        }

        ret = check_wpa_passwd(passwd);
        if(ret == 0){
            printf("check wpa-psk passwd is error!\n");
		sprintf(cmd, "REMOVE_NETWORK %s", netid2);
            cmd[CMD_LEN] = '\0';
            wifi_command(cmd, reply, sizeof(reply));
			sprintf(cmd, "%s", "SAVE_CONFIG");
			wifi_command(cmd, reply, sizeof(reply));
            ret = -1;
            event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
            goto end;
        }

	    sprintf(cmd, "SET_NETWORK %s psk \"%s\"", netid2, passwd);
	    ret = wifi_command(cmd, reply, sizeof(reply));
        if(ret){
            printf("do set network psk error!\n");
            /* cancel saved in wpa_supplicant.conf */
            sprintf(cmd, "REMOVE_NETWORK %s", netid2);
            wifi_command(cmd, reply, sizeof(reply));
	        /* save config */
		    sprintf(cmd, "%s", "SAVE_CONFIG");
		    wifi_command(cmd, reply, sizeof(reply));
            ret = -1;
            event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
            goto end;
        }
	  } else if(key_mgmt == WIFIMG_WEP){
        /* set network  key_mgmt none */
	  sprintf(cmd, "SET_NETWORK %s key_mgmt NONE", netid2);
	  ret = wifi_command(cmd, reply, sizeof(reply));
        if(ret){
            printf("do set network key_mgmt none error!\n");
            /* cancel saved in wpa_supplicant.conf */
            sprintf(cmd, "REMOVE_NETWORK %s", netid2);
            wifi_command(cmd, reply, sizeof(reply));
	        /* save config */
		    sprintf(cmd, "%s", "SAVE_CONFIG");
		    wifi_command(cmd, reply, sizeof(reply));
            ret = -1;
            event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
            goto end;
        }

        /* set network wep_key0 */
	sprintf(cmd, "SET_NETWORK %s wep_key0 %s", netid2, passwd);
	ret = wifi_command(cmd, reply, sizeof(reply));
        if(ret){
            sprintf(cmd, "SET_NETWORK %s wep_key0 \"%s\"", netid2, passwd);
            ret = wifi_command(cmd, reply, sizeof(reply));
            if(ret){
                printf("do set network wep_key0 error!\n");
				/* cancel saved in wpa_supplicant.conf */
				sprintf(cmd, "REMOVE_NETWORK %s", netid2);
				wifi_command(cmd, reply, sizeof(reply));
				/* save config */
				sprintf(cmd, "%s", "SAVE_CONFIG");
				wifi_command(cmd, reply, sizeof(reply));
                ret = -1;
                event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
                goto end;
            }
        }

        /* set network auth_alg */
	  sprintf(cmd, "SET_NETWORK %s auth_alg OPEN SHARED", netid2);
	  ret = wifi_command(cmd, reply, sizeof(reply));
        if(ret){
            printf("do set network auth_alg error!\n");
            /* cancel saved in wpa_supplicant.conf */
            sprintf(cmd, "REMOVE_NETWORK %s", netid2);
            wifi_command(cmd, reply, sizeof(reply));
	        /* save config */
		    sprintf(cmd, "%s", "SAVE_CONFIG");
		    wifi_command(cmd, reply, sizeof(reply));
            ret = -1;
            event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
            goto end;
        }

	  } else {
	      printf("Error: key mgmt not support!\n");
		  /* cancel saved in wpa_supplicant.conf */
		  sprintf(cmd, "REMOVE_NETWORK %s", netid2);
		  wifi_command(cmd, reply, sizeof(reply));
		  /* save config */
		  sprintf(cmd, "%s", "SAVE_CONFIG");
		  wifi_command(cmd, reply, sizeof(reply));
	      ret = -1;
	      event_code = WIFIMG_KEY_MGMT_NOT_SUPPORT;
	      goto end;
	  }

	/* set scan_ssid to 1 for network */
    sprintf(cmd,"SET_NETWORK %s scan_ssid 1", netid2);
    ret = wifi_command(cmd, reply, sizeof(reply));
    if(ret){
        printf("do set scan_ssid error!\n");
		/* cancel saved in wpa_supplicant.conf */
		sprintf(cmd, "REMOVE_NETWORK %s", netid2);
		wifi_command(cmd, reply, sizeof(reply));
		/* save config */
		sprintf(cmd, "%s", "SAVE_CONFIG");
		wifi_command(cmd, reply, sizeof(reply));
        ret = -1;
        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        goto end;
    }

	  /* get max priority in wpa_supplicant.conf */
    max_prio =  wpa_conf_get_max_priority();

    /* set priority for network */
    sprintf(cmd,"SET_NETWORK %s priority %d", netid2, (max_prio+1));
    ret = wifi_command(cmd, reply, sizeof(reply));
    if(ret){
        printf("do set priority error!\n");
        /* cancel saved in wpa_supplicant.conf */
        sprintf(cmd, "REMOVE_NETWORK %s", netid2);
        wifi_command(cmd, reply, sizeof(reply));
		/* save config */
		sprintf(cmd, "%s", "SAVE_CONFIG");
		wifi_command(cmd, reply, sizeof(reply));
        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        ret = -1;
        goto end;
    }

	/* select network */
	sprintf(cmd, "SELECT_NETWORK %s", netid2);
	ret = wifi_command(cmd, reply, sizeof(reply));
    if(ret){
        printf("do select network error!\n");
		/* cancel saved in wpa_supplicant.conf */
		sprintf(cmd, "REMOVE_NETWORK %s", netid2);
		wifi_command(cmd, reply, sizeof(reply));
		/* save config */
		sprintf(cmd, "%s", "SAVE_CONFIG");
		wifi_command(cmd, reply, sizeof(reply));
        ret = -1;
        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        goto end;
    }

    /* save config */
	  sprintf(cmd, "%s", "SAVE_CONFIG");
	  ret = wifi_command(cmd, reply, sizeof(reply));
    if(ret){
        printf("do save config error!\n");
		/* cancel saved in wpa_supplicant.conf */
		sprintf(cmd, "REMOVE_NETWORK %s", netid2);
		wifi_command(cmd, reply, sizeof(reply));
		/* save config */
		sprintf(cmd, "%s", "SAVE_CONFIG");
		wifi_command(cmd, reply, sizeof(reply));
        ret = -1;
        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        goto end;
    }

    /* save netid */
    strcpy(netid_connecting, netid2);

    /* wait for check status connected/disconnected */
    reset_assoc_reject_count();
    i = 0;
    do {
        usleep(200000);

        state = get_wifi_machine_state();
        event = get_cur_wifi_event();
        /* password incorrect*/
        if ((state == DISCONNECTED_STATE) && (event == PASSWORD_INCORRECT)){
						printf("wifi_connect_ap_inner: password failed!\n");
            break;
        }

		if(get_assoc_reject_count() >= MAX_ASSOC_REJECT_COUNT){
			reset_assoc_reject_count();
			printf("wifi_connect_ap_inner: assoc reject %d times\n", MAX_ASSOC_REJECT_COUNT);
			break;
		}

        i++;
    } while((state != L2CONNECTED_STATE) && (state != CONNECTED_STATE) && (i < 225));

	if (state == CONNECTING_STATE) { /* It can't connect AP */
		/* stop connect */
		sprintf(cmd, "%s", "DISCONNECT");
		wifi_command(cmd, reply, sizeof(reply));
		set_wifi_machine_state(DISCONNECTED_STATE);
		set_cur_wifi_event(CONNECT_AP_TIMEOUT);

		/* disable network in wpa_supplicant.conf */
		sprintf(cmd, "DISABLE_NETWORK %s", netid2);
		wifi_command(cmd, reply, sizeof(reply));

		/* cancel saved in wpa_supplicant.conf */
		sprintf(cmd, "REMOVE_NETWORK %s", netid2);
		wifi_command(cmd, reply, sizeof(reply));

		/* save config */
		sprintf(cmd, "%s", "SAVE_CONFIG");
		wifi_command(cmd, reply, sizeof(reply));

		ret = -1;
		event_code = WIFIMG_NETWORK_NOT_EXIST;
		goto end;
	}else if(state == DISCONNECTED_STATE){ /* Errot when connecting */
        if (event == PASSWORD_INCORRECT) {
            /* disable network in wpa_supplicant.conf */
            sprintf(cmd, "DISABLE_NETWORK %s", netid2);
            wifi_command(cmd, reply, sizeof(reply));

            /* cancel saved in wpa_supplicant.conf */
            sprintf(cmd, "REMOVE_NETWORK %s", netid2);
            wifi_command(cmd, reply, sizeof(reply));

	        /* save config */
		    sprintf(cmd, "%s", "SAVE_CONFIG");
		    wifi_command(cmd, reply, sizeof(reply));

            ret = -1;
            event_code = WIFIMG_PASSWORD_FAILED;
            goto end;
        }else if(event == OBTAINING_IP_TIMEOUT){
            if(is_exist == 1 || is_exist == 3){
		    //network is exist or connected
		    sprintf(cmd, "REMOVE_NETWORK %s", netid1);
                cmd[CMD_LEN] = '\0';
                wifi_command(cmd, reply, sizeof(reply));
	        }

            /* save config */
		    sprintf(cmd, "%s", "SAVE_CONFIG");
		    wifi_command(cmd, reply, sizeof(reply));
            ret = 0;
        }else{
            ;
        }
    }else if(state == L2CONNECTED_STATE || state == CONNECTED_STATE){ /* connect ap l2 */
        if(is_exist == 1 || is_exist == 3){
	    //network is exist or connected
	    sprintf(cmd, "REMOVE_NETWORK %s", netid1);
            cmd[CMD_LEN] = '\0';
            wifi_command(cmd, reply, sizeof(reply));
        }

        /* save config */
		sprintf(cmd, "%s", "SAVE_CONFIG");
		wifi_command(cmd, reply, sizeof(reply));
        ret = 0;
    }else{
        ;
    }

end:
    //enable all networks in wpa_supplicant.conf
    wpa_conf_enable_all_networks();

	/* resume scan thread */
    resume_wifi_scan_thread();

	//restore state when call wrong
    if(ret != 0){
        set_wifi_machine_state(DISCONNECTED_STATE);
        call_event_callback_function(event_code, NULL, event_label);
    }

    return ret;
}

static int wifi_connect_ap_inner(const char *ssid, tKEY_MGMT key_mgmt, const char *passwd, int event_label)
{
    int i=0, ret = -1, len = 0, max_prio = -1;
    char cmd[CMD_LEN+1] = {0};
    char reply[REPLY_BUF_SIZE] = {0}, netid1[NET_ID_LEN+1]={0}, netid2[NET_ID_LEN+1] = {0};
    int is_exist = 0;
    tWIFI_MACHINE_STATE  state;
    tWIFI_EVENT_INNER    event;

    /* connecting */
    set_wifi_machine_state(CONNECTING_STATE);

    /* set connecting event label */
    connecting_ap_event_label = event_label;

    /* remove disconnecting flag */
    disconnecting = 0;

    /* check already exist or connected */
    len = NET_ID_LEN+1;
    is_exist = wpa_conf_is_ap_exist(ssid, key_mgmt, netid1, &len);

    /* add network */
    strncpy(cmd, "ADD_NETWORK", CMD_LEN);
    cmd[CMD_LEN] = '\0';
    ret = wifi_command(cmd, netid2, sizeof(netid2));
    if(ret){
        printf("do add network results error!\n");
        ret = -1;
        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        goto end;
    }

	  /* set network ssid */
    if(ssid_contain_chinese == 0){
        sprintf(cmd, "SET_NETWORK %s ssid \"%s\"", netid2, ssid);
    }else{
        sprintf(cmd, "SET_NETWORK %s ssid %s", netid2, ssid);
    }

	ret = wifi_command(cmd, reply, sizeof(reply));
    if(ret){
        printf("do set network ssid error!\n");

        /* cancel saved in wpa_supplicant.conf */
        sprintf(cmd, "REMOVE_NETWORK %s", netid2);
        wifi_command(cmd, reply, sizeof(reply));

        ret = -1;
        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        goto end;
    }

	  /* no passwd */
	  if (key_mgmt == WIFIMG_NONE){
	      /* set network no passwd */
	  sprintf(cmd, "SET_NETWORK %s key_mgmt NONE", netid2);
	  ret = wifi_command(cmd, reply, sizeof(reply));
        if(ret){
            printf("do set network key_mgmt error!\n");

            /* cancel saved in wpa_supplicant.conf */
            sprintf(cmd, "REMOVE_NETWORK %s", netid2);
            wifi_command(cmd, reply, sizeof(reply));

            event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
            ret = -1;
            goto end;
        }
	  } else if(key_mgmt == WIFIMG_WPA_PSK || key_mgmt == WIFIMG_WPA2_PSK){
	      /* set network psk passwd */
	      sprintf(cmd,"SET_NETWORK %s key_mgmt WPA-PSK", netid2);
	      ret = wifi_command(cmd, reply, sizeof(reply));
        if(ret){
            printf("do set network key_mgmt WPA-PSK error!\n");

            /* cancel saved in wpa_supplicant.conf */
            sprintf(cmd, "REMOVE_NETWORK %s", netid2);
            wifi_command(cmd, reply, sizeof(reply));

            ret = -1;
            event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
            goto end;
        }

        ret = check_wpa_passwd(passwd);
        if(ret == 0){
            printf("check wpa-psk passwd is error!\n");

            /* cancel saved in wpa_supplicant.conf */
            sprintf(cmd, "REMOVE_NETWORK %s", netid2);
            wifi_command(cmd, reply, sizeof(reply));

            ret = -1;
            event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
            goto end;
        }

	  sprintf(cmd, "SET_NETWORK %s psk \"%s\"", netid2, passwd);
	  ret = wifi_command(cmd, reply, sizeof(reply));
        if(ret){
            printf("do set network psk error!\n");

            /* cancel saved in wpa_supplicant.conf */
            sprintf(cmd, "REMOVE_NETWORK %s", netid2);
            wifi_command(cmd, reply, sizeof(reply));

            ret = -1;
            event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
            goto end;
        }
	  } else if(key_mgmt == WIFIMG_WEP){
        /* set network  key_mgmt none */
	  sprintf(cmd, "SET_NETWORK %s key_mgmt NONE", netid2);
	  ret = wifi_command(cmd, reply, sizeof(reply));
        if(ret){
            printf("do set network key_mgmt none error!\n");

            /* cancel saved in wpa_supplicant.conf */
            sprintf(cmd, "REMOVE_NETWORK %s", netid2);
            wifi_command(cmd, reply, sizeof(reply));

            ret = -1;
            event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
            goto end;
        }

        /* set network wep_key0 */
	sprintf(cmd, "SET_NETWORK %s wep_key0 %s", netid2, passwd);
	ret = wifi_command(cmd, reply, sizeof(reply));
        if(ret){
            sprintf(cmd, "SET_NETWORK %s wep_key0 \"%s\"", netid2, passwd);
            ret = wifi_command(cmd, reply, sizeof(reply));
            if(ret){
                printf("do set network wep_key0 error!\n");

                /* cancel saved in wpa_supplicant.conf */
                sprintf(cmd, "REMOVE_NETWORK %s", netid2);
                wifi_command(cmd, reply, sizeof(reply));

                ret = -1;
                event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
                goto end;
            }
        }

        /* set network auth_alg */
	  sprintf(cmd, "SET_NETWORK %s auth_alg OPEN SHARED", netid2);
	  ret = wifi_command(cmd, reply, sizeof(reply));
        if(ret){
            printf("do set network auth_alg error!\n");

            /* cancel saved in wpa_supplicant.conf */
            sprintf(cmd, "REMOVE_NETWORK %s", netid2);
            wifi_command(cmd, reply, sizeof(reply));

            ret = -1;
            event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
            goto end;
        }

	  } else {
	      printf("Error: key mgmt is not support!\n");

          /* cancel saved in wpa_supplicant.conf */
          sprintf(cmd, "REMOVE_NETWORK %s", netid2);
          wifi_command(cmd, reply, sizeof(reply));

          ret = -1;
	      event_code = WIFIMG_KEY_MGMT_NOT_SUPPORT;
	      goto end;
	  }

	  /* get max priority in wpa_supplicant.conf */
    max_prio =  wpa_conf_get_max_priority();

    /* set priority for network */
    sprintf(cmd,"SET_NETWORK %s priority %d", netid2, (max_prio+1));
    ret = wifi_command(cmd, reply, sizeof(reply));
    if(ret){
        printf("do set priority error!\n");

        /* cancel saved in wpa_supplicant.conf */
        sprintf(cmd, "REMOVE_NETWORK %s", netid2);
        wifi_command(cmd, reply, sizeof(reply));

        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        ret = -1;
        goto end;
    }

	  /* select network */
	  sprintf(cmd, "SELECT_NETWORK %s", netid2);
	  ret = wifi_command(cmd, reply, sizeof(reply));
    if(ret){
        printf("do select network error!\n");

        /* cancel saved in wpa_supplicant.conf */
        sprintf(cmd, "REMOVE_NETWORK %s", netid2);
        wifi_command(cmd, reply, sizeof(reply));

        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        ret = -1;
        goto end;
    }

    /* save netid */
    strcpy(netid_connecting, netid2);

    /* reconnect */
	  sprintf(cmd, "%s", "RECONNECT");
	  ret = wifi_command(cmd, reply, sizeof(reply));
    if(ret){
        printf("do reconnect network error!\n");

        /* cancel saved in wpa_supplicant.conf */
        sprintf(cmd, "REMOVE_NETWORK %s", netid2);
        wifi_command(cmd, reply, sizeof(reply));

        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        ret = -1;
        goto end;
    }

	reset_assoc_reject_count();

    /* wait for check status connected/disconnected */
    i = 0;
    do {
        usleep(200000);

        state = get_wifi_machine_state();
        event = get_cur_wifi_event();
        /* password incorrect*/
        if ((state == DISCONNECTED_STATE) && (event == PASSWORD_INCORRECT)){
		  printf("wifi_connect_ap_inner: password failed!\n");
            break;
        }

		if(get_assoc_reject_count() >= MAX_ASSOC_REJECT_COUNT){
			reset_assoc_reject_count();
			printf("wifi_connect_ap_inner: assoc reject %d times\n", MAX_ASSOC_REJECT_COUNT);
			break;
		}

        i++;
    } while((state != L2CONNECTED_STATE) && (state != CONNECTED_STATE) && (i < 225));

	if (state == CONNECTING_STATE) { /* It can't connect AP */
		/* stop connect */
		sprintf(cmd, "%s", "DISCONNECT");
		wifi_command(cmd, reply, sizeof(reply));
		set_wifi_machine_state(DISCONNECTED_STATE);
		set_cur_wifi_event(CONNECT_AP_TIMEOUT);

		/* disable network in wpa_supplicant.conf */
		sprintf(cmd, "DISABLE_NETWORK %s", netid2);
		wifi_command(cmd, reply, sizeof(reply));

		/* cancel saved in wpa_supplicant.conf */
		sprintf(cmd, "REMOVE_NETWORK %s", netid2);
		wifi_command(cmd, reply, sizeof(reply));

		/* save config */
		sprintf(cmd, "%s", "SAVE_CONFIG");
		wifi_command(cmd, reply, sizeof(reply));

		ret = -1;
		printf("connect ap inner:still connecting!\n");
		event_code = WIFIMG_NETWORK_NOT_EXIST;
		goto end;
	}else if(state == DISCONNECTED_STATE){ /* Errot when connecting */
        if (event == PASSWORD_INCORRECT) {
            /* disable network in wpa_supplicant.conf */
            sprintf(cmd, "DISABLE_NETWORK %s", netid2);
            wifi_command(cmd, reply, sizeof(reply));

            /* cancel saved in wpa_supplicant.conf */
            sprintf(cmd, "REMOVE_NETWORK %s", netid2);
            wifi_command(cmd, reply, sizeof(reply));

	        /* save config */
		    sprintf(cmd, "%s", "SAVE_CONFIG");
		    wifi_command(cmd, reply, sizeof(reply));

            ret = -1;
            event_code = WIFIMG_PASSWORD_FAILED;
			printf("connect ap inner:passwd failed!\n");
            goto end;
        }else if(event == OBTAINING_IP_TIMEOUT){
            if(is_exist == 1 || is_exist == 3){
		      //network is exist or connected
		      sprintf(cmd, "REMOVE_NETWORK %s", netid1);
                cmd[CMD_LEN] = '\0';
                wifi_command(cmd, reply, sizeof(reply));
	        }

            /* save config */
		    sprintf(cmd, "%s", "SAVE_CONFIG");
		    wifi_command(cmd, reply, sizeof(reply));
			printf("connect ap inner:obtian IP time out!\n");
            ret = 0;
        }else{
            event_code = WIFIMG_NETWORK_NOT_EXIST;
            ret = -1;
        }
    }else if(state == L2CONNECTED_STATE || state == CONNECTED_STATE){
        if(is_exist == 1 || is_exist == 3){
	      //network is exist or connected
	      sprintf(cmd, "REMOVE_NETWORK %s", netid1);
            cmd[CMD_LEN] = '\0';
            wifi_command(cmd, reply, sizeof(reply));
        }

        /* save config */
		sprintf(cmd, "%s", "SAVE_CONFIG");
		wifi_command(cmd, reply, sizeof(reply));
		printf("wifi connected in inner!\n");
        ret = 0;
    }else{
        event_code = WIFIMG_NETWORK_NOT_EXIST;
        ret = -1;
    }

end:
    //enable all networks in wpa_supplicant.conf
    wpa_conf_enable_all_networks();

    //restore state when call wrong
    if(ret != 0){
        set_wifi_machine_state(DISCONNECTED_STATE);
        return ret;
    }

    return ret;
}

/* connect visiable network */
static int aw_wifi_connect_ap_key_mgmt(const char *ssid, tKEY_MGMT key_mgmt, const char *passwd, int event_label)
{
	  int ret = -1, key[4] = {0};
	  tWIFI_MACHINE_STATE  state;
	  const char *p_ssid = NULL;

	  if(gwifi_state == WIFIMG_WIFI_DISABLED){
        return -1;
    }

	  if(!ssid || !ssid[0]){
	      printf("Error: ssid is NULL!\n");
	      ret = -1;
	      event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
	      goto end;
	  }

	  state = get_wifi_machine_state();
	  if(state != CONNECTED_STATE && state != DISCONNECTED_STATE){
        ret = -1;
        event_code = WIFIMG_DEV_BUSING_EVENT;
        goto end;
    }

    /* convert app ssid to wpa scan ssid */
    ret = ssid_app_to_wpa_scan(ssid, wpa_scan_ssid);
    if(ret < 0){
        ret = -1;
        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        goto end;
    }else if(ret > 0){
        ssid_contain_chinese = 1;
    }else {
        ssid_contain_chinese = 0;
    }

    /* has no chinese code */
    if(ssid_contain_chinese == 0){
        p_ssid = ssid;
    }else{
        ssid_app_to_wpa_conf(ssid, wpa_conf_ssid);
        p_ssid = wpa_conf_ssid;
    }

    /* checking network exist at first time */
    get_key_mgmt(wpa_scan_ssid, key);

	  /* no password */
    if (key_mgmt == WIFIMG_NONE){
        if(key[0] == 0){
            update_scan_results();
            get_key_mgmt(wpa_scan_ssid, key);
            if(key[0] == 0){
                ret = -1;
                event_code = WIFIMG_NETWORK_NOT_EXIST;
                goto end;
            }
        }
	  }else if(key_mgmt == WIFIMG_WPA_PSK || key_mgmt == WIFIMG_WPA2_PSK){
        if(key[1] == 0){
            update_scan_results();
            get_key_mgmt(wpa_scan_ssid, key);
            if(key[1] == 0){
                ret = -1;
                event_code = WIFIMG_NETWORK_NOT_EXIST;
                goto end;
            }
        }
    }else if(key_mgmt == WIFIMG_WEP){
        if(key[2] == 0){
            update_scan_results();
            get_key_mgmt(wpa_scan_ssid, key);
            if(key[2] == 0){
                ret = -1;
                event_code = WIFIMG_NETWORK_NOT_EXIST;
                goto end;
            }
        }
    }else{
        ret = -1;
        event_code = WIFIMG_KEY_MGMT_NOT_SUPPORT;
        goto end;
    }

    /* pause scan thread */
    pause_wifi_scan_thread();

    ret = wifi_connect_ap_inner(p_ssid, key_mgmt, passwd, event_label);

end:
    if(ret != 0){
        call_event_callback_function(event_code, NULL, event_label);
    }

    /* resume scan thread */
    resume_wifi_scan_thread();

    return ret;
}


static int aw_wifi_connect_ap(const char *ssid, const char *passwd, int event_label)
{
    //int i = 0;
	int ret = 0;
    int  key[4] = {0};
    tWIFI_MACHINE_STATE  state;
    //tWIFI_EVENT_INNER    event;
    const char *p_ssid = NULL;

    if(gwifi_state == WIFIMG_WIFI_DISABLED){
        printf("aw wifi connect ap wifi disabled\n");
        return -1;
    }

    if(!ssid || !ssid[0]){
        printf("Error: ssid is NULL!\n");
        ret = -1;
        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        goto end;
    }

    state = get_wifi_machine_state();
    printf("aw wifi connect state 0x%x\n", state);
    if(state != CONNECTED_STATE && state != DISCONNECTED_STATE){
        ret = -1;
        printf("aw wifi connect ap dev busing\n");
        event_code = WIFIMG_DEV_BUSING_EVENT;
        goto end;
    }

     /* convert app ssid to wpa scan ssid */
    ret = ssid_app_to_wpa_scan(ssid, wpa_scan_ssid);
    if(ret < 0){
        ret = -1;
        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        goto end;
    }else if(ret > 0){
        ssid_contain_chinese = 1;
    }else {
        ssid_contain_chinese = 0;
    }

    /* has no chinese code */
    if(ssid_contain_chinese == 0){
        p_ssid = ssid;
    }else{
        ssid_app_to_wpa_conf(ssid, wpa_conf_ssid);
        p_ssid = wpa_conf_ssid;
    }

    /* checking network exist at first time */
    get_key_mgmt(p_ssid, key);

    /* no password */
    if(!passwd || !passwd[0]){
        if(key[0] == 0){
            update_scan_results();
            get_key_mgmt(p_ssid, key);
            if(key[0] == 0){
                ret = -1;
                event_code = WIFIMG_NETWORK_NOT_EXIST;
                goto end;
            }
        }

        /* pase scan thread */
        pause_wifi_scan_thread();

        ret = wifi_connect_ap_inner(p_ssid, WIFIMG_NONE, passwd, event_label);
    }else{
        if((key[1] == 0) && (key[2] == 0)){
            update_scan_results();
            get_key_mgmt(wpa_scan_ssid, key);
            if((key[1] == 0) && (key[2] == 0)){
                ret = -1;
                event_code = WIFIMG_NETWORK_NOT_EXIST;
                goto end;
            }
        }

        /* pause scan thread */
        pause_wifi_scan_thread();

        /* wpa-psk */
        if(key[1] == 1){
            /* try WPA-PSK */
            ret = wifi_connect_ap_inner(p_ssid, WIFIMG_WPA_PSK, passwd, event_label);
            if(ret == 0){
                goto end;
            }
        }

        /* wep */
        if(key[2] == 1){
        /* try WEP */
            ret = wifi_connect_ap_inner(p_ssid, WIFIMG_WEP, passwd, event_label);
        }
    }

end:
    if(ret != 0){
        call_event_callback_function(event_code, NULL, event_label);
    }

    /* resume scan thread */
    resume_wifi_scan_thread();

    return ret;
}


static int aw_wifi_connect_ap_with_netid(const char *net_id, int event_label)
{

int i=0, ret = -1;
//int len = 0;
char cmd[CMD_LEN+1] = {0};
char reply[REPLY_BUF_SIZE] = {0};
tWIFI_MACHINE_STATE wifi_machine_state;
//const char *p_ssid = NULL;
tWIFI_MACHINE_STATE  state;
tWIFI_EVENT_INNER    event;

if(gwifi_state == WIFIMG_WIFI_DISABLED){
	return -1;
}


wifi_machine_state = get_wifi_machine_state();
if(wifi_machine_state != CONNECTED_STATE && wifi_machine_state != DISCONNECTED_STATE){
	ret = -1;
	event_code = WIFIMG_DEV_BUSING_EVENT;
	goto end;
}

/*disconnect*/
wifi_machine_state = get_wifi_machine_state();
if(wifi_machine_state == CONNECTED_STATE){
	aw_wifi_disconnect_ap(0x7fffffff);
}

/* pase scan thread */
pause_wifi_scan_thread();

/* connecting */
set_wifi_machine_state(CONNECTING_STATE);

/* set connecting event label */
connecting_ap_event_label = event_label;

/* remove disconnecting flag */
disconnecting = 0;

	/* selected_network */
		sprintf(cmd, "SELECT_NETWORK %s", net_id);
		ret = wifi_command(cmd, reply, sizeof(reply));
		if(ret){
			printf("do selected network error!\n");
			ret = -1;
			event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
			goto end;
		}

	 /* save netid */
	 strcpy(netid_connecting, net_id);

     /*reconnect*/
	strncpy(cmd, "RECONNECT", CMD_LEN);
       cmd[CMD_LEN] = '\0';
	ret = wifi_command(cmd, reply, sizeof(reply));
	if(ret){
		printf("do reconnect error!\n");
		ret = -1;
		event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
	}



	/* wait for check status connected/disconnected */
	 reset_assoc_reject_count();
	 i = 0;
	 do {
		 usleep(200000);

		 state = get_wifi_machine_state();
		 event = get_cur_wifi_event();
		 /* password incorrect*/
		 if ((state == DISCONNECTED_STATE) && (event == PASSWORD_INCORRECT)){
						 printf("wifi_connect_ap_inner: password failed!\n");
			 break;
		 }

		 if(get_assoc_reject_count() >= MAX_ASSOC_REJECT_COUNT){
			 reset_assoc_reject_count();
			 printf("wifi_connect_ap_inner: assoc reject %d times\n", MAX_ASSOC_REJECT_COUNT);
			 break;
		 }

		 i++;
	 } while((state != L2CONNECTED_STATE) && (state != CONNECTED_STATE) && (i < 225));

	 if (state == CONNECTING_STATE) { /* It can't connect AP */
		 /* stop connect */
		 sprintf(cmd, "%s", "DISCONNECT");
		 wifi_command(cmd, reply, sizeof(reply));
		 set_wifi_machine_state(DISCONNECTED_STATE);
		 set_cur_wifi_event(CONNECT_AP_TIMEOUT);
		 /* disable network in wpa_supplicant.conf */
		 sprintf(cmd, "DISABLE_NETWORK %s", net_id);
		 wifi_command(cmd, reply, sizeof(reply));
		 ret = -1;
		 event_code = WIFIMG_NETWORK_NOT_EXIST;
		 goto end;
	 }else if(state == DISCONNECTED_STATE){ /* Errot when connecting */
		 if (event == PASSWORD_INCORRECT) {
			 /* disable network in wpa_supplicant.conf */
			 sprintf(cmd, "DISABLE_NETWORK %s", net_id);
			 wifi_command(cmd, reply, sizeof(reply));
			 ret = -1;
			 event_code = WIFIMG_PASSWORD_FAILED;
			 goto end;
		 }else if(event == OBTAINING_IP_TIMEOUT){
			 ret = 0;
		 }else{
			 ;
		 }
	 }else if(state == L2CONNECTED_STATE || state == CONNECTED_STATE){
		 ret = 0;
	 }else{
		 ;
	 }

end:
	if(ret != 0){
	call_event_callback_function(event_code, NULL, event_label);
	}

   /* resume scan thread */
    resume_wifi_scan_thread();

	return ret;


}




/* cancel saved AP in wpa_supplicant.conf */
static int aw_wifi_remove_network(char *ssid, tKEY_MGMT key_mgmt)
{
    int ret = -1, len = 0;
    char cmd[CMD_LEN+1] = {0};
    char reply[REPLY_BUF_SIZE] = {0};
    char net_id[NET_ID_LEN+1] = {0};
    tWIFI_MACHINE_STATE wifi_machine_state;

    if(gwifi_state == WIFIMG_WIFI_DISABLED){
        return -1;
    }

    if(!ssid || !ssid[0]){
        printf("Error: ssid is null!\n");
        return -1;
    }

    wifi_machine_state = get_wifi_machine_state();
    if(wifi_machine_state != CONNECTED_STATE && wifi_machine_state != DISCONNECTED_STATE){
        printf("%s: wifi_busing, please try again later!\n", __func__);
        return -1;
    }

    /* pause scan thread */
     pause_wifi_scan_thread();

    /* check AP is exist in wpa_supplicant.conf */
    len = NET_ID_LEN+1;
    ret = wpa_conf_ssid2netid(ssid, key_mgmt, net_id, &len);
    if(ret <= 0){
        printf("Warning: %s is not in wpa_supplicant.conf!\n", ssid);
        return 0;
    }
    else if(!(ret & (0x01<<1) ))
    {
	 printf("Warning: %s exists in wpa_supplicant.conf, but the key_mgmt is not accordant!\n", ssid);
	 return 0;
    }

    /* cancel saved in wpa_supplicant.conf */
    sprintf(cmd, "REMOVE_NETWORK %s", net_id);
    ret = wifi_command(cmd, reply, sizeof(reply));
    if(ret){
        printf("do remove network %s error!\n", net_id);
        return -1;
    }

    /* save config */
	  sprintf(cmd, "%s", "SAVE_CONFIG");
	  ret = wifi_command(cmd, reply, sizeof(reply));
    if(ret){
        printf("do save config error!\n");
        return -1;
    }

    /* resume scan thread */
    resume_wifi_scan_thread();

    return 0;
}

static int aw_wifi_remove_all_networks()
{
    int ret = -1;
    tWIFI_MACHINE_STATE wifi_machine_state;

    if(gwifi_state == WIFIMG_WIFI_DISABLED){
        return -1;
    }

    wifi_machine_state = get_wifi_machine_state();
    if(wifi_machine_state != CONNECTED_STATE && wifi_machine_state != DISCONNECTED_STATE){
        printf("%s: wifi_busing, please try again later!\n", __func__);
        return -1;
    }

    pause_wifi_scan_thread();
    ret = wpa_conf_remove_all_networks();
    resume_wifi_scan_thread();
    return ret;
}

static int aw_wifi_connect_ap_auto(int event_label)
{
    //int i=0;
	int ret = -1;
	//int len = 0;
    char cmd[CMD_LEN+1] = {0}, reply[REPLY_BUF_SIZE] = {0};
    //char netid[NET_ID_LEN+1]={0};
    tWIFI_MACHINE_STATE wifi_machine_state;

    if(gwifi_state == WIFIMG_WIFI_DISABLED){
        return -1;
    }

    /* pase scan thread */
    pause_wifi_scan_thread();

    wifi_machine_state = get_wifi_machine_state();
    if(wifi_machine_state == CONNECTED_STATE){
        ret = -1;
        event_code = WIFIMG_OPT_NO_USE_EVENT;
        goto end;
    }

	if(wifi_machine_state != DISCONNECTED_STATE){
        ret = -1;
        event_code = WIFIMG_DEV_BUSING_EVENT;
        goto end;
    }

    /* check network exist in wpa_supplicant.conf */
    if(wpa_conf_network_info_exist() == 0){
        ret = -1;
        event_code = WIFIMG_NO_NETWORK_CONNECTING;
        goto end;
    }

    /* connecting */
    set_wifi_machine_state(CONNECTING_STATE);

	netid_connecting[0] = '\0';
    disconnecting = 0;
    connecting_ap_event_label = event_label;

    /* reconnected */
	sprintf(cmd, "%s", "RECONNECT");
    ret = wifi_command(cmd, reply, sizeof(reply));
    if(ret){
        printf("do reconnect error!\n");
        ret = -1;
        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
    }

    /* check timeout */
    start_check_connect_timeout(0);

end:
    if(ret != 0){
	  call_event_callback_function(event_code, NULL, event_label);
    }

    /* resume scan thread */
    resume_wifi_scan_thread();

    return ret;
}

static int aw_wifi_disconnect_ap(int event_label)
{
    int i=0, ret = -1, len = 0;
    char cmd[CMD_LEN+1] = {0}, reply[REPLY_BUF_SIZE] = {0};
    char netid[NET_ID_LEN+1]={0};
    tWIFI_MACHINE_STATE wifi_machine_state;

    if(gwifi_state == WIFIMG_WIFI_DISABLED){
        return -1;
    }

    disconnecting = 1;

    /* pase scan thread */
    pause_wifi_scan_thread();

    /* check wifi state machine */
    wifi_machine_state = get_wifi_machine_state();
    if(wifi_machine_state == CONNECTING_STATE
        || wifi_machine_state == DISCONNECTING_STATE){
        ret = -1;
        event_code = WIFIMG_DEV_BUSING_EVENT;
        goto end;
    }

    if(wifi_machine_state != L2CONNECTED_STATE
        && wifi_machine_state != CONNECTED_STATE){
        ret = -1;
        event_code = WIFIMG_OPT_NO_USE_EVENT;
        goto end;
    }

    len = NET_ID_LEN+1;
    ret = wpa_conf_get_netid_connected(netid, &len);
    if(ret <= 0){
        printf("This no connected AP!\n");
        ret = -1;
        event_code = WIFIMG_OPT_NO_USE_EVENT;
        goto end;
    }

    /* set disconnecting */
    set_wifi_machine_state(DISCONNECTING_STATE);

    /* set disconnect event label */
    disconnect_ap_event_label = event_label;

    /* disconnected */
	  sprintf(cmd, "%s", "DISCONNECT");
	  ret = wifi_command(cmd, reply, sizeof(reply));
    if(ret){
        printf("do disconnect network error!\n");
        ret = -1;
        event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
        goto end;
    }

    i=0;
    do{
        usleep(200000);
        if(get_wifi_machine_state() == DISCONNECTED_STATE){
            break;
        }
        i++;
    }while(i<15);

end:
    if(ret != 0){
	  call_event_callback_function(event_code, NULL, event_label);
    }

    /* resume scan thread */
    resume_wifi_scan_thread();

    return ret;
}

static int aw_wifi_list_networks(char *reply, size_t reply_len, int event_label)
{
	//int i=0;
	int ret = -1;
	char cmd[CMD_LEN+1] = {0};
	//char reply[REPLY_BUF_SIZE] = {0};
	//char netid[NET_ID_LEN+1]={0};
	tWIFI_MACHINE_STATE wifi_machine_state;

	if(gwifi_state == WIFIMG_WIFI_DISABLED){
		return -1;
	}

	/* pause scan thread */
	   pause_wifi_scan_thread();

	/* check network exist in wpa_supplicant.conf */

    wifi_machine_state = get_wifi_machine_state();
    if(wifi_machine_state != CONNECTED_STATE && wifi_machine_state != DISCONNECTED_STATE){
        ret = -1;
        event_code = WIFIMG_DEV_BUSING_EVENT;
        goto end;
    }

	/*there is no network information in the supplicant.conf file now!*/
	if(wpa_conf_network_info_exist() == 0){
		ret = 0;
		goto end;
	}



	/* list_networks */
	//sprintf(cmd, "%s", "LIST_NETWORKS");
	strncpy(cmd, "LIST_NETWORKS", CMD_LEN);
       cmd[CMD_LEN] = '\0';
	ret = wifi_command(cmd, reply, reply_len);
	if(ret){
		printf("do list_networks error!\n");
		ret = -1;
		event_code = WIFIMG_CMD_OR_PARAMS_ERROR;
	}
	printf("do list_networks finished!\n");
end:
	if(ret != 0){
	call_event_callback_function(event_code, NULL, event_label);
	}

    /* resume scan thread */
    resume_wifi_scan_thread();

	return ret;

}

/*
*Ap with certain key_mgmt exists in the .conf file:return is 0, get the *net_id as expectation;
*else:return -1
*/
static int aw_wifi_get_netid(const char *ssid, tKEY_MGMT key_mgmt, char *net_id, int *length)
{
	int ret = -1, len = NET_ID_LEN+1;

	if(*length > (NET_ID_LEN+1))
	    len = NET_ID_LEN+1;
	else
	    len = *length;

	/* pause scan thread */
	pause_wifi_scan_thread();
	ret = wpa_conf_is_ap_exist(ssid, key_mgmt, net_id, &len);
	/* resume scan thread */
	resume_wifi_scan_thread();
	if(ret == 1 || ret == 3){
		*length = len;
		return 0;
	}else{
		return -1;
	}
}

static int aw_wifi_stop_scan()
{
    shutdown_wifi_scan_thread();
    return 0;
}

static int aw_wifi_restart_scan()
{
    restart_wifi_scan_thread();
    return 0;
}

static const aw_wifi_interface_t aw_wifi_interface = {
    aw_wifi_add_event_callback,
    aw_wifi_is_ap_connected,
    aw_wifi_scan,
    aw_wifi_get_scan_results,
    aw_wifi_connect_ap,
    aw_wifi_connect_ap_key_mgmt,
    aw_wifi_connect_ap_auto,
    aw_wifi_connect_ap_with_netid,
    aw_wifi_add_network,
    aw_wifi_disconnect_ap,
    aw_wifi_remove_network,
    aw_wifi_remove_all_networks,
    aw_wifi_list_networks,
    aw_wifi_get_netid,
    aw_wifi_stop_scan,
    aw_wifi_restart_scan
};

const aw_wifi_interface_t * aw_wifi_on(tWifi_event_callback pcb, int event_label)
{
    int i = 0, ret = -1, connected = 0, len = 64;
    char ssid[64];

    if(gwifi_state != WIFIMG_WIFI_DISABLED){
        return NULL;
    }

    ret = wifi_connect_to_supplicant();
    if(ret){
        printf("wpa_suppplicant not running!\n");
        wifi_start_supplicant(0);
        do{
		usleep(300000);
		ret = wifi_connect_to_supplicant();
		if(!ret){
		    printf("Connected to wpa_supplicant!\n");
		    break;
		}
        printf("Connected to wpa_supplicant fail! continue\n");
		i++;
        }while(ret && i<10);
        if(ret < 0){
            if(pcb != NULL){
                pcb(WIFIMG_WIFI_ON_FAILED, NULL, event_label);
            }
            return NULL;
        }
    }
    if(pcb != NULL){
        pcb(WIFIMG_WIFI_ON_SUCCESS, NULL, event_label);
    }

    gwifi_state = WIFIMG_WIFI_ENABLE;

    aw_wifi_add_event_callback(pcb);

    wifi_event_loop(NULL);

    /* check has network info in wpa_supplicant.conf */
    if(wpa_conf_network_info_exist() == 1){
        set_wifi_machine_state(CONNECTING_STATE);
	  /* wpa_supplicant already run by other process and connected an ap */
        connected = wpa_conf_is_ap_connected(ssid, &len);
        if(connected >= 4){
            set_wifi_machine_state(CONNECTED_STATE);
	    set_cur_wifi_event(AP_CONNECTED);
	    call_event_callback_function(WIFIMG_NETWORK_CONNECTED, NULL, event_label);
            ret = 0;
        }else{
            connecting_ap_event_label = event_label;
            start_wifi_on_check_connect_timeout();
        }
    }else{
        set_wifi_machine_state(DISCONNECTED_STATE);
        event_code = WIFIMG_NO_NETWORK_CONNECTING;
        ret = -1;
    }

    start_wifi_scan_thread(NULL);

    if(ret != 0){
        call_event_callback_function(event_code, NULL, event_label);
    }

    return &aw_wifi_interface;
}

int aw_wifi_off(const aw_wifi_interface_t *p_wifi_interface)
{
    const aw_wifi_interface_t *p_aw_wifi_intf = &aw_wifi_interface;

    if(p_aw_wifi_intf != p_wifi_interface){
	call_event_callback_function(WIFIMG_WIFI_OFF_FAILED, NULL, 0);
        return -1;
    }

    if(gwifi_state == WIFIMG_WIFI_DISABLED){
        return 0;
    }

    stop_wifi_scan_thread();
    wifi_close_supplicant_connection();
    wifi_stop_supplicant(0);

    while(gwifi_state != WIFIMG_WIFI_DISABLED)
        usleep(200*1000);
    reset_wifi_event_callback();
    gwifi_state = WIFIMG_WIFI_DISABLED;
    return 0;
}

int aw_wifi_get_wifi_state()
{
    int ret = -1, len = 0;
    char ssid[64] = {0};
    tWIFI_MACHINE_STATE machine_state;
    int tmp_state;

    /* wpa_supplicant already running not by self process */
    if(gwifi_state == WIFIMG_WIFI_DISABLED){
	  /* check wifi already on by self process or other process */
        ret = wifi_connect_to_supplicant();
        if(ret){
            printf("WiFi not on\n");
            return WIFIMG_WIFI_DISABLED;
        }

	  /* sync wifi state by wpa_supplicant */
	len = sizeof(ssid);
	ret = wpa_conf_is_ap_connected(ssid, &len);
        if(ret >= 4){
            tmp_state = WIFIMG_WIFI_CONNECTED;
        }else{
            tmp_state = WIFIMG_WIFI_DISCONNECTED;
        }

        /*close connect */
        wifi_close_supplicant_connection();
        return tmp_state;
    }

    machine_state = get_wifi_machine_state();
    if(machine_state == DISCONNECTED_STATE){
        gwifi_state = WIFIMG_WIFI_DISCONNECTED;
    }else if(machine_state == CONNECTING_STATE
        || machine_state == DISCONNECTING_STATE
        || machine_state == L2CONNECTED_STATE){
        gwifi_state = WIFIMG_WIFI_BUSING;
    }else if(machine_state == CONNECTED_STATE){
        gwifi_state = WIFIMG_WIFI_CONNECTED;
    }

    return gwifi_state;
}
