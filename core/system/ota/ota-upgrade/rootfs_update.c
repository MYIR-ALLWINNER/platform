#include "upgrade.h"
#include "misc.h"

static int local_dir = 0;
static char media[64];
static char path[256];
static int get_update_flags(void)
{
    int fd, ret;
	update_cfg_t *cfg;
	char config[512];

	cfg = (update_cfg_t *)config;
	fd = ret = -1;
    memset(config, 0, sizeof(config));
	if((fd = open(MMCBLK0, O_RDWR)) > 0){
		lseek(fd, CONF_SECTOR * 512, SEEK_SET);
		ret = read(fd, (void *)cfg, sizeof(config));
		close(fd);
		if(ret > 0){
			printf("UPDATE_FLAGS:0x%x media_name:%s\n", cfg->flags, cfg->media_name);
			if(cfg->flags || cfg->flags == UPDATE_FLAGS || cfg->media_name[0]){
				if(cfg->local_dir == 1)
					local_dir = 1;
				else
					local_dir = 0;
				strncpy(media, cfg->media_name, sizeof(media));
				return 1;
			}
		}else{
		    LOGE("read update configure failed\n");
			return -1;
		}
	}else{
	    LOGE("open %s block devices failed \n", MMCBLK0);
		return -1;
	}

	return 0;
}

static int clean_update_flags(void)
{
    int fd, ret;
	char config[512];
	
	fd = ret = -1;
    unlock_boot_part();
    fd = open(MMCBLK0, O_RDWR);
	if(fd < 0){
		LOGE("%s open %s failed \n", __func__, MMCBLK0);
		ret = -1;
		goto err_ret;
	}
	memset(config, 0, sizeof(config));
	lseek(fd, CONF_SECTOR * 512, SEEK_SET);
	ret = write(fd, config, sizeof(config));
	if(ret < 0){
		LOGE("%s clean flags failed \n", __func__);
		ret = -1;
		goto err_write;
	}
	fsync(fd);
err_write:
    close(fd);
err_ret:
	lock_boot_part();
	return ret;
}

static void mnt_dev(void)
{
    char cmd[128];

    system("mkdir -p /tmp");
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "mount -t vfat /dev/%s /tmp", media);
	system(cmd);
}

static inline void clean_files(void)
{
	system("cd /tmp && rm -rf /tmp/.*.fex");
}
int main(int argc, char *argv[])
{
    char *dev_str;
    if(get_update_flags() <= 0){
		LOGE("No need upgrade firmware\n");
		return 0;
    }
	get_env();
	if(local_dir == 0 && wait_dev_ready(media) < 0){
		LOGE("open device %s failed \n");
		return 0;
	}
	mnt_dev();
    dev_str = getenv("rootfs");
	if(dev_str){
		update_part(dev_str, "/tmp/.rootfs.fex", 0);
		clean_update_flags();
		clean_files();
		LOGE("OTA successfull, system will rebooting \n");
		sleep(5);
		reboot(RB_AUTOBOOT);
	}
	return 0;
}

