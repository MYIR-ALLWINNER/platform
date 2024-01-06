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

#include "sys_info.h"

static int product_info(void)
{
	char chip_id[128]={0};
	char brand[64]={0};
	char name[64]={0};
	char device[64]={0};
	char model[64]={0};
	char manufacturer[64]={0};
	int ret = -1;

	ret = sys_get_chipid(chip_id);
	if(ret != 0){
		printf("err: sys_get_chipid failed\n");
		goto end;
	}

	ret = sys_get_product_brand(brand);
	if(ret != 0){
		printf("err: sys_get_product_brand failed\n");
		goto end;
	}

	ret = sys_get_product_name(name);
	if(ret != 0){
		printf("err: sys_get_product_name failed\n");
		goto end;
	}

	ret = sys_get_product_device(device);
	if(ret != 0){
		printf("err: sys_get_product_device failed\n");
		goto end;
	}

	ret = sys_get_product_model(model);
	if(ret != 0){
		printf("err: sys_get_product_model failed\n");
		goto end;
	}

	ret = sys_get_product_manufacturer(manufacturer);
	if(ret != 0){
		printf("err: sys_get_product_manufacturer failed\n");
		goto end;
	}

end:
	printf("\nproduct info: \n");
	printf("    chip id : %s\n", chip_id);
	printf("    brand   : %s\n", brand);
	printf("    name    : %s\n", name);
	printf("    device  : %s\n", device);
	printf("    model   : %s\n", model);
	printf("    manufacturer : %s\n", manufacturer);

	return ret;
}

static int sysinfo(void)
{
	char kernel_release[16]={0};
	char kernel_version[64]={0};
	char kernel_version_ex[64]={0};
	char building_system[64]={0};
	char system_version[64]={0};
	char build_date[64]={0};
	int secure = 0;
	int ret = 0;

	ret = sys_get_kernel_release(kernel_release);
	if(ret != 0){
		printf("err: sys_get_kernel_release failed\n");
		goto end;
	}

	ret = sys_get_kernel_version(kernel_version);
	if(ret != 0){
		printf("err: sys_get_kernel_version failed\n");
		goto end;
	}
	
	sprintf(kernel_version_ex, "%s %s", kernel_release, kernel_version);

	ret = sys_get_building_system(building_system);
	if(ret != 0){
		printf("err: sys_get_building_system failed\n");
		goto end;
	}

	ret = sys_get_system_version(system_version);
	if(ret != 0){
		printf("err: sys_get_system_version failed\n");
		goto end;
	}

end:
	printf("\nsystem info: \n");
	printf("    kernel version  : %s\n", kernel_version_ex);
	printf("    building system : %s\n", building_system);
	printf("    system version  : %s\n", system_version);

	return ret;
}

int main(void)
{
	product_info();
	sysinfo();

	return 0;
}
