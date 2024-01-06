#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <wpa_ctrl.h>
#include <wifi.h>
#include <unistd.h>

#define IFACE_VALUE_MAX 32

static struct wpa_ctrl *ctrl_conn;
static struct wpa_ctrl *monitor_conn;

/* socket pair used to exit from a blocking read */
static int exit_sockets[2];

//static const char IFACE_DIR[]           = "/data/misc/wifi/sockets";
static const char IFACE_DIR[]           = "/etc/wifi/sockets";
static char primary_iface[IFACE_VALUE_MAX];
static const char SUPP_CONFIG_TEMPLATE[]= "/etc/wifi/wpa_supplicant_src.conf";
static const char SUPP_CONFIG_FILE[]    = "/etc/wifi/wpa_supplicant.conf";
static const char CONTROL_IFACE_PATH[]  = "/etc/wifi/sockets";

static const char SUPP_ENTROPY_FILE[]   = WIFI_ENTROPY_FILE;
static unsigned char dummy_key[21] = { 0x02, 0x11, 0xbe, 0x33, 0x43, 0x35,
                                       0x68, 0x47, 0x84, 0x99, 0xa9, 0x2b,
                                       0x1c, 0xd3, 0xee, 0xff, 0xf1, 0xe2,
                                       0xf3, 0xf4, 0xf5 };

static const char IFNAME[]              = "IFNAME=";
#define IFNAMELEN			(sizeof(IFNAME) - 1)
static const char WPA_EVENT_IGNORE[]    = "CTRL-EVENT-IGNORE ";

static int insmod(const char *filename, const char *args)
{
#if 0

    void *module;
    unsigned int size;
    int ret;

    module = load_file(filename, &size);
    if (!module)
        return -1;

    ret = init_module(module, size, args);

    free(module);

#else

    int ret = 0;
    char cmd[256] = {0};
    sprintf(cmd,"insmod '%s' '%s'", filename, args);
    system(cmd);

#endif
    return ret;
}

static int rmmod(const char *modname)
{
#if 0

    int ret = -1;
    int maxtry = 10;

    while (maxtry-- > 0) {
        ret = delete_module(modname, O_NONBLOCK | O_EXCL);
        if (ret < 0 && errno == EAGAIN)
            usleep(500000);
        else
            break;
    }

    if (ret != 0)
        printf("Unable to unload driver module \"%s\": %s\n",
             modname, strerror(errno));

#else

    int ret = 0;
    char cmd[256] = {0};
    sprintf(cmd,"rmmod '%s'", modname);
    system(cmd);

#endif
    return ret;
}

#define TIME_COUNT 20 // 200ms*20 = 4 seconds for completion
int wifi_load_driver(const char *path, const char *args)
{
	int  count = 0;
	int  i=0;
	//int name_len = 0;
	int  ret        = 0;
	char name[256] = {0}, tmp_buf[512] = {0};
    char * p_s = NULL, * p_e = NULL;
    char *p_strstr_wlan  = NULL;
    FILE *fp        = NULL;

    if (!path) {
        printf("driver path is NULL!\n");
        return -1;
    }

    p_s = strrchr(path, '/');
    p_s++;

    p_e = strrchr(path, '.');
    p_e--;

    i = 0;
    while(p_s <= p_e){
        name[i] = *p_s;
        i++;
        p_s++;
    }
    name[i] = '\0';
    printf("driver name %s\n", name);

    if (insmod(path, args) < 0) {
        printf("insmod %s %s firmware failed!\n", path, args);
        rmmod(name);//it may be load driver already,try remove it.
        return -1;
    }

    do{
        fp=fopen("/proc/net/wireless", "r");
        if (!fp) {
            printf("failed to fopen file: /proc/net/wireless\n");
            rmmod(name); //try remove it.
            return -1;
        }
        ret = fread(tmp_buf, sizeof(tmp_buf), 1, fp);
        if (ret==0){
            printf("faied to read proc/net/wireless\n");
        }
        fclose(fp);

        printf("loading wifi driver...\n");
        p_strstr_wlan = strstr(tmp_buf, "wlan0");
        if (p_strstr_wlan != NULL) {
            break;
        }
        usleep(200000);// 200ms

    } while (count++ <= TIME_COUNT);

    if(count > TIME_COUNT) {
        printf("timeout, register netdevice wlan0 failed.\n");
        rmmod(name);
        return -1;
    }
    return 0;
}

int wifi_unload_driver(const char *name)
{
    if (rmmod(name) == 0){
        usleep(2000000);
	return 0;
    }else
	  return -1;
}

int ensure_entropy_file_exists()
{
    int ret;
    int destfd;

    ret = access(SUPP_ENTROPY_FILE, R_OK|W_OK);
    if ((ret == 0) || (errno == EACCES)) {
        if ((ret != 0) &&
            (chmod(SUPP_ENTROPY_FILE, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) != 0)) {
            printf("Cannot set RW to \"%s\": %s\n", SUPP_ENTROPY_FILE, strerror(errno));
            return -1;
        }
        return 0;
    }
    destfd = TEMP_FAILURE_RETRY(open(SUPP_ENTROPY_FILE, O_CREAT|O_RDWR, 0660));
    if (destfd < 0) {
        printf("Cannot create \"%s\": %s\n", SUPP_ENTROPY_FILE, strerror(errno));
        return -1;
    }

    if (TEMP_FAILURE_RETRY(write(destfd, dummy_key, sizeof(dummy_key))) != sizeof(dummy_key)) {
        printf("Error writing \"%s\": %s\n", SUPP_ENTROPY_FILE, strerror(errno));
        close(destfd);
        return -1;
    }
    close(destfd);

    /* chmod is needed because open() didn't set permisions properly */
    if (chmod(SUPP_ENTROPY_FILE, 0660) < 0) {
        printf("Error changing permissions of %s to 0660: %s\n",
             SUPP_ENTROPY_FILE, strerror(errno));
        unlink(SUPP_ENTROPY_FILE);
        return -1;
    }

    return 0;
}

int update_ctrl_interface(const char *config_file) {

    int srcfd, destfd;
    int nread;
    char ifc[IFACE_VALUE_MAX];
    char *pbuf;
    char *sptr;
    struct stat sb;
    int ret;

    if (stat(config_file, &sb) != 0)
        return -1;

    pbuf = (char *)malloc(sb.st_size + IFACE_VALUE_MAX);
    if (!pbuf)
        return 0;
    srcfd = TEMP_FAILURE_RETRY(open(config_file, O_RDONLY));
    if (srcfd < 0) {
        printf("Cannot open \"%s\": %s\n", config_file, strerror(errno));
        free(pbuf);
        return 0;
    }
    nread = TEMP_FAILURE_RETRY(read(srcfd, pbuf, sb.st_size));
    close(srcfd);
    if (nread < 0) {
        printf("Cannot read \"%s\": %s\n", config_file, strerror(errno));
        free(pbuf);
        return 0;
    }

    strcpy(ifc, CONTROL_IFACE_PATH);

    /* Assume file is invalid to begin with */
    ret = -1;
    /*
     * if there is a "ctrl_interface=<value>" entry, re-write it ONLY if it is
     * NOT a directory.  The non-directory value option is an Android add-on
     * that allows the control interface to be exchanged through an environment
     * variable (initialized by the "init" program when it starts a service
     * with a "socket" option).
     *
     * The <value> is deemed to be a directory if the "DIR=" form is used or
     * the value begins with "/".
     */
    if ((sptr = strstr(pbuf, "ctrl_interface="))) {
        ret = 0;
        if ((!strstr(pbuf, "ctrl_interface=DIR=")) &&
                (!strstr(pbuf, "ctrl_interface=/"))) {
            char *iptr = sptr + strlen("ctrl_interface=");
            int ilen = 0;
            int mlen = strlen(ifc);
            //int nwrite;
            if (strncmp(ifc, iptr, mlen) != 0) {
                printf("ctrl_interface != %s\n", ifc);
                while (((ilen + (iptr - pbuf)) < nread) && (iptr[ilen] != '\n'))
                    ilen++;
                mlen = ((ilen >= mlen) ? ilen : mlen) + 1;
                memmove(iptr + mlen, iptr + ilen + 1, nread - (iptr + ilen + 1 - pbuf));
                memset(iptr, '\n', mlen);
                memcpy(iptr, ifc, strlen(ifc));
                destfd = TEMP_FAILURE_RETRY(open(config_file, O_RDWR, 0660));
                if (destfd < 0) {
                    printf("Cannot update \"%s\": %s\n", config_file, strerror(errno));
                    free(pbuf);
                    return -1;
                }
                TEMP_FAILURE_RETRY(write(destfd, pbuf, nread + mlen - ilen -1));
                close(destfd);
            }
        }
    }
    free(pbuf);
    return ret;
}

int ensure_config_file_exists(const char *config_file)
{
    char buf[2048];
    int srcfd, destfd;
    //struct stat sb;
    int nread;
    int ret;

    ret = access(config_file, R_OK|W_OK);
    if ((ret == 0) || (errno == EACCES)) {
        if ((ret != 0) &&
            (chmod(config_file, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) != 0)) {
            printf("Cannot set RW to \"%s\": %s\n", config_file, strerror(errno));
            return -1;
        }
        /* return if we were able to update control interface properly */
        if (update_ctrl_interface(config_file) >=0) {
            return 0;
        } else {
            /* This handles the scenario where the file had bad data
             * for some reason. We continue and recreate the file.
             */
        }
    } else if (errno != ENOENT) {
        printf("Cannot access \"%s\": %s\n", config_file, strerror(errno));
        return -1;
    }

    srcfd = TEMP_FAILURE_RETRY(open(SUPP_CONFIG_TEMPLATE, O_RDONLY));
    if (srcfd < 0) {
        printf("Cannot open \"%s\": %s\n", SUPP_CONFIG_TEMPLATE, strerror(errno));
        return -1;
    }

    destfd = TEMP_FAILURE_RETRY(open(config_file, O_CREAT|O_RDWR, 0660));
    if (destfd < 0) {
        close(srcfd);
        printf("Cannot create \"%s\": %s\n", config_file, strerror(errno));
        return -1;
    }

    while ((nread = TEMP_FAILURE_RETRY(read(srcfd, buf, sizeof(buf)))) != 0) {
        if (nread < 0) {
            printf("Error reading \"%s\": %s\n", SUPP_CONFIG_TEMPLATE, strerror(errno));
            close(srcfd);
            close(destfd);
            unlink(config_file);
            return -1;
        }
        TEMP_FAILURE_RETRY(write(destfd, buf, nread));
    }

    close(destfd);
    close(srcfd);

    /* chmod is needed because open() didn't set permisions properly */
    if (chmod(config_file, 0660) < 0) {
        printf("Error changing permissions of %s to 0660: %s\n",
             config_file, strerror(errno));
        unlink(config_file);
        return -1;
    }

    return update_ctrl_interface(config_file);
}

int wifi_start_supplicant(int p2p_supported)
{
    //char cmd[512] = {0};

    /* Before starting the daemon, make sure its config file exists */
    if (ensure_config_file_exists(SUPP_CONFIG_FILE) < 0) {
        printf("Wi-Fi will not be enabled\n");
        return -1;
    }

    if (ensure_entropy_file_exists() < 0) {
        printf("Wi-Fi entropy file was not created\n");
    }

    /* Clear out any stale socket files that might be left over. */
    //wpa_ctrl_cleanup();

    /* Reset sockets used for exiting from hung state */
    exit_sockets[0] = exit_sockets[1] = -1;

    /* start wpa_supplicant */
//  strncpy(cmd, "/etc/wifi/wifi start", 511);
//  cmd[511] = '\0';
//  system(cmd);
    system("killall wpa_supplicant");
    sleep(1);
    system("ifconfig wlan0 up");
    system("/etc/wifi/wifi_start.sh");
    printf("wifi_start_supplicant success\n");
    return 0;
}

int wifi_stop_supplicant(int p2p_supported)
{
//	  system("/etc/wifi/wifi stop");
      system("killall wpa_supplicant");
      system("ifconfig wlan0 down");  
	  return 0;
}

#define SUPPLICANT_TIMEOUT      3000000  // microseconds
#define SUPPLICANT_TIMEOUT_STEP  100000  // microseconds
int wifi_connect_on_socket_path(const char *path)
{
    int  supplicant_timeout = SUPPLICANT_TIMEOUT;

    ctrl_conn = wpa_ctrl_open(path);
    while (ctrl_conn == NULL && supplicant_timeout > 0){
        usleep(SUPPLICANT_TIMEOUT_STEP);
        supplicant_timeout -= SUPPLICANT_TIMEOUT_STEP;
        ctrl_conn = wpa_ctrl_open(path);
    }
    if (ctrl_conn == NULL) {
        printf("Unable to open connection to supplicant on \"%s\": %s\n",
             path, strerror(errno));
        return -1;
    }
    monitor_conn = wpa_ctrl_open(path);
    if (monitor_conn == NULL) {
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = NULL;
        return -1;
    }
    if (wpa_ctrl_attach(monitor_conn) != 0) {
        wpa_ctrl_close(monitor_conn);
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = monitor_conn = NULL;
        return -1;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, exit_sockets) == -1) {
        wpa_ctrl_close(monitor_conn);
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = monitor_conn = NULL;
        return -1;
    }

    return 0;
}

/* Establishes the control and monitor socket connections on the interface */
int wifi_connect_to_supplicant()
{
    static char path[PATH_MAX];

    strncpy(primary_iface, "wlan0", IFACE_VALUE_MAX);
    if (access(IFACE_DIR, F_OK) == 0) {
        snprintf(path, sizeof(path), "%s/%s", IFACE_DIR, primary_iface);
    } else {
        return -1;
    }
    return wifi_connect_on_socket_path(path);
}

int wifi_send_command(const char *cmd, char *reply, size_t *reply_len)
{
    int ret;
    if (ctrl_conn == NULL) {
        printf("Not connected to wpa_supplicant - \"%s\" command dropped.\n", cmd);
        return -1;
    }

    ret = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd), reply, reply_len, NULL);
    if (ret == -2) {
        printf("'%s' command timed out.\n", cmd);
        /* unblocks the monitor receive socket for termination */
        TEMP_FAILURE_RETRY(write(exit_sockets[0], "T", 1));
        return -2;
    } else if (ret < 0 || strncmp(reply, "FAIL", 4) == 0) {
        return -1;
    }
    if (strncmp(cmd, "PING", 4) == 0) {
        reply[*reply_len] = '\0';
    }
    return 0;
}

int wifi_ctrl_recv(char *reply, size_t *reply_len)
{
    int res;
    int ctrlfd = wpa_ctrl_get_fd(monitor_conn);
    struct pollfd rfds[2];

    memset(rfds, 0, 2 * sizeof(struct pollfd));
    rfds[0].fd = ctrlfd;
    rfds[0].events |= POLLIN;
    rfds[1].fd = exit_sockets[1];
    rfds[1].events |= POLLIN;
    res = TEMP_FAILURE_RETRY(poll(rfds, 2, -1));
    if (res < 0) {
        printf("Error poll = %d\n", res);
        return res;
    }
    if (rfds[0].revents & POLLIN) {
        return wpa_ctrl_recv(monitor_conn, reply, reply_len);
    }

    /* it is not rfds[0], then it must be rfts[1] (i.e. the exit socket)
     * or we timed out. In either case, this call has failed ..
     */
    return -2;
}

int wifi_wait_on_socket(char *buf, size_t buflen)
{
    size_t nread = buflen - 1;
    int result;
    char *match, *match2;

    if (monitor_conn == NULL) {
        return snprintf(buf, buflen, WPA_EVENT_TERMINATING " - connection closed");
    }

    result = wifi_ctrl_recv(buf, &nread);

    /* Terminate reception on exit socket */
    if (result == -2) {
        return snprintf(buf, buflen, WPA_EVENT_TERMINATING " - connection closed");
    }

    if (result < 0) {
        printf("wifi_ctrl_recv failed: %s\n", strerror(errno));
        return snprintf(buf, buflen, WPA_EVENT_TERMINATING " - recv error");
    }
    buf[nread] = '\0';
    /* Check for EOF on the socket */
    if (result == 0 && nread == 0) {
        /* Fabricate an event to pass up */
        printf("Received EOF on supplicant socket\n");
        return snprintf(buf, buflen, WPA_EVENT_TERMINATING " - signal 0 received");
    }
    /*
     * Events strings are in the format
     *
     *     IFNAME=iface <N>CTRL-EVENT-XXX
     *        or
     *     <N>CTRL-EVENT-XXX
     *
     * where N is the message level in numerical form (0=VERBOSE, 1=DEBUG,
     * etc.) and XXX is the event name. The level information is not useful
     * to us, so strip it off.
     */

    if (strncmp(buf, IFNAME, IFNAMELEN) == 0) {
        match = strchr(buf, ' ');
        if (match != NULL) {
            if (match[1] == '<') {
                match2 = strchr(match + 2, '>');
                if (match2 != NULL) {
                    nread -= (match2 - match);
                    memmove(match + 1, match2 + 1, nread - (match - buf) + 1);
                }
            }
        } else {
            return snprintf(buf, buflen, "%s", WPA_EVENT_IGNORE);
        }
    } else if (buf[0] == '<') {
        match = strchr(buf, '>');
        if (match != NULL) {
            nread -= (match + 1 - buf);
            memmove(buf, match + 1, nread + 1);
            //printf("supplicant generated event without interface - %s\n", buf);
        }
    } else {
        /* let the event go as is! */
        //printf("supplicant generated event without interface and without message level - %s\n", buf);
    }

    return nread;
}

int wifi_wait_for_event(char *buf, size_t buflen)
{
    return wifi_wait_on_socket(buf, buflen);
}

void wifi_close_sockets()
{
    if (ctrl_conn != NULL) {
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = NULL;
    }

    if (monitor_conn != NULL) {
        wpa_ctrl_detach(monitor_conn);
        wpa_ctrl_close(monitor_conn);
        monitor_conn = NULL;
    }

    if (exit_sockets[0] >= 0) {
        close(exit_sockets[0]);
        exit_sockets[0] = -1;
    }

    if (exit_sockets[1] >= 0) {
        close(exit_sockets[1]);
        exit_sockets[1] = -1;
    }
}

void wifi_close_supplicant_connection()
{
    int count = 20; /* wait 2 seconds to ensure init has stopped stupplicant */

    wifi_close_sockets();

    while (count-- > 0) {
        usleep(100000);
    }
}

int wifi_command(char const *cmd, char *reply, size_t reply_len)
{
    if(!cmd || !cmd[0]){
        return -1;
    }

    printf("do cmd %s\n", cmd);

    --reply_len; // Ensure we have room to add NUL termination.
    if (wifi_send_command(cmd, reply, &reply_len) != 0) {
        return -1;
    }

    // Strip off trailing newline.
    if (reply_len > 0 && reply[reply_len-1] == '\n') {
        reply[reply_len-1] = '\0';
    } else {
        reply[reply_len] = '\0';
    }
    return 0;
}
