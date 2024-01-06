
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
	printf("NAME:\n\twifi_scan_results_test\n");
	printf("DESCRIPTION:\n\tget a list of scanned APs.\n");
	printf("USAGE:\n\twifi_scan_results_test\n");
	printf("--------------------------------------MORE---------------------------------------\n");
	printf("The way to get help information:\n");
	printf("\twifi_scan_results_test --help\n");
	printf("\twifi_scan_results_test -h\n");
	printf("\twifi_scan_results_test -H\n");
	printf("---------------------------------------------------------------------------------\n");
}
/*
 *
 *
*/
int main(int argv, char *argc[]){
    int ret = 0, len = 0, i = 0;
    int times = 0, event_label = 0;
    char ssid[256] = {0}, scan_results[4096] = {0}, reply[4069]= {0};
    int  wifi_state = WIFIMG_WIFI_DISABLED;
    const aw_wifi_interface_t *p_wifi_interface = NULL;

	if(argv == 2 && (!strcmp(argc[1],"--help") || !strcmp(argc[1], "-h") || !strcmp(argc[1], "-H"))){
		print_help();
		return -1;
	}

	if(argv != 1){
		printf("ERROR: No need other params!\n");
		print_help();
		return -1;
	}

    event_label = rand();
    p_wifi_interface = aw_wifi_on(wifi_event_handle, event_label);
    if(p_wifi_interface == NULL){
        printf("wifi on failed event 0x%x\n", event);
        return -1;
    }

    printf("aw_wifi_on success\n");
    while(aw_wifi_get_wifi_state() == WIFIMG_WIFI_BUSING){
        printf("wifi state busing,waiting\n");
        usleep(2000000);
    }

    //pthread_create(&app_scan_tid, NULL, &app_scan_task,(void *)p_wifi_interface);

    event_label++;
/*    p_wifi_interface->connect_ap(argc[1], argc[2], event_label);

    while(aw_wifi_get_wifi_state() == WIFIMG_WIFI_BUSING){
        printf("wifi state busing,waiting\n");
        usleep(2000000);
    }

    printf("******************************\n");
    printf("Wifi connect ap test: Success!\n");
    printf("******************************\n");




    for(i=0;i<10;i++)
   {
	usleep(2000000);
   }


   */
    printf("\n*********************************\n");
    printf("***Start scan!***\n");
    printf("*********************************\n");


    ret=( p_wifi_interface->start_scan(event_label));
     printf("ret of scan is %d\n", ret);
    if(ret==0)
    {
	printf("******************************\n");
       printf("Wifi scan: Success!\n");
       printf("******************************\n");
    }
    else
    {
	printf("start scan failed!\n");

    }



	len = 4096;
	 ret=( p_wifi_interface->get_scan_results(scan_results, &len));

	   printf("ret of get_scan_results is %d\n", ret);
    if(ret==0)
    {
	printf("%s\n",scan_results);
	printf("******************************\n");
       printf("Wifi get_scan_results: Success!\n");
       printf("******************************\n");
    }
    else
    {
	printf("Get_scan_results failed!\n");

    }


    return 0;
}
