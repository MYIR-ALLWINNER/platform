#!/bin/bash

#
# cp ./busybox/init ${TARGET_DIR}/
#

pwd=`cd $(dirname $0);pwd -P`

add_init_to_ramfs()
{
	if [ -L ${TARGET_DIR}/init ]; then
		rm ${TARGET_DIR}/init
		cp -f ${pwd}/busybox/init ${TARGET_DIR}/init
	else
		cp -f ${pwd}/busybox/init ${TARGET_DIR}/init
	fi
}

add_init_to_ramfs
