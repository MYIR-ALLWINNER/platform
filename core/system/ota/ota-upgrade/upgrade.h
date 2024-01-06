#ifndef __MISC_MESSAGE_H__
#define __MISC_MESSAGE_H__

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mtd/mtd-user.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include "boot_head.h"
#include "dos.h"

#ifndef __DEF_DEBUG__
#define __DEF_DEBUG__
#endif


#define DEATH(mess) { perror(mess); exit(errno); }

#define UBOOT_BUF_SIZE      (2 * 1024 * 1024)
#define MISC_LIMITED_SIZE   (4 * 1024)
#define MISC_DEVICE         "/dev/mmcblk1"

#ifdef __DEF_DEBUG__
#define LOGE(...) printf( __VA_ARGS__)
#else
#define LOGE(...)
#endif

#define CHECK_MISC_MESSAGE \
    ((sizeof(bootloader_message) > MISC_LIMIT_SIZE) ? -1 : 0)

#ifndef ROUND_UP	
#define ROUND_UP(size, algin) ( (size + algin - 1) & ~(algin - 1))
#endif

#if defined(__linux__)
#  define RB_HALT_SYSTEM  0xcdef0123
#  define RB_ENABLE_CAD   0x89abcdef
#  define RB_DISABLE_CAD  0
#  define RB_POWER_OFF    0x4321fedc
#  define RB_AUTOBOOT     0x01234567
# elif defined(RB_HALT)
#  define RB_HALT_SYSTEM  RB_HALT
#endif

#define DEFAULT_FW_NAME           "sun8iw17p1_linux_t7-p1_uart0.img"

#define IMAGE_MAGIC			      "IMAGEWTY"
#define	IMAGE_HEAD_VERSION	      0x00000300
#define IMAGE_HEAD_SIZE     	  1024
#define IMAGE_ITEM_TABLE_SIZE     1024


#define MMCBOOT0                  "/dev/mmcblk0boot0"
#define BOOT0_PERM                "/sys/block/mmcblk0boot0/force_ro"
#define MMCBOOT1                  "/dev/mmcblk0boot1"
#define BOOT1_PERM                "/sys/block/mmcblk0boot1/force_ro"
#define MMCBLK0                   "/dev/mmcblk0"

#define PERM_ENABLE               "0"

#define MMC_SECT_SIZE             512
#define BOOT0_START               BOOT0_SDMMC_START_ADDR
#define BOOT0_BAKUP               BOOT0_SDMMC_BACKUP_START_ADDR  
#define UBOOT_START               UBOOT_START_SECTOR_IN_SDMMC
#define UBOOT_BAKUP               UBOOT_BACKUP_START_SECTOR_IN_SDMMC

#define SECTOR_SIZE               MMC_SECT_SIZE

#define IMAGE_ALIGN_SIZE		  0x400
#define HEAD_ATTR_NO_COMPRESS 	  0x4d    /* 1001101 */


#ifdef __CONFIG_NAND__
#define NAND_BLKBURNBOOT0 		  _IO('v',127)
#define NAND_BLKBURNUBOOT 		  _IO('v',128) 

typedef struct {
    unsigned char* buffer;
    long len;
} buf_cookies_t;
#endif             

#pragma  pack (push,1)
typedef struct
{
	unsigned char magic[8];		/* IMAGE_MAGIC */
	unsigned int  version;		/* IMAGE_HEAD_VERSION */
	unsigned int  size;
	unsigned int  attr;
	unsigned int  imagever;
	unsigned int  lenLo;
	unsigned int  lenHi;
	unsigned int  align;		/* align with 1024 */
	unsigned int  pid;			/* PID  */
	unsigned int  vid;			/* VID  */
	unsigned int  hardwareid;
	unsigned int  firmwareid;
	unsigned int  itemattr;	
	unsigned int  itemsize;
	unsigned int  itemcount;
	unsigned int  itemoffset;
	unsigned int  imageattr;
	unsigned int  appendsize;
	unsigned int  appendoffsetLo;
	unsigned int  appendoffsetHi;
	unsigned char reserve[980];
}img_hdr_t;
#pragma pack(pop)


#pragma pack(push, 1)
typedef struct tagImageHeadAttr{
	unsigned int res:12;
	unsigned int len:8;
	unsigned int encode:7;		            /* HEAD_ATTR_NO_COMPRESS */
	unsigned int compress:5;
}ImageHeadAttr_t;
#pragma pack(pop)

#define	IMAGE_ITEM_VERSION	0x00000100
#define MAINTYPE_LEN		8
#define SUBTYPE_LEN			16
#define FILE_PATH			256
#define IMAGE_ITEM_RCSIZE   640


#pragma pack(push, 1)
typedef struct tag_ImageItem
{
	unsigned int  version;
	unsigned int  size;
	unsigned char mainType[MAINTYPE_LEN];
	unsigned char subType[SUBTYPE_LEN];
	unsigned int  attr;
	unsigned char name[FILE_PATH];
	unsigned int  datalenLo;
	unsigned int  datalenHi;
	unsigned int  filelenLo;
	unsigned int  filelenHi;
	unsigned int  offsetLo;
	unsigned int  offsetHi;
	unsigned char encryptID[64];
	unsigned int  checksum;
	unsigned char res[IMAGE_ITEM_RCSIZE];
}img_item_t;
#pragma pack(pop)

typedef struct
{
	img_hdr_t img_hdr;	
	img_item_t *item_tbl;
}img_hdl_t;

typedef struct{
	unsigned int index;	
	unsigned int reserved[3];
    /*long long pos;*/
}item_hdl_t;

#define     DOWNLOAD_MAP_NAME           "dlinfo.fex"
#define     SUNXI_MBR_NAME              "sunxi_mbr.fex"
/* MBR       */
#define     SUNXI_MBR_SIZE			    (16 * 1024)
#define     SUNXI_DL_SIZE				(16 * 1024)
#define   	SUNXI_MBR_MAGIC			    "softw411"
#define     SUNXI_MBR_MAX_PART_COUNT	120
#define     SUNXI_MBR_COPY_NUM          4 
#define     SUNXI_MBR_RESERVED          (SUNXI_MBR_SIZE - 32 - 4 - (SUNXI_MBR_MAX_PART_COUNT * sizeof(sunxi_partition)))
#define     SUNXI_DL_RESERVED           (SUNXI_DL_SIZE - 32 - (SUNXI_MBR_MAX_PART_COUNT * sizeof(dl_one_part_info)))

#define     SUNXI_NOLOCK                (0)
#define     SUNXI_LOCKING               (0xAA)
#define     SUNXI_RELOCKING             (0xA0)
#define     SUNXI_UNLOCK                (0xA5)

#define     EXTRACT_BUF_SIZE            (16 * 1024 * 1024)
/* partition information */
typedef struct sunxi_partition_t
{
	unsigned int addrhi;
	unsigned int addrlo;
	unsigned int lenhi;
	unsigned int lenlo;
	unsigned char classname[16];
	unsigned char name[16];
	unsigned int user_type;
	unsigned int keydata;
	unsigned int ro;
	unsigned int sig_verify;
	unsigned int sig_erase;
	unsigned int sig_value[4];
	unsigned int sig_pubkey;
	unsigned int sig_pbumode;
	unsigned char reserved2[36];
}__attribute__ ((packed))sunxi_partition;

/* mbr information */
typedef struct sunxi_mbr
{
	unsigned int crc32;
	unsigned int version;
	unsigned char magic[8];			        /* softw311" */
	unsigned int copy;
	unsigned int index;
	unsigned int PartCount;
	unsigned int stamp[1];
	sunxi_partition array[SUNXI_MBR_MAX_PART_COUNT];
	unsigned int lockflag;
	unsigned char res[SUNXI_MBR_RESERVED];
}__attribute__ ((packed)) sunxi_mbr_t;

typedef struct tag_one_part_info
{
	unsigned char name[16];
	unsigned int addrhi;
	unsigned int addrlo;
	unsigned int lenhi;
	unsigned int lenlo;
	unsigned char dl_filename[16];
	unsigned char vf_filename[16];
	unsigned int encrypt;
	unsigned int verify;
}__attribute__ ((packed)) dl_one_part_info;

typedef struct tag_download_info
{
	unsigned int crc32;
	unsigned int version;             /* 0x00000101 */
	unsigned char magic[8];			  /* "softw311" */
	unsigned int download_count; 
	unsigned int stamp[3];
	dl_one_part_info one_part_info[SUNXI_MBR_MAX_PART_COUNT];
	unsigned char res[SUNXI_DL_RESERVED];
}
__attribute__ ((packed)) sunxi_download_info;



typedef struct{
	char overridden[32];
	char upgrade_mode[32];
	char store_media[64];
	char firmware_name[64];
	char fw_store_dir[64];
	char down_url[256];
}config_ini_t;

#define CONF_SECTOR          448
#define UPDATE_FLAGS         0xa5a5a5a5
typedef struct 
{
    int local_dir;
    unsigned int flags;
    char media_name[64];
}update_cfg_t;

typedef struct{
	char fw_path[128];
	char boot0[128];
	char uboot[128];
}fw_path_t;

extern fw_path_t firm_path;
#define IMG_PATH                  firm_path.fw_path
#define UBOOT                     firm_path.uboot
#define BOOT0                     firm_path.boot0


#define CONFIG_INI                "/etc/config.ini"


static inline int unlock_boot_part(void)
{
	int fd;
	int ret;

	fd = open(BOOT0_PERM, O_RDWR);
	if(fd < 0)
		return -1;

	ret = write(fd, "0", 1);
	close(fd);

	return ret;
}

static inline int lock_boot_part(void)
{
	int fd;
	int ret;

	fd = open(BOOT0_PERM, O_RDWR);
	if(fd < 0)
		return -1;

	ret = write(fd, "1", 1);
	close(fd);

	return ret;
}

static int wait_dev_ready(const char *media)
{
    int fd = -1;
	int count = 0;
    char path[128];

    memset(path, 0, sizeof(path));
    snprintf(path, sizeof(path), "/dev/%s", media);
	LOGE("Test device %s ready\n", path);
retry:
	fd = open(path, O_RDONLY);
	if(fd < 0){
		sleep(2);
		count ++;
		if(count < 10)
		    goto retry;

		return -1;
	}

	if(fd > 0)
		close(fd);

	return 0;
}
#endif /*__MISC_MESSAGE_H__*/
