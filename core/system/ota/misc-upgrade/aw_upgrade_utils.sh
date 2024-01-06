#!/bin/sh

#err code
ERR_ILLEGAL_ARGS=2
ERR_NETWORK_FAILED=3
ERR_NOT_ENOUGH_SPACE=4
ERR_VENDOR_HOOK_NOT_SUPPORT=5
ERR_MD5_CHECK_FAILED=10

#image package name
RAMDISK_PKG=ramdisk_sys.tar.gz
TARGET_PKG=target_sys.tar.gz
USR_PKG=usr_sys.tar.gz

#image dir name
RAMDISK_DIR=ramdisk_sys
TARGET_DIR=target_sys
USR_DIR=usr_sys

#image name
RAMDISK_IMG=boot_initramfs.img
BOOT_IMG=boot.img
ROOTFS_IMG=rootfs.img
USR_IMG=usr.img

UPGRADE_SETTING_PATH_PROCD_INIT=/overlay/etc/.misc-upgrade/
UPGRADE_SETTING_PATH_BUSYBOX_INIT=/etc/.misc-upgrade/

#PID1=`busybox ps | busybox awk '$1==1 {print $5}'`
PID1=`busybox awk 'NR==1 {print $2}' /proc/1/status`
if [ x"$PID1" = x"init" ];then
	INIT_MODE="BUSYBOX_INIT"
else
	INIT_MODE="PROCD_INIT"
fi
if [ x"$INIT_MODE" = x"BUSYBOX_INIT" ];then
	UPGRADE_SETTING_PATH=$UPGRADE_SETTING_PATH_BUSYBOX_INIT
else
	UPGRADE_SETTING_PATH=$UPGRADE_SETTING_PATH_PROCD_INIT
fi
