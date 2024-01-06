#define TAG "wifi"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <wifi_intf.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>

#define TEST_TIMES 1001

static pthread_t  app_scan_tid;
static int event = WIFIMG_NETWORK_DISCONNECTED;
static int terminate_flag = 0;

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
	printf("NAME:\n\twifi_longtime_test\n");
	printf("DESCRIPTION:\n\tTest the stability of wifi module(see the test result in file:/etc/test_result).\n");
	printf("USAGE:\n\twifi_add_network_test <ssid> <passwd> [test_times]\n");
	printf("PARAMS:\n\tssid       : ssid of the AP\n");
	printf("\tpasswd     : Password of the AP\n");
	printf("\ttest_times : test times of the action(connect AP and disconnect AP)\n");
	printf("\t             default test times: %d\n",TEST_TIMES);
	printf("--------------------------------------MORE---------------------------------------\n");
	printf("The way to get help information:\n");
	printf("\twifi_longtime_test --help\n");
	printf("\twifi_longtime_test -h\n");
	printf("\twifi_longtime_test -H\n");
	printf("---------------------------------------------------------------------------------\n");
}

static void signal_handler(int signo)
{
    printf("Recieve signal: SIGINT\n");
    terminate_flag = 1;
}

/*
 *argc[1]   ap ssid
 *argc[2]   ap passwd
 *agrc[3]   [test times]
 *see the test result in file:/etc/test_result
*/
int main(int argv, char *argc[]){
    int i = 0, ret = 0, len = 0;
    int times = 0, event_label = 0;;
    char ssid[256] = {0}, scan_results[4096] = {0};
    int  wifi_state = WIFIMG_WIFI_DISABLED;
    const aw_wifi_interface_t *p_wifi_interface = NULL;
    unsigned long long int total_con_sec = 0, total_con_microsec = 0;
    unsigned long long int con_sec = 0, con_microsec = 0;
    unsigned long long int total_disc_sec = 0, total_disc_microsec = 0;
    unsigned long long int discon_sec = 0, discon_microsec = 0;
    struct timeval tv1;
    struct timeval tv2;
    int success_times = 0, fail_times = 0, timeout_times = 0;
    char prt_buf[256] = {0};
    int ttest_times = 0;

    if(argv == 2 && (!strcmp(argc[1],"--help") || !strcmp(argc[1], "-h") || !strcmp(argc[1], "-H"))){
	print_help();
	return -1;
    }

    if(NULL == argc[1]){
	//printf("Usage:wifi_long_test <ssid> <psk> [test_times]!\n");
	//printf("default test times: %d\n",TEST_TIMES);
	print_help();
	return -1;
    }
    if(NULL == argc[3])
	ttest_times = TEST_TIMES;
    else
	ttest_times = atoi(argc[3]);

    printf("\n*********************************\n");
    printf("***Start wifi long test!***\n");
    printf("*********************************\n");

	printf("Test times: %d\n",ttest_times);

	sleep(2);

    if(signal(SIGINT, signal_handler) == SIG_ERR)
        printf("signal(SIGINT) error\n");

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
    for(i=0;i<ttest_times;i++){
	event_label++;
        gettimeofday(&tv1, NULL);
	p_wifi_interface->connect_ap(argc[1], argc[2], event_label);

	while(aw_wifi_get_wifi_state() == WIFIMG_WIFI_BUSING){
		printf("wifi state busing,waiting\n");
		usleep(2000000);
	}

        if(event == WIFIMG_NETWORK_CONNECTED || event == WIFIMG_CONNECT_TIMEOUT)
	{
	    success_times++;
	    if(event == WIFIMG_CONNECT_TIMEOUT)
	        timeout_times++;
	}
	else
	{
	    fail_times++;
	}
        gettimeofday(&tv2, NULL);
        con_microsec = (tv2.tv_sec*1000000 + tv2.tv_usec)-(tv1.tv_sec*1000000 + tv1.tv_usec);
        con_sec = con_microsec/1000000;
        con_microsec = con_microsec%1000000;
        total_con_sec += con_sec;
        total_con_microsec += con_microsec;
	printf("==========================================\n");
	printf("Test Times: %d\nSuccess Times: %d\nFailed Times: %d\n",i+1,success_times,fail_times);
	printf("Connecting time: %llu.%-llu seconds\n", con_sec, con_microsec);
	printf("==========================================\n");
        sleep(1);

	event_label++;

        gettimeofday(&tv1, NULL);
	p_wifi_interface->disconnect_ap(event_label);

	while(aw_wifi_get_wifi_state() == WIFIMG_WIFI_BUSING){
		printf("wifi state busing,waiting\n");
		usleep(2000000);
	}
        gettimeofday(&tv2, NULL);
        discon_microsec = (tv2.tv_sec*1000000 + tv2.tv_usec)-(tv1.tv_sec*1000000 + tv1.tv_usec);
        discon_sec = discon_microsec/1000000;
        discon_microsec = discon_microsec%1000000;
        total_disc_sec += discon_sec;
        total_disc_microsec += discon_microsec;
	printf("==========================================\n");
	printf("Disconnecting time: %llu.%-llu seconds\n", discon_sec, discon_microsec);
	printf("==========================================\n");
        sleep(1);

        if(1 == terminate_flag)
            exit(-1);
    }

    total_con_microsec = total_con_sec*1000000+total_con_microsec;
    total_disc_microsec = total_disc_sec*1000000+total_disc_microsec;
    printf("Test Times:%d, Success Times:%d(including get IP timeout times:%d), Failed Times:%d\n",i,success_times,timeout_times,fail_times);
    sprintf(prt_buf,"echo \"Test Times:%d, Success Times:%d(including get IP timeout times:%d), Failed Times:%d\" > /etc/test_results",i,success_times,timeout_times,fail_times);
    system(prt_buf);
    printf("Connecting mean time: %llu.%-llu seconds\n",(total_con_microsec/1000000)/i,(total_con_microsec/i)%1000000);
    sprintf(prt_buf,"echo \"Connecting mean time: %llu.%-llu seconds\" >> /etc/test_results",(total_con_microsec/1000000)/i,(total_con_microsec/i)%1000000);
    system(prt_buf);
    printf("Disconnecting mean time: %llu.%-llu seconds\n",(total_disc_microsec/1000000)/i,(total_disc_microsec/i)%1000000);
    sprintf(prt_buf,"echo \"Disconnecting mean time: %llu.%-llu seconds\" >> /etc/test_results",(total_disc_microsec/1000000)/i,(total_disc_microsec/i)%1000000);
    system(prt_buf);
    if(success_times == ttest_times)
    {
        sprintf(prt_buf,"echo Congratulations! >> /etc/test_results");
	system(prt_buf);
    }

    printf("******************************\n");
    printf("Wifi connect ap test: Success!\n");
    printf("******************************\n");

    return 0;
}
