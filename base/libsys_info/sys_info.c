/*
 * Copyright (C) 2016 Allwinnertech
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/rtc.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/utsname.h>

#define LONGAN_CONFIG_PATH "/etc/longan.conf"

#define SYS_INFO_NODE	"/dev/sunxi_soc_info"
#define CHECK_SOC_SECURE_ATTR 0x00
#define CHECK_SOC_VERSION     0x01
#define CHECK_SOC_CHIPID      0x04

static int get_value_from_file(char *file_path, char *str, void *buf)
{
	FILE *fp = NULL;
	char buffer[256] = {0};
	char *temp_str = NULL;
	int ret = 0;

	fp = fopen(file_path,"r");
	if ( fp == NULL ){
		printf("err: open %s failed\n", file_path);
		return -1;
	}

	memset(buffer, 0, sizeof(buffer));
	while(fgets(buffer, sizeof(buffer), fp)){
		temp_str = strstr(buffer, str);
		if (temp_str != NULL) {
			temp_str = strrchr(buffer, '=');
			if (temp_str != NULL) {
				//jump '='
				if(*temp_str == '='){
					temp_str++;
				}

				//jump ''
				while(*temp_str == ' '){
					temp_str++;
				}

				//delete line breaks '\n'
				if (temp_str[strlen(temp_str)-1] == '\n'){
					temp_str[strlen(temp_str)-1] = '\0';
				}

				strcpy((char *)buf, temp_str);

				break;
			}
		}

		memset(buffer, 0, sizeof(buffer));
	}
	fclose(fp);

	return ret;
}

static int get_value_for_sysinfo(char *node_path, unsigned int cmd, void *buf)
{
	int fd = 0;
	int ret = 0;

	fd = open(node_path, O_RDWR);
	if (fd == 1) {
		printf("open %s error.\n", node_path);
		return -1;
	}

	ret = ioctl(fd, cmd, (unsigned long)buf);
	if (ret != 0) {
		printf("ioctl CHECK_SOC_CHIPID error, %d\n", errno);
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

int sys_get_chipid(void *id)
{
	int ret = 0;

	ret = get_value_for_sysinfo(SYS_INFO_NODE, CHECK_SOC_CHIPID, id);
	if(ret != 0){
		printf("err: sys_get_chipid failed\n");
		return -1;
	}

	return 0;
}

static int get_uname(char *release, char *version)
{
	int ret = 0;
	struct utsname utsname;

	memset(&utsname, 0, sizeof(struct utsname));
	ret = uname(&utsname);
    if(ret != 0){
        printf("uname failed");
        return -1;
    }

/*
    printf("sysname:%s\n nodename:%s\n release:%s\n version:%s\n machine:%s\n \n ",\
                    utsname.sysname,\
                    utsname.nodename,\
                    utsname.release,\
                    utsname.version,\
                    utsname.machine);
*/

	if(release){
		strcpy(release, utsname.release);
	}

	if(version){
		strcpy(version, utsname.version);
	}

	return 0;
}

int sys_get_kernel_release(void *release)
{
	return get_uname(release, NULL);
}

int sys_get_kernel_version(void *version)
{
	return get_uname(NULL, version);
}

int sys_get_building_system(void *system)
{
	return get_value_from_file(LONGAN_CONFIG_PATH, "BUILDING_SYSTEM", system);
}

int sys_get_system_version(void *version)
{
	return get_value_from_file(LONGAN_CONFIG_PATH, "SYSTEM_VERSION", version);
}

int sys_get_product_brand(void *brand)
{
	return get_value_from_file(LONGAN_CONFIG_PATH, "PRODUCT_BRAND", brand);
}

int sys_get_product_name(void *name)
{
	return get_value_from_file(LONGAN_CONFIG_PATH, "PRODUCT_NAME", name);
}

int sys_get_product_device(void *device)
{
	return get_value_from_file(LONGAN_CONFIG_PATH, "PRODUCT_DEVICE", device);
}

int sys_get_product_model(void *model)
{
	return get_value_from_file(LONGAN_CONFIG_PATH, "PRODUCT_MODEL", model);
}

int sys_get_product_manufacturer(void *manufacturer)
{
	return get_value_from_file(LONGAN_CONFIG_PATH, "PRODUCT_MANUFACTURER", manufacturer);
}
