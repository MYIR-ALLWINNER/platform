#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "upgrade.h"
#include "misc.h"

img_hdl_t *img_hdl;
size_t img_size;
sunxi_download_info dl_map;
char *mbr_ptr;
config_ini_t configure;
fw_path_t firm_path;
static update_cfg_t conf;
char fw_path[64];

#define PING_CNT     4
int update_part(const char *, const char *, loff_t);

static int hostname_to_ip(const char *name, char *ip)
{
	struct hostent *h = NULL;

	h = gethostbyname(name);
	if(!h){
		LOGE("get host by name retur %d\n", (int)h);
		return -1;
	}
	strncpy(ip, inet_ntoa(*((struct in_addr *)h->h_addr)),  64);
	return 0;
}

static int test_resp(const char *path)
{
    int ret;
	struct stat st;

	assert(path);
	ret = lstat(path, &st);
	if(ret < 0){
		perror("test resp");
		return -1;
	}

	ret = st.st_size;
	return ret;
}
static int ping_server(const char *server)
{
    char *start;
	char *end;
	int size;
	int status;
	char out[128];
    char buf[256];
	char ip[64];
    assert(server);
#ifndef __DEF_DEBUG__
    LOGE("server: %s\n", server);
#endif
	start = strstr(server, "http://");
	if(start){
		size = strlen("http://");
		goto found_server;
	}

	start = strstr(server, "https://");
	if(start){
	    size = strlen("https://");
		goto found_server;
	}

	start = strstr(server, "ftp://");
	if(start){
		size = strlen("ftp://");
		goto found_server;
	}
	start = NULL;
	size = -1;

found_server:
    if(size < 0 || start == NULL){
		LOGE("can not to parse server address %d\n", __LINE__);
		return -1;
    }
    start += size;
    end = strchr(start, '/');
	if(end == NULL)
        end = strchr(start, '\0');
    if(end)
	    size = end - start;
    else
	    size = -1;
    if(size > 0){
	    strncpy(out, start, size);
	    out[size] = '\0';
    }else{
        LOGE("can not to parse server address %d\n", __LINE__);
	    return -1;
    }
	
#ifdef __DEF_DEBUG__
    LOGE("Server name:%s\n", out);
#endif

    if(hostname_to_ip(out, ip) <0)
		return -1;
#ifdef __DEF_DEBUG__
	LOGE("Host ip: %s\n", ip);
#endif
	snprintf(buf, sizeof(buf), \
		"ping -c %d %s | grep \"%d received\" | cat > /.resp", \
		 PING_CNT, ip, PING_CNT);
	LOGE("%s\n", buf);
	status = system(buf);
	if(status == -1){
		LOGE("ping server %s return %d\n", out, status);
		return -1;
	}

	if(WIFEXITED(status)){
		if(WEXITSTATUS(status) != 0){
			LOGE("No reponse when ping %s\n", out);
			return WEXITSTATUS(status);
		}

		return test_resp("/.resp");

	}else{
	    LOGE("shell running return error \n");

		return -1;
	}

	return -1;
}

static int download_fw(const char *server, const char *dir)
{
    int status;
    char buf[256];

    if(ping_server(server) <= 0){
		return -1;
    }
	LOGE("ping server ok \n");
	LOGE("Download dir:%s\n", dir);
    memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "wget %s -t 2 -P %s", server, dir);
	status = system(buf);
	if(WIFEXITED(status))
		if(WEXITSTATUS(status) != 0)
			return WEXITSTATUS(status);
			
	return 0;
}

static int try_to_open(const char *path)
{
    int fd = -1;
	fd = open(path, O_RDONLY);
	if(fd >= 0){
		close(fd);
		return 1;
	}
	return 0;
}
static img_hdl_t *get_fw_info(const char *name)
{
	int ret;
	FILE *fp;
	img_hdl_t *img_hdl = NULL;
	int item_table_size;

	printf("%s:%d: IMG_PATH=%s, name=%s\n", __func__, __LINE__, IMG_PATH, name);
	
	assert(name != NULL);
	fp = fopen(name, "rb");
	if(fp == NULL){
		LOGE("Open firmware(%s) failed or firmwae not exist\n", name);
		goto open_fw_failed;
	}
	
	img_hdl = (img_hdl_t *)malloc(sizeof(img_hdl_t));
	if(img_hdl == NULL){
		LOGE("%s get memory failed!\n", __func__);
		goto get_memory_failed;
	}
	memset(img_hdl, 0, sizeof(*img_hdl));
	
	ret = fread(&img_hdl->img_hdr, 1024, 1, fp);
	if(ret <= 0){
		LOGE("read img informations failed!\n");
		goto read_fw_failed;
	}
	
	if(memcmp(img_hdl->img_hdr.magic, IMAGE_MAGIC, 8)){
		LOGE("verify image magic err!\n");
        goto read_fw_failed;
	}
	
	item_table_size = img_hdl->img_hdr.itemcount * sizeof(img_item_t);
	img_hdl->item_tbl = malloc(item_table_size);
	if(img_hdl->item_tbl == NULL){
		LOGE("get img_hdr memory failed!\n");
		goto read_fw_failed;
	}
	
	ret = fread(img_hdl->item_tbl, item_table_size, 1, fp);
	if(ret <= 0){
		LOGE("Reading item table failed!\n");
		goto read_item_failed;
	}
	return img_hdl;	
		
read_item_failed:
    free(img_hdl->item_tbl);
read_fw_failed:
	free(img_hdl);
get_memory_failed:
	fclose(fp);
	fp = NULL;
open_fw_failed:	
	return NULL;
}

static int verify_fw(const char *buf)
{
    return 0;
}

static inline size_t get_fw_size(const void *img)
{	
	return (((img_hdl_t *)img)->img_hdr.lenLo);
}

static void *get_fw_items(const void *img, const char *major_type, const char *minor_stype)
{
	int i;
	img_hdl_t *img_hdl;
	item_hdl_t *pitem;
	
    assert(img);
	assert(major_type);
	assert(minor_stype);
	
	pitem = malloc(sizeof(item_hdl_t));
	if(pitem == NULL){
		LOGE("%s %d get memory failed!\n", __func__, __LINE__);
		return NULL;
	}
	img_hdl = (img_hdl_t *)img;
	
	pitem->index = ~0;
	
	for(i=0; i<img_hdl->img_hdr.itemcount; i++){
		if(!memcmp(img_hdl->item_tbl[i].subType, minor_stype, SUBTYPE_LEN)){
			pitem->index = i;
			return pitem;
		}
	}
	
	LOGE("%s %d find items failed!\n", __func__, __LINE__);
	free(pitem);
	return NULL;
}


static int read_item(const void *img, const item_hdl_t *pitem, void *dest, size_t size)
{
	size_t len;
	__uint64_t ofs;
	img_hdl_t *img_hdl = (img_hdl_t *)img;
	int fd;
	int ret;
	
	assert(img);
	assert(pitem);
	assert(dest);
	
	if(img_hdl->item_tbl[pitem->index].filelenHi){
		LOGE("%s %d Error items size \n", __func__, __LINE__);
		return -1;
	}
	
	len = (size_t)img_hdl->item_tbl[pitem->index].filelenLo;
	len = ROUND_UP(len, 1024);

#ifdef __DEF_DEBUG__
	LOGE("Item size:%d\n", len);
#endif
	if(len > size){
		LOGE("%s %d Item too large, request size %d:!\n", __func__, __LINE__, len);
		return -1;
	}
		
	ofs = (img_hdl->item_tbl[pitem->index].offsetHi) << 32;
	ofs |= img_hdl->item_tbl[pitem->index].offsetLo;

#ifdef __DEF_DEBUG__	
	LOGE("Read max %d Bytes from offset: %lld\n", size, ofs);
#endif

	fd = open(IMG_PATH, O_RDONLY);
	if(fd < 0){
		LOGE("%s %d Open file %s failed\n", __func__, __LINE__, IMG_PATH);
		return -1;
	}
	
	lseek(fd, ofs, SEEK_SET);
	ret = read(fd, dest, len);
	if(ret <= 0){
		LOGE("%s %d Read failed!\n", __func__, __LINE__);
		close(fd);
		return -1;
	}
	
	close(fd);
	return len;
}

static int read_large_item1(const void *img, const item_hdl_t *pitem, const char *name)
{
	int len;
	int size;
	off_t ofs = 0;
	img_hdl_t *img_hdl = (img_hdl_t *)img;
	int fd;
	int fd1;
	int ret = -1;
	char *buf;
	char path[64];
	char out_name[128];
	
	assert(img);
	assert(pitem);

	LOGE("read item:%s\n", name);
	if(img_hdl->item_tbl[pitem->index].filelenHi){
		LOGE("%s %d Error items size \n", __func__, __LINE__);
		return -1;
	}
	
	len = img_hdl->item_tbl[pitem->index].filelenLo;
	len = ROUND_UP(len, 1024);
		
	ofs = (img_hdl->item_tbl[pitem->index].offsetHi) << 32;
	ofs |= img_hdl->item_tbl[pitem->index].offsetLo;
	
	LOGE("Read %d Bytes from fw offset: %lld\n", len, ofs);
	fd = open(IMG_PATH, O_RDONLY);
	if(fd < 0){
		LOGE("%s %d Open file failed\n", __func__, __LINE__);
		goto err_open_img;
	}

	memset(path, 0, sizeof(path));
	memcpy(path, fw_path, strlen(fw_path));
	//path[strlen(fw_path) - 1] = '\0';
	path[strlen(fw_path)] = '\0';
	snprintf(out_name, sizeof(out_name), "%s/.%s.fex", path, name);
	fd1 = open(out_name, O_RDWR | O_CREAT);
	if(fd1 < 0){
		LOGE("failed to open out file %s\n", out_name);
		ret = -1;
		goto err_open_out_file;
	}
	buf = malloc(EXTRACT_BUF_SIZE);
	if(!buf){
		LOGE("Allocate extract buffer failed! \n");
		ret = -1;
		goto err_get_memory;
	}
	size = EXTRACT_BUF_SIZE;
	lseek(fd, ofs, SEEK_SET);
	lseek(fd1, 0, SEEK_SET);
	if(len > size){
        while(len > size){
		    ret = read(fd, buf, size);
		    if(ret == size){
			   ret = write(fd1, buf, ret);
			   if(ret > 0){
			       len -= ret;
				   fsync(fd1);
			   }
		    }else{
		        LOGE("Reading failed %s %d\n", __func__, __LINE__);
				ret = -1;
				goto err_read_img;
			}
		};
		
		if(len > 0){
			ret = read(fd, buf, len);
			ret = write(fd1, buf, ret);
			if(ret > 0)
				fsync(fd1);
		}
	}
	else{
		if(len > 0){
			ret = read(fd, buf, len);
			ret = write(fd1, buf, ret);
			if(ret > 0)
				fsync(fd1);
		}
	}
err_read_img:
    free(buf);
err_get_memory:
    close(fd1);
err_open_out_file:
    close(fd);
err_open_img:
    return ret;
}

static int show_partitions_info(const void *data)
{
#if 1
	int i;
	const sunxi_download_info *info;
	const dl_one_part_info *part_info;
	char buffer[32];
	
	assert(data);
	
	info = (const sunxi_download_info *)data;
	LOGE("*************IMG MAP DUMP************\n");
	LOGE("total IMG part %d\n", info->download_count);
	LOGE("\n");
	for(part_info = info->one_part_info, i=0; i<info->download_count; i++, part_info++)
	{
		memset(buffer, 0, 32);
		memcpy(buffer, part_info->name, 16);
		LOGE("IMG part[%d] name          :%s\n", i, buffer);
		memset(buffer, 0, 32);
		memcpy(buffer, part_info->dl_filename, 16);
		LOGE("IMG part[%d] IMG file      :%s\n", i, buffer);
		memset(buffer, 0, 32);
		memcpy(buffer, part_info->vf_filename, 16);
		LOGE("IMG part[%d] verify file   :%s\n", i, buffer);
		LOGE("IMG part[%d] lenlo         :0x%x\n", i, part_info->lenlo);
		LOGE("IMG part[%d] addrlo        :0x%x\n", i, part_info->addrlo);
		LOGE("IMG part[%d] encrypt       :0x%x\n", i, part_info->encrypt);
		LOGE("IMG part[%d] verify        :0x%x\n", i, part_info->verify);
		LOGE("\n");
	}
#else
#endif
    return 0;
}

static int get_fw_map(const void *img, sunxi_download_info *map_info)
{
	int ret;
	item_hdl_t *pitem = NULL;
    assert(map_info);
	
	pitem = get_fw_items(img, "12345678", "1234567890DLINFO");
	assert(pitem);
	
	ret = read_item(img, pitem, (void *)map_info, sizeof(sunxi_download_info));
	if(ret > 0){
		sunxi_download_info *info;
		info = (sunxi_download_info *)map_info;
		if(memcmp(info->magic, SUNXI_MBR_MAGIC, strlen(SUNXI_MBR_MAGIC)))
		    ret = -1;
	}else{
		ret = -1;
	}
	if(pitem)
		free((void *)pitem);
	return ret;
}

static int show_mbr_info(const void *data)
{
#if 0
	sunxi_mbr_t *mbr = (sunxi_mbr_t *)data;
	sunxi_partition *part_info;
	unsigned int i;
	char buffer[32];

	LOGE("*************MBR DUMP***************\n");
	LOGE("total mbr part %d\n", mbr->PartCount);
	LOGE("\n");
	for(part_info = mbr->array, i=0; i<mbr->PartCount;i++, part_info++)
	{
		memset(buffer, 0, 32);
		memcpy(buffer, part_info->name, 16);
		LOGE("part[%d] name      :%s\n", i, buffer);
		memset(buffer, 0, 32);
		memcpy(buffer, part_info->classname, 16);
		LOGE("part[%d] classname :%s\n", i, buffer);
		LOGE("part[%d] addrlo    :0x%x\n", i, part_info->addrlo);
		LOGE("part[%d] lenlo     :0x%x\n", i, part_info->lenlo);
		LOGE("part[%d] user_type :%d\n", i, part_info->user_type);
		LOGE("part[%d] keydata   :%d\n", i, part_info->keydata);
		LOGE("part[%d] ro        :%d\n", i, part_info->ro);
		LOGE("\n");
	}
#else
#endif
    return 0;
}

static int get_fw_mbr(const void *img, void **data)
{
	int ret;
	char *buf;
	item_hdl_t *pitem = NULL;
	int mbr_cnt = SUNXI_MBR_COPY_NUM;
	
    assert(img);
	
	pitem = get_fw_items(img, "12345678", "1234567890___MBR");
	assert(pitem);
	buf = malloc(1 << 20);
	assert(buf);
    
	ret = read_item(img, pitem, buf, sizeof(sunxi_mbr_t) * mbr_cnt);
	if(ret > 0){
		sunxi_mbr_t *mbr = (sunxi_mbr_t *)buf;
		if(!memcmp(mbr->magic, SUNXI_MBR_MAGIC, strlen(SUNXI_MBR_MAGIC))){
		    *data = buf;
		}else
			ret = -1;
	}else{
		ret = -1;
	}
	
	if(pitem){
		free((void *)pitem);
	}
	return ret;
}

static int store_update_flags(const void *buf, off_t offset, size_t size)
{
    int fd;
	int ret = -1;

	fd = open(MMCBLK0, O_RDWR);
	if(fd < 0){
		LOGE("%s open device %s failed \n", __func__, MMCBLK0);
		return -1;
	}

	lseek(fd, offset, SEEK_SET);
	ret = write(fd, buf, size);
	fsync(fd);
	close(fd);

	return ret;
}

static int erase_boot0(void)
{
    int fd;
	int ret;
	char *buf;

	fd = open(MMCBOOT0, O_RDWR);
	if(fd < 0)
		return -1;
	buf = malloc(32 * 1024);
	if(!buf)
		return -1;
	memset(buf, 0, 32 * 1024);

	ret = write(fd, buf, 32 * 1024);
	free(buf);
	fsync(fd);
	close(fd);

	return ret;
}

static int extract_boot(const void *data, const void *name)
{
	int ret;
	int fd;
	int size;
	item_hdl_t *pitem = NULL;
	char *buf;
	int boot_type = -1;
	update_cfg_t *cfg;

	cfg = &conf;
	
	assert(data);
	assert(name);

    if(!strncmp(name, "boot0", 5)){
		boot_type = 1;
		pitem = get_fw_items(data, "12345678", "1234567890BOOT_0");
    }else if(!strncmp(name, "uboot", 5)){
        boot_type = 2;
	    pitem = get_fw_items(data, "12345678", "BOOTPKG-00000000");
    }else{
		LOGE("unkonwn partitions %s\n", name);
		ret = -1;
		goto err_get_item;
    }
	if(!pitem){
		LOGE("get uboot items failed! \n");
		ret = -1;
		goto err_get_item;
	}
	
	buf = malloc(UBOOT_BUF_SIZE);
	if(!buf){
		ret = -1;
		goto err_get_item;
	}

	ret = read_item(data, pitem, buf, UBOOT_BUF_SIZE);
    if(ret < 0){
		ret = -1;
		goto err_allocate;
    }
	
    size = ret;
    if(boot_type == 1){
		boot0_file_head_t  *head  = (boot0_file_head_t *)buf;
		if(strncmp((const char *)head->boot_head.magic, 
			BOOT0_MAGIC, strlen(BOOT0_MAGIC))){
			LOGE("boot0 Magic error!\n");
			ret = -1;
			goto err_ret;
		}
		fd = open(BOOT0, O_WRONLY | O_CREAT);
    }
	else if(boot_type == 2){
		sbrom_toc1_head_info_t *toc1 = (sbrom_toc1_head_info_t *)buf;
		if(toc1->magic != TOC_MAIN_INFO_MAGIC){
			LOGE("uboot pkg magic error!\n");
			ret = -1;
			goto err_ret;

		}
		LOGE("uboot_pkg magic 0x%x\n", toc1->magic);
        fd = open(UBOOT, O_WRONLY | O_CREAT);
    }
	else{
        ret = -1;
		goto err_ret;
    }
	if(fd < 0){
		LOGE("Open or create file for boot data failed! \n");
		ret = -1;
		goto err_ret;
	}
		
	ret = write(fd, buf, size);
	if(ret < 0){
		LOGE("Store boot raw data failed! \n");
		ret = -1;
		goto err_write;
	}
	fsync(fd);
    if(boot_type == 1){
		cfg->flags = UPDATE_FLAGS;
		LOGE("FW upgrade flags:0x%x, media:%s\n", cfg->flags, cfg->media_name);
		unlock_boot_part();
        update_part(MMCBLK0, BOOT0, 16 * 512);
        update_part(MMCBLK0, BOOT0, 256 * 512);
        store_update_flags((const void *)cfg, CONF_SECTOR * 512, 512 );
		erase_boot0();
		lock_boot_part();
    }else if(boot_type == 2){
		unlock_boot_part();
        update_part(MMCBLK0, UBOOT, 24576 * 512);
        update_part(MMCBLK0, UBOOT, 32800 * 512);
		lock_boot_part();
    }
	ret = 0;
err_write:
	close(fd);
err_ret:
	free(buf);
err_allocate:
	free((void *)pitem);
err_get_item:
	return ret;
}


static int extract_udisk(const void *img, void **data)
{

}
static int extract_fex(const void *data, const dl_one_part_info *part_info)
{
    int ret;
	int fd = -1;
	item_hdl_t *pitem = NULL;
	
    fd = open(IMG_PATH, O_RDONLY);
	if(fd >= 0){
		pitem = get_fw_items(data, "RFSFAT16", (char *)part_info->dl_filename);
		read_large_item1(data, pitem, (const char *)part_info->name);
		ret = 0;
	}
	else
		ret = -1;
	if(pitem)
		free((void *)pitem);

	return ret;
}

static int extract_normal(const void *data, const sunxi_download_info *dl_map)
{
	int i;
	int ret = 0;
	const dl_one_part_info *part_info;
	LOGE("Total %d dowload part\n", dl_map->download_count);
	for(part_info=dl_map->one_part_info, i=0; i<dl_map->download_count; i++, part_info++){
		LOGE("begin to extract %s form image\n", part_info->name);
        if(!strncmp("sysrecovery", (char*)part_info->name, strlen("sysrecovery"))){
			continue;
        }
        else if(!strncmp("private", (char*)part_info->name, strlen("private"))){
			continue;
        }

		else{
			ret = extract_fex(data, part_info);
		}
	}
	return ret;
}

static int get_img_info(img_hdl_t **out)
{
	img_hdl_t *img_hdl;

	printf("%s:%d: \n", __func__, __LINE__);

	assert(out);
	LOGE("firmware upgrade start! firmware name: %s\n", IMG_PATH);
	img_hdl = get_fw_info(IMG_PATH);
	if(img_hdl){
	    *out = img_hdl;	
	    return 0;
	}else
	    return -1;
}


static int write_part(const char *part, off_t offset, const char *buf, size_t size)
{
	int fd;
	int ret;
	int len;
	
	assert(part);
	assert(buf);
	
	LOGE("Write %d bytes to %s!\n", size, part);
	fd = open(part, O_RDWR);
	if(fd < 0){
		LOGE("Open partitions %s failed!\n", part);
		ret = -1;
	    goto open_failed;
	}
	len = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	lseek(fd, offset, SEEK_CUR);
	LOGE("partitions size = %d\n", len);
	ret = write(fd, buf, size);
	if(ret <= 0){
		LOGE("write partitions %s failed!, ret value=%d\n", part, ret);
		ret = -1;
	}
	LOGE("%d Bytes be written to %s \n", ret, part);
	fsync(fd);
	close(fd);	
open_failed:	
	return ret;
}

static int write_user_mbr(const void *data)
{
    int fd;
    int ret;
	int size = sizeof(sunxi_mbr_t) * SUNXI_MBR_COPY_NUM;
	
	fd = open(MMCBLK0, O_RDWR);
	if(fd < 0){
		LOGE("functions %s open %s partitions failed\n", __func__, MMCBLK0);
		return -1;
	}
	lseek(fd, 40960 * SECTOR_SIZE, SEEK_SET);
	ret = write(fd, data, size);
	fsync(fd);
	close(fd);

	return ret == size ? size : -1;
}

static inline int sunxi_sprite_size()
{
    return 0;
}

static int write_system_mbr(const void *data)
{
    int fd;
    char buf[SECTOR_SIZE];
	mbr_sys_t *sys_mbr;
	int i;
	int ret;
	int sectors;
	int unusd_sectors;

	const sunxi_mbr_t *mbr = (const sunxi_mbr_t *)data;
    assert(data);

	fd = open(MMCBLK0, O_RDWR);
	if(fd < 0){
		LOGE("functions %s open %s partitions failed\n", __func__, MMCBLK0);
		return -1;
	}
	
	lseek(fd, (40960 + 1) * SECTOR_SIZE, SEEK_SET);
	sectors = 0;
	for(i=1; i<mbr->PartCount-1; i++)
	{
		memset(buf, 0, SECTOR_SIZE);
		sys_mbr = (mbr_sys_t *)buf;

		sectors += mbr->array[i].lenlo;

		sys_mbr->part_info[0].part_type = 0x83;
		sys_mbr->part_info[0].start_sectorl = 
			    ((mbr->array[i].addrlo - i + 20 * 1024 * 1024/512 ) & 0x0000ffff) >> 0;
		sys_mbr->part_info[0].start_sectorh = 
			    ((mbr->array[i].addrlo - i + 20 * 1024 * 1024/512 ) & 0xffff0000) >> 16;
		sys_mbr->part_info[0].total_sectorsl = ( mbr->array[i].lenlo & 0x0000ffff) >> 0;
		sys_mbr->part_info[0].total_sectorsh = ( mbr->array[i].lenlo & 0xffff0000) >> 16;

		if(i != mbr->PartCount-2)
		{
			sys_mbr->part_info[1].part_type = 0x05;
			sys_mbr->part_info[1].start_sectorl = i;
			sys_mbr->part_info[1].start_sectorh = 0;
			sys_mbr->part_info[1].total_sectorsl = (mbr->array[i].lenlo  & 0x0000ffff);
			sys_mbr->part_info[1].total_sectorsh = (mbr->array[i].lenlo  & 0xffff0000) >> 16;
		}

		sys_mbr->end_flag = 0xAA55;
		if(write(fd, buf, SECTOR_SIZE) < 0)
		{
			LOGE("write system mbr %d failed\n", i);
			ret = -1;
			goto err_ret;
		}
		fsync(fd);
	}
	memset(buf, 0, 512);
	sys_mbr = (mbr_sys_t *)buf;

	unusd_sectors = sunxi_sprite_size() - 20 * 1024 * 1024/512 - sectors;
	sys_mbr->part_info[0].indicator = 0x80;
	sys_mbr->part_info[0].part_type = 0x0B;
	sys_mbr->part_info[0].start_sectorl  = 
		    ((mbr->array[mbr->PartCount-1].addrlo + 20 * 1024 * 1024/512 ) & 0x0000ffff) >> 0;
	sys_mbr->part_info[0].start_sectorh  = 
		    ((mbr->array[mbr->PartCount-1].addrlo + 20 * 1024 * 1024/512 ) & 0xffff0000) >> 16;
	sys_mbr->part_info[0].total_sectorsl = ( unusd_sectors & 0x0000ffff) >> 0;
	sys_mbr->part_info[0].total_sectorsh = ( unusd_sectors & 0xffff0000) >> 16;

	sys_mbr->part_info[1].part_type = 0x06;
	sys_mbr->part_info[1].start_sectorl  = 
		    ((mbr->array[0].addrlo + 20 * 1024 * 1024/512) & 0x0000ffff) >> 0;
	sys_mbr->part_info[1].start_sectorh  = 
		    ((mbr->array[0].addrlo + 20 * 1024 * 1024/512) & 0xffff0000) >> 16;
	sys_mbr->part_info[1].total_sectorsl = (mbr->array[0].lenlo  & 0x0000ffff) >> 0;
	sys_mbr->part_info[1].total_sectorsh = (mbr->array[0].lenlo  & 0xffff0000) >> 16;

	sys_mbr->part_info[2].part_type = 0x05;
	sys_mbr->part_info[2].start_sectorl  = 1;
	sys_mbr->part_info[2].start_sectorh  = 0;
	sys_mbr->part_info[2].total_sectorsl = (sectors & 0x0000ffff) >> 0;
	sys_mbr->part_info[2].total_sectorsh = (sectors & 0xffff0000) >> 16;
	sys_mbr->end_flag = 0xAA55;

    i = 0;
	lseek(fd, (off_t)i, SEEK_SET);
	if(write(fd, buf, SECTOR_SIZE) <= 0){
		LOGE("write system mbr %d failed\n", i);
		ret = -1;
		goto err_ret;
	}
	ret = 0;
	fsync(fd);
err_ret:
	close(fd);
	return ret;
}

int update_part(const char *partitons, const char *path, loff_t offset)
{
	int fd = -1;
	char *buf = NULL;
	int ret;
	size_t size;
	struct stat st;
	
	assert(path);
	assert(partitons);
	
	LOGE("%s will be written to %s\n", path, partitons);
    fd = open(path, O_RDWR);
	if(fd < 0){
		LOGE("%s open failed!\n", path);
		return -1;
	}

	memset(&st, 0, sizeof(struct stat));
	fstat(fd, &st);
	size = ROUND_UP(st.st_size, 1024);
	LOGE("update firmware size : %d\n", size);
	buf = malloc(size);
	if(!buf){
		perror("allocator partitions");
		ret = -1;
		goto err_ret;
	}
	memset(buf, 0, size);
	ret = read(fd, buf, st.st_size);
	close(fd);
	if(ret <= 0){
		perror("reading fex file");
		ret = -1;
        goto err_ret;
	}
    LOGE("Total reading %d Bytes raw data \n", ret);
	write_part(partitons, offset, buf, (size_t)ret);
err_ret:	
	if(buf)
		free(buf);
	return ret;
}

static int write_normal_part(const sunxi_download_info *dl_map)
{
	int i;
	char *dev_str;
	char *name;
	char path[64];
	char buf[128];
	const dl_one_part_info *part_info;

    assert(dl_map);
    memset(path, 0, sizeof(path));
	memcpy(path, fw_path, strlen(fw_path));
	//path[strlen(fw_path) - 1] = '\0';
	path[strlen(fw_path)] = '\0';
	for(part_info=dl_map->one_part_info, i=0; i<dl_map->download_count; i++, part_info++){
        name = (char*)part_info->name;
		if(!strncmp("rootfs", name, strlen("rootfs")) ||
		   !strncmp("private", name, strlen("private")) ||
		   !strncmp("sysrecovery", name, strlen("sysrecovery")))
			continue;

        dev_str = getenv(name);
		if(dev_str){
	        snprintf(buf, sizeof(buf), "%s/.%s.fex", path, name);
			update_part(dev_str, buf, 0);
		}
	}

	return 0;
}

int firmware_update()
{
	int ret;
	size_t size;
	char path[256];
	struct stat st;

	printf("%s:%d: flags=%d, media_name=%s\n", __func__, __LINE__, conf.flags, conf.media_name);

	if(!strncmp(configure.upgrade_mode, "server", strlen("server"))){

        memset(path, 0, sizeof(path));
		if(!strncmp(configure.overridden, "yes", strlen("yes"))){
			snprintf(path, sizeof(path), "rm -f %s", IMG_PATH);
			system(path);
			memset(path, 0, sizeof(path));
		}
		snprintf(path, sizeof(path), "%s%s", configure.down_url,
			           configure.firmware_name);
		ret = download_fw(path, fw_path);
		if(ret != 0 && ret != -1){
            LOGE("Download firmware failed, try to exist\n");
			if(!try_to_open(IMG_PATH))	
			    return -1;
		}
		if(!strlen(conf.media_name))
		    goto ok_continue;
	}
#if 0
	LOGE("Waitting device %s\n", conf.media_name);
	if(wait_dev_ready(conf.media_name) < 0)
		return -1;
#endif
	
ok_continue:	
	ret = get_img_info(&img_hdl);
	if(ret < 0)
		return ret;
	size = get_fw_size((void *)img_hdl);
	lstat(IMG_PATH, &st);
	if(st.st_size != size){
		LOGE("firmware size error, recoder size:%d, really size%d\n",
			   size, st.st_size);
		return -1;
	}
	LOGE("firmware: %s size: %d\n", IMG_PATH, size);
	
	ret = get_fw_map(img_hdl, &dl_map);
	if(ret < 0){
		LOGE("fetch firmware image map failed!\n");
		return ret;
	}
	show_partitions_info((const void *)&dl_map);
	
	ret = get_fw_mbr((const *)img_hdl, (void **)&mbr_ptr);
	if(ret < 0){
		LOGE("fetch firmware MBR failed!\n");
		return ret;
	}
	show_mbr_info(mbr_ptr);
	ret = extract_boot(img_hdl, "boot0");
	if(ret < 0){
		LOGE("extract && write boot0 failed \n");
		return ret;
	}
	ret = extract_boot(img_hdl, "uboot");
	if(ret < 0){
		LOGE("extract && write uboot failed \n");
		return ret;
	}
	
	extract_normal(img_hdl, &dl_map);
	write_user_mbr(mbr_ptr);
	write_system_mbr(mbr_ptr);

	ret = write_normal_part(&dl_map);

	if(img_hdl){
		free(img_hdl);
		img_hdl = NULL;
	}

	if(mbr_ptr){
		free(mbr_ptr);
		mbr_ptr = NULL;
	}
    return 0;
}

static int get_media_name(const char *str, int type)
{
    int dir_len;
	int dev_len;
	update_cfg_t *cfg = &conf;

    switch (type){
	case 0:
		dir_len = strlen("/mnt/sdcard/");
		dev_len = strlen(str) - dir_len;
	    memcpy(cfg->media_name, &str[dir_len], dev_len);
		cfg->media_name[dev_len] = '\0';
		break;
	case 1:
		dir_len = strlen("/mnt/usb/");
		dev_len = strlen(str) - dir_len;
	    memcpy(cfg->media_name, &str[dir_len], dev_len);
		cfg->media_name[dev_len] = '\0';		
		break;
	case 2:
	    cfg->local_dir = 1;
	    strncpy(cfg->media_name, str, sizeof(cfg->media_name));
	}

    LOGE("block storage: %s\n", cfg->media_name);
    return 0;
}
static int get_ota_path(const void *data)
{
    int fd;
	int ret;
	size_t size = 0;
	int media = -1;
	struct stat st;
	char buf[64];
	char *start;
	fw_path_t *path = &firm_path;;
    const config_ini_t *ini;

	assert(data);
	ini = (const config_ini_t *)data;

    memset(buf, 0, sizeof(buf));
    memset(fw_path, 0, sizeof(fw_path));
    if(!strncmp(ini->upgrade_mode, "server", strlen("server"))){
		size = strlen(ini->fw_store_dir);
	    if(size > 0){
		    strncpy(fw_path, ini->fw_store_dir, size);
		    fw_path[size] = '\0';
		    goto found_store_path;
	    }else{
	        media = 3;
	    }
    }
    memset(buf, 0, sizeof(buf));
	if(strlen(ini->fw_store_dir)){
		strncpy(fw_path, ini->fw_store_dir, sizeof(fw_path));
		goto found_store_path;
	}
    if(!strcmp(ini->store_media, "sd")){
		int cnt = 0;
		media = 0;
try_sd_ready:
		system("ls /mnt/sdcard > /.store");
		lstat("/.store", &st);
		if(st.st_size < 4){
			if(cnt ++ < 5){
				sleep(5);
			    goto try_sd_ready;
			}
			else{
				media = 3;
				goto set_local_dir;
			}
		}
		goto get_media_str;
    }
	else if(!strcmp(ini->store_media, "usb")){
		int cnt = 0;
		media = 1;		
try_usb_ready:
		system("ls /mnt/usb > /.store");
		lstat("/.store", &st);
		if(st.st_size < 4){
			if(cnt ++ < 5){
				sleep(5);
				goto try_usb_ready;
			}
			else{
				media = 3;
				goto set_local_dir;
			}
		}
		goto get_media_str;
	}
set_local_dir:
	if(media == 3){
	    strncpy(fw_path, "/extp/", strlen("/extp/"));
		fw_path[strlen("/extp/")] = '\0';
		goto found_store_path;
	}

get_media_str:
	fd = open("/.store", O_RDONLY);
	if(fd < 0){
		LOGE("failed to open firmware path store files \n");
		return -1;
	}
	lseek(fd, 0, SEEK_SET);
	ret = read(fd, buf, sizeof(buf));
	if(ret < 0){
		LOGE("failed to read firmware path store files \n");
		close(fd);
		return -1;
	}
	close(fd);
	system("rm -f /.store");

    start = NULL;
	start = strchr(buf, '\r');
	if(start == NULL){
		start = strchr(buf, '\n');
	}
	memset(start, 0, buf + strlen(buf) - start);

	/*LOGE("#######XXXXXXX%s\n", buf);*/
	if(media == 0){
	    snprintf(fw_path, 64, "/mnt/sdcard/%s", buf);
	}
	else if(media == 1){
		snprintf(fw_path, 64, "/mnt/usb/%s", buf);
	}

found_store_path:
	LOGE("firmware path        :%s\n", fw_path);
    memset(buf, 0, sizeof(buf));
	size = strlen(fw_path);
	memcpy(buf, fw_path, size);
	//buf[size - 1] = '\0';
	buf[size] = '\0';
	snprintf(path->fw_path, 128,  "%s/%s", buf, configure.firmware_name);
	snprintf(path->boot0, 64,     "%s/.boot0.fex", buf);
	snprintf(path->uboot, 64,     "%s/.uboot.fex", buf);
	get_media_name(buf, media);

	printf("%s:%d: firm_path.fw_path=%s\n", __func__, __LINE__,firm_path.fw_path);
	printf("%s:%d: firm_path.boot0=%s\n", __func__, __LINE__,firm_path.boot0);
	printf("%s:%d: firm_path.uboot=%s\n", __func__, __LINE__,firm_path.uboot);
	printf("%s:%d: conf.flags=%d\n", __func__, __LINE__,conf.flags);
	printf("%s:%d: conf.media_name=%s\n", __func__, __LINE__,conf.media_name);

	return 0;
}

int parse_init(void)
{
    int fd;
    char *buf;

	fd = open(CONFIG_INI, O_RDWR);
	/*fd = open("/config.ini", O_RDWR);*/
	if(fd <= 0){
		LOGE("read configure file failed!\n");
		return -1;
	}

    memset((void *)&configure, 0, sizeof(configure));
    buf = malloc(4096);
	if(buf){
		char *start;
		char *end;
		if(read(fd, buf, 4096) > 0){
			close(fd);
			start = strstr(buf, "overridden=");
			if(start){
				start += strlen("overridden=");
				end = strchr(start, ';');
				if(end){
					strncpy(configure.overridden, start, end - start);
					configure.overridden[end - start] = '\0';
				}
			}
		    start = strstr(buf, "upgrade_mode=");
			if(start){
				start += strlen("upgrade_mode=");
				end = strchr(start, ';');
				if(end){
					strncpy(configure.upgrade_mode, start, end - start);
					configure.upgrade_mode[end - start] = '\0';
				}
			}
			start = strstr(buf, "firmware_name=");
			if(start){
				start += strlen("firmware_name=");
				end = strchr(start, ';');
				if(end){
					strncpy(configure.firmware_name, start, end - start);
				    configure.firmware_name[end - start] = '\0';
			    }
			}
			start = strstr(buf, "fw_store_dir=");
			if(start){
				start += strlen("fw_store_dir=");
				end = strchr(start, ';');
				if(end){
					strncpy(configure.fw_store_dir, start, end - start);
					configure.fw_store_dir[end - start] = '\0';
				}
			}

			start = strstr(buf, "down_url=");
			if(start){
				start += strlen("down_url=");
				end = strchr(start, ';');
				if(end){
					strncpy(configure.down_url, start, end - start);
					configure.down_url[end - start] = '\0';
				}
			}

			start = strstr(buf, "store_media=");
			if(start){
				start += strlen("store_media=");
				end = strchr(start, ';');
				if(end){
					strncpy(configure.store_media, start, end - start);
					configure.store_media[end - start] = '\0';
				}
			}
#ifdef __DEF_DEBUG__

			LOGE("upgrade configure file:\n");
			LOGE("store_media           :%s\n", configure.store_media);
			LOGE("upgrade_mode          :%s\n", configure.upgrade_mode);
			LOGE("down_url              :%s\n", configure.down_url);
	     	LOGE("firmware_name         :%s\n", configure.firmware_name);
#endif
		}
	}else{
	    close(fd);
		return -1;
	}
    free(buf);
	return get_ota_path((const char *)&configure);
}
