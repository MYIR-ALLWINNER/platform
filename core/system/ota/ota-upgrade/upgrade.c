#include "upgrade.h"
#include "misc.h"
#if 0
#include "spare_head.h"
#include "privite_uboot.h"
#endif

int main(int argc, char* argv[])
{
    int ret = 0;	
    ret = parse_init();
	if(ret < 0){
		LOGE("parse_init error !\n");
		goto err_ret;
	}
	ret = get_env();
	if(ret < 0){
		LOGE("parse command line failed!\n");
		goto err_ret;		
	}
	ret = firmware_update();
	if(ret < 0)
		goto err_ret;

	LOGE("system will be reboot after 5s to continue \n" );
	LOGE("*****Don't turn off the power!!!*****\n");
	sleep(5);
	reboot(RB_AUTOBOOT);
err_ret:
    
	return ret;
}