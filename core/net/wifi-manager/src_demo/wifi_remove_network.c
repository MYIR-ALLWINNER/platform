
#define TAG "wifi"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <wifi_intf.h>
#include <pthread.h>

static pthread_t  app_scan_tid;
static int event = WIFIMG_NETWORK_DISCONNECTED;

static void wifi_event_handle(tWIFI_EVENT wifi_event, void *buf, int event_label)
{
    printf("event_label 0x%x\n", event_label);

    switch(wifi_event)
    {
        case WIFIMG_WIFI_ON_SUCCESS:
        {
            printf("WiFi on success!\n");
            event = WIFIMG_WIFI_ON_SUCCESS;
            break;
        }

        case WIFIMG_WIFI_ON_FAILED:
        {
            printf("WiFi on failed!\n");
            event = WIFIMG_WIFI_ON_FAILED;
            break;
        }

        case WIFIMG_WIFI_OFF_FAILED:
        {
            printf("wifi off failed!\n");
            event = WIFIMG_WIFI_OFF_FAILED;
            break;
        }

        case WIFIMG_WIFI_OFF_SUCCESS:
        {
            printf("wifi off success!\n");
            event = WIFIMG_WIFI_OFF_SUCCESS;
            break;
        }

        case WIFIMG_NETWORK_CONNECTED:
        {
            printf("WiFi connected ap!\n");
            event = WIFIMG_NETWORK_CONNECTED;
            break;
        }

        case WIFIMG_NETWORK_DISCONNECTED:
        {
            printf("WiFi disconnected!\n");
            event = WIFIMG_NETWORK_DISCONNECTED;
            break;
        }

        case WIFIMG_PASSWORD_FAILED:
        {
            printf("Password authentication failed!\n");
            event = WIFIMG_PASSWORD_FAILED;
            break;
        }

        case WIFIMG_CONNECT_TIMEOUT:
        {
            printf("Connected timeout!\n");
            event = WIFIMG_CONNECT_TIMEOUT;
            break;
        }

        case WIFIMG_NO_NETWORK_CONNECTING:
        {
            printf("It has no wifi auto connect when wifi on!\n");
            event = WIFIMG_NO_NETWORK_CONNECTING;
            break;
        }

        case WIFIMG_CMD_OR_PARAMS_ERROR:
        {
            printf("cmd or params error!\n");
            event = WIFIMG_CMD_OR_PARAMS_ERROR;
            break;
        }

        case WIFIMG_KEY_MGMT_NOT_SUPPORT:
        {
            printf("key mgmt is not supported!\n");
            event = WIFIMG_KEY_MGMT_NOT_SUPPORT;
            break;
        }

        case WIFIMG_OPT_NO_USE_EVENT:
        {
            printf("operation no use!\n");
            event = WIFIMG_OPT_NO_USE_EVENT;
            break;
        }

        case WIFIMG_NETWORK_NOT_EXIST:
        {
            printf("network not exist!\n");
            event = WIFIMG_NETWORK_NOT_EXIST;
            break;
        }

        case WIFIMG_DEV_BUSING_EVENT:
        {
            printf("wifi device busing!\n");
            event = WIFIMG_DEV_BUSING_EVENT;
            break;
        }

        default:
        {
            printf("Other event, no care!\n");
        }
    }
}

void *app_scan_task(void *args)
{
    const aw_wifi_interface_t *p_wifi = (aw_wifi_interface_t *)args;
    char scan_results[4096];
    int len = 0;
    int event_label = 0;

    while(1){
        event_label++;
        p_wifi->start_scan(event_label);
        len = 4096;
        p_wifi->get_scan_results(scan_results, &len);
    }
}


void print_help(){
	printf("---------------------------------------------------------------------------------\n");
	printf("NAME:\n\twifi_remove_network_test\n");
	printf("DESCRIPTION:\n\tremove the network in wpa_supplicant.conf.\n");
	printf("USAGE:\n\twifi_remove_network_test <ssid> <key_mgmt>\n");
	printf("PARAMS:\n\tssid     : ssid of the AP\n");
	printf("\tkey_mgmt : encryption method of the AP\n");
	printf("\t\t0 : NONE\n");
	printf("\t\t1 : key_mgmt = WPA_PSK\n");
	printf("\t\t2 : key_mgmt = WPA2_PSK\n");
	printf("\t\t3 : key_mgmt = WEP\n");
	printf("--------------------------------------MORE---------------------------------------\n");
	printf("The way to get help information:\n");
	printf("\twifi_remove_network_test --help\n");
	printf("\twifi_remove_network_test -h\n");
	printf("\twifi_remove_network_test -H\n");
	printf("---------------------------------------------------------------------------------\n");
}


int check_params(int num, char *str[]){
	if(num != 3){
		printf("ERROR: params more or less!\n");
		return -1;
	}

	int mgmt = atoi(str[2]);
	if(mgmt >= 0 && mgmt <= 3){
		return 0;
	}else{
		printf("ERROR: key_mgmt not allowed\n");
		return -1;
	}
}

/*
 *argc[1]   ap ssid
 *argc[2]   ap key_mgmt
*/
int main(int argv, char *argc[]){
    int ret = 0, switch_int = 0;
    int times = 0, event_label = 0;;
    char ssid[256] = {0}, scan_results[4096] = {0};
    int  wifi_state = WIFIMG_WIFI_DISABLED;
    const aw_wifi_interface_t *p_wifi_interface = NULL;
    tKEY_MGMT key_mgmt = WIFIMG_NONE;

	if(argv == 2 && (!strcmp(argc[1],"--help") || !strcmp(argc[1], "-h") || !strcmp(argc[1], "-H"))){
		print_help();
		return -1;
	}

	if(check_params(argv, argc)){
		print_help();
		return -1;
	}

    printf("\n*********************************\n");
    printf("***Start wifi remove network test!***\n");
    printf("*********************************\n");

    event_label = rand();
    p_wifi_interface = aw_wifi_on(wifi_event_handle, event_label);
    if(p_wifi_interface == NULL){
        printf("wifi on failed event 0x%x\n", event);
        return -1;
    }

    while(aw_wifi_get_wifi_state() == WIFIMG_WIFI_BUSING){
        printf("wifi state busing,waiting\n");
        usleep(2000000);
    }

    //pthread_create(&app_scan_tid, NULL, &app_scan_task,(void *)p_wifi_interface);

    event_label++;

     printf("The input string is %s\n",argc[2]);
    switch_int= atoi(argc[2]);
    printf("The switch_int is %d\n",switch_int);

    switch(switch_int)
    {
	case 0: key_mgmt = WIFIMG_NONE; break;
	case 1: key_mgmt = WIFIMG_WPA_PSK; break;
	case 2: key_mgmt = WIFIMG_WPA2_PSK; break;
	case 3: key_mgmt = WIFIMG_WEP; break;
       default: ; break;
    }

    ret = (p_wifi_interface->remove_network(argc[1], key_mgmt));

    while(aw_wifi_get_wifi_state() == WIFIMG_WIFI_BUSING){
        printf("wifi state busing,waiting\n");
        usleep(2000000);
    }
    if(ret == 0)
    {
    printf("******************************\n");
    printf("Wifi remove network test: Success!\n");
    printf("******************************\n");
    }

    return 0;
}
